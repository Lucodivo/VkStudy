
#include "vk_engine.h"

#define DEFAULT_NANOSEC_TIMEOUT 1'000'000'000

#define PRESENT_MODE VK_PRESENT_MODE_FIFO_KHR

#define MAX_OBJECTS 100'000

const VkClearValue colorClearValue{
        {0.1f, 0.1f, 0.1f, 1.0f}
};

const VkClearValue depthClearValue{
        {1.f, 0.f}
};

void VulkanEngine::init() {

  WindowManager::getInstance().getExtent(&windowExtent.width, &windowExtent.height);

  initVulkan();
  initSwapchain();
  initCommands();
  initDefaultRenderpass();
  initFramebuffers();
  initSyncStructures();
  initSamplers();
  initDescriptors();
  initPipelines();
  loadImages();
  loadMeshes();
  initScene();

  initImgui(); // move out?

  isInitialized = true;
}

void VulkanEngine::cleanup() {
  VK_CHECK(vkDeviceWaitIdle(device));

  if(isInitialized) {
    vkImguiDeinit(imguiInstance);
    cleanupSwapChain();
    mainDeletionQueue.flush();

    for(std::pair<std::string, Texture> pair: loadedTextures) {
      const Texture& texture = pair.second;
      vkDestroyImageView(device, texture.imageView, nullptr);
      vmaDestroyImage(vmaAllocator, texture.image.vkImage, texture.image.vmaAllocation);
    }
    loadedTextures.clear();

    vmaDestroyAllocator(vmaAllocator);
    materialManager.destroyAll(device);

    vkDestroyDevice(device, nullptr);
    vkDestroySurfaceKHR(instance, surface, nullptr);
    vkb::destroy_debug_utils_messenger(instance, debugMessenger);
    vkDestroyInstance(instance, nullptr);
  }
}

void VulkanEngine::draw(const Camera& camera) {
  FrameData& frame = getCurrentFrame();
  VK_CHECK(vkWaitForFences(device, 1, &frame.renderFence, true, DEFAULT_NANOSEC_TIMEOUT));
  VK_CHECK(vkResetFences(device, 1, &frame.renderFence));

  // present semaphore is signaled when the presentation engine has finished using the image and
  // it may now be used as a target for drawing
  u32 swapchainImageIndex;
  VkResult acquireResult = vkAcquireNextImageKHR(device, swapchain, DEFAULT_NANOSEC_TIMEOUT, frame.presentSemaphore, nullptr, &swapchainImageIndex);
  if(acquireResult == VK_ERROR_OUT_OF_DATE_KHR) {
    recreateSwapChain();
    acquireResult = vkAcquireNextImageKHR(device, swapchain, DEFAULT_NANOSEC_TIMEOUT, frame.presentSemaphore, nullptr, &swapchainImageIndex);
  }
  VK_CHECK(acquireResult);

  VkCommandBuffer cmd = frame.mainCommandBuffer;
  VK_CHECK(vkResetCommandBuffer(cmd, 0));

  VkCommandBufferBeginInfo cmdBeginInfo = vkinit::commandBufferBeginInfo(VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT);

  VK_CHECK(vkBeginCommandBuffer(cmd, &cmdBeginInfo));
  {
    VkClearValue clearValues[] = {colorClearValue, depthClearValue};

    VkRenderPassBeginInfo rpInfo = vkinit::renderPassBeginInfo(renderPass, windowExtent, framebuffers[swapchainImageIndex]);
    rpInfo.clearValueCount = 2;
    rpInfo.pClearValues = clearValues; // Clear values for each attachment

    // binds the framebuffers, clears the color & depth images and puts the image in the layout specified in the renderpass
    vkCmdBeginRenderPass(cmd, &rpInfo, VK_SUBPASS_CONTENTS_INLINE);

    drawFragmentShader(cmd);
    drawObjects(cmd, camera,renderables.data(), (u32)renderables.size());
    vkImguiRender(cmd);

    // finishes render and transitions image to layout specified in renderpass
    vkCmdEndRenderPass(cmd);
  }
  VK_CHECK(vkEndCommandBuffer(cmd));

  VkSubmitInfo submitInfo = {};
  submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
  submitInfo.pNext = nullptr;

  VkPipelineStageFlags waitStage = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;

  submitInfo.pWaitDstStageMask = &waitStage;
  //we want to wait on the presentSemaphore, as that semaphore is signaled when the swapchain is ready
  submitInfo.waitSemaphoreCount = 1;
  submitInfo.pWaitSemaphores = &frame.presentSemaphore;
  //we will signal the renderSemaphore, to signal that rendering has finished
  submitInfo.signalSemaphoreCount = 1;
  submitInfo.pSignalSemaphores = &frame.renderSemaphore;
  submitInfo.commandBufferCount = 1;
  submitInfo.pCommandBuffers = &cmd;

  // renderFence will now block until the graphic commands finish execution
  VK_CHECK(vkQueueSubmit(graphicsQueue, 1, &submitInfo, frame.renderFence));

  VkPresentInfoKHR presentInfo = {};
  presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
  presentInfo.pNext = nullptr;
  presentInfo.pSwapchains = &swapchain;
  presentInfo.swapchainCount = 1;
  presentInfo.pWaitSemaphores = &frame.renderSemaphore;
  presentInfo.waitSemaphoreCount = 1;
  presentInfo.pImageIndices = &swapchainImageIndex;

  VkResult presentResult = vkQueuePresentKHR(graphicsQueue, &presentInfo);
  if(presentResult == VK_ERROR_OUT_OF_DATE_KHR || presentResult == VK_SUBOPTIMAL_KHR) {
    recreateSwapChain();
  } else {
    VK_CHECK(presentResult);
  }

  frameNumber++;
}

void VulkanEngine::initFrame(const bool showRenderDebugInfo) {
  renderDebugInfoRequestedForFrame = showRenderDebugInfo;
  vkImguiStartFrame();
}

FrameData& VulkanEngine::getCurrentFrame() {
  return frames[frameNumber % FRAME_OVERLAP];
}

void VulkanEngine::initVulkan() {
  vkb::InstanceBuilder builder;

  vkb::Instance vkbInst = builder
          .set_app_name("Example Vulkan Application")
          .request_validation_layers(true)
          .require_api_version(1, 1, 0)
          .use_default_debug_messenger()
          //.enable_extension("VK_KHR_Maintenance1") // needed for Vulkan versions <1.1 when using negative viewport valuesto perform y-inversion of the clip space
          .build()
          .value();

  instance = vkbInst.instance;
  debugMessenger = vkbInst.debug_messenger;

  WindowManager::getInstance().createSurface(instance, &surface, &windowExtent.width, &windowExtent.height);

//  VkPhysicalDeviceFeatures physicalDeviceFeatures{};
//  physicalDeviceFeatures.fillModeNonSolid = VK_TRUE;

  vkb::PhysicalDeviceSelector selector{vkbInst};
  vkb::PhysicalDevice physicalDevice = selector
          .set_minimum_version(1, 1)
          .set_surface(surface)
          .require_present()
//          .set_required_features(physicalDeviceFeatures)
          .select()
          .value();

  vkb::DeviceBuilder deviceBuilder{physicalDevice};

  vkb::Device vkbDevice = deviceBuilder
          .build()
          .value();

  device = vkbDevice.device;
  chosenGPU = physicalDevice.physical_device;

  graphicsQueue = vkbDevice.get_queue(vkb::QueueType::graphics).value();
  graphicsQueueFamily = vkbDevice.get_queue_index(vkb::QueueType::graphics).value();

  vkGetPhysicalDeviceProperties(chosenGPU, &gpuProperties);

  //initialize the memory allocator
  VmaAllocatorCreateInfo allocatorInfo = {};
  allocatorInfo.physicalDevice = chosenGPU;
  allocatorInfo.device = device;
  allocatorInfo.instance = instance;
  vmaCreateAllocator(&allocatorInfo, &vmaAllocator);

  uploadContext.device = device;
  uploadContext.queue = graphicsQueue;
}

void VulkanEngine::initSwapchain() {
  vkb::SwapchainBuilder swapchainBuilder{chosenGPU, device, surface};

  VkSurfaceFormatKHR desiredFormat{};
  desiredFormat.colorSpace = VK_COLOR_SPACE_SRGB_NONLINEAR_KHR;
  desiredFormat.format = VK_FORMAT_B8G8R8A8_UNORM;// VK_FORMAT_B8G8R8A8_SRGB;

  vkb::Swapchain vkbSwapchain = swapchainBuilder
          .set_desired_format(desiredFormat)
          .set_desired_present_mode(PRESENT_MODE)
          .set_desired_extent(windowExtent.width, windowExtent.height)
          .build()
          .value();

  swapchain = vkbSwapchain.swapchain;
  swapchainImages = vkbSwapchain.get_images().value();
  swapchainImageViews = vkbSwapchain.get_image_views().value();
  swapchainImageFormat = vkbSwapchain.image_format;

  // setup depth buffer
  VkExtent3D depthImageExtent = {
          windowExtent.width,
          windowExtent.height,
          1
  };
  depthFormat = VK_FORMAT_D32_SFLOAT;
  VkImageCreateInfo depthImageInfo = vkinit::imageCreateInfo(depthFormat, VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT, depthImageExtent);

  // We want GPU local memory as we do not currently need to access the depth buffer on the CPU side
  VmaAllocationCreateInfo depthImageAllocInfo = {};
  depthImageAllocInfo.usage = VMA_MEMORY_USAGE_GPU_ONLY; // Requst it to be on GPU
  depthImageAllocInfo.requiredFlags = VkMemoryPropertyFlags(VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT); // Require it to be on GPU
  vmaCreateImage(vmaAllocator, &depthImageInfo, &depthImageAllocInfo, &depthImage.vkImage, &depthImage.vmaAllocation, nullptr);

  // build an image-view for the depth image to use in framebuffers for rendering
  VkImageViewCreateInfo depthImageViewInfo = vkinit::imageViewCreateInfo(depthFormat, depthImage.vkImage, VK_IMAGE_ASPECT_DEPTH_BIT);

  VK_CHECK(vkCreateImageView(device, &depthImageViewInfo, nullptr, &depthImageView));
}

void VulkanEngine::initCommands() {
  VkCommandPoolCreateInfo commandPoolInfo = vkinit::commandPoolCreateInfo(graphicsQueueFamily, VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT);

  for(u32 i = 0; i < FRAME_OVERLAP; i++) {
    VK_CHECK(vkCreateCommandPool(device, &commandPoolInfo, nullptr, &frames[i].commandPool));

    VkCommandBufferAllocateInfo cmdAllocInfo = vkinit::commandBufferAllocateInfo(frames[i].commandPool, 1, VK_COMMAND_BUFFER_LEVEL_PRIMARY);

    VK_CHECK(vkAllocateCommandBuffers(device, &cmdAllocInfo, &frames[i].mainCommandBuffer));

    mainDeletionQueue.pushFunction([=]() {
      vkDestroyCommandPool(device, frames[i].commandPool, nullptr);
    });
  }

  // immediate push upload command pool/buffer
  VkCommandPoolCreateInfo uploadCommandPoolInfo = vkinit::commandPoolCreateInfo(graphicsQueueFamily);
  VK_CHECK(vkCreateCommandPool(device, &uploadCommandPoolInfo, nullptr, &uploadContext.commandPool));

  VkCommandBufferAllocateInfo cmdAllocInfo = vkinit::commandBufferAllocateInfo(uploadContext.commandPool, 1);
  VK_CHECK(vkAllocateCommandBuffers(device, &cmdAllocInfo, &uploadContext.commandBuffer));

  mainDeletionQueue.pushFunction([=]() { vkDestroyCommandPool(device, uploadContext.commandPool, nullptr); });

}

void VulkanEngine::initDefaultRenderpass() {
  VkAttachmentDescription colorAttachment = {};
  colorAttachment.flags = 0;
  colorAttachment.format = swapchainImageFormat;
  colorAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
  colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
  colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
  colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
  colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
  colorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
  colorAttachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

  VkAttachmentDescription depthAttachment = {};
  depthAttachment.flags = 0;
  depthAttachment.format = depthFormat;
  depthAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
  depthAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
  depthAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
  depthAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
  depthAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
  depthAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
  depthAttachment.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

  VkAttachmentDescription attachmentDescriptions[] = {colorAttachment, depthAttachment};

  // Renderpasses hold VkAttachmentDescriptions but subpasses hold VkAttachmentReferences to those descriptions,
  // this allows two subpasses to simply reference the same attachment instead of holding the entire description
  VkAttachmentReference colorAttachmentRef = {};
  //attachment number will index into the pAttachments array in the parent renderpass
  colorAttachmentRef.attachment = 0;
  colorAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

  VkAttachmentReference depthAttachmentRef = {};
  depthAttachmentRef.attachment = 1;
  depthAttachmentRef.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

  //we are going to create 1 subpass, which is the minimum you can do
  VkSubpassDescription subpass = {};
  subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
  subpass.colorAttachmentCount = 1;
  subpass.pColorAttachments = &colorAttachmentRef;
  subpass.pDepthStencilAttachment = &depthAttachmentRef;

  VkRenderPassCreateInfo renderPassInfo = {};
  renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
  renderPassInfo.attachmentCount = 2;
  renderPassInfo.pAttachments = attachmentDescriptions;
  renderPassInfo.subpassCount = 1;
  renderPassInfo.pSubpasses = &subpass;

  VK_CHECK(vkCreateRenderPass(device, &renderPassInfo, nullptr, &renderPass));
}

void VulkanEngine::initFramebuffers() {
  //create the framebuffers for the swapchain images. This will connect the render-pass to the images for rendering
  VkFramebufferCreateInfo fbInfo = {};
  fbInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
  fbInfo.pNext = nullptr;
  fbInfo.renderPass = renderPass;
  fbInfo.attachmentCount = 2;
  fbInfo.width = windowExtent.width;
  fbInfo.height = windowExtent.height;
  fbInfo.layers = 1;

  const u64 swapchainImageCount = swapchainImages.size();
  framebuffers = std::vector<VkFramebuffer>(swapchainImageCount);
  for(u32 i = 0; i < swapchainImageCount; i++) {
    VkImageView attachments[] = {swapchainImageViews[i], depthImageView};
    fbInfo.pAttachments = attachments;
    VK_CHECK(vkCreateFramebuffer(device, &fbInfo, nullptr, &framebuffers[i]));
  }
}

void VulkanEngine::initSyncStructures() {
  VkFenceCreateInfo fenceCreateInfo = vkinit::fenceCreateInfo(VK_FENCE_CREATE_SIGNALED_BIT);
  VkSemaphoreCreateInfo semaphoreCreateInfo = vkinit::semaphoreCreateInfo();

  for(u32 i = 0; i < FRAME_OVERLAP; i++) {
    VK_CHECK(vkCreateFence(device, &fenceCreateInfo, nullptr, &frames[i].renderFence));
    VK_CHECK(vkCreateSemaphore(device, &semaphoreCreateInfo, nullptr, &frames[i].presentSemaphore));
    VK_CHECK(vkCreateSemaphore(device, &semaphoreCreateInfo, nullptr, &frames[i].renderSemaphore));
    mainDeletionQueue.pushFunction([=]() {
      vkDestroyFence(device, frames[i].renderFence, nullptr);
      vkDestroySemaphore(device, frames[i].renderSemaphore, nullptr);
      vkDestroySemaphore(device, frames[i].presentSemaphore, nullptr);
    });
  }

  // immediate command buffer submit fence
  VkFenceCreateInfo uploadFenceCreateInfo = vkinit::fenceCreateInfo();
  VK_CHECK(vkCreateFence(device, &uploadFenceCreateInfo, nullptr, &uploadContext.uploadFence));
  mainDeletionQueue.pushFunction([=]() {
    vkDestroyFence(device, uploadContext.uploadFence, nullptr);
  });
}

void VulkanEngine::initDescriptors() {
  // reserve 10 uniform buffer pointers/handles in the descriptor pool
  VkDescriptorPoolSize descriptorPoolSizes[] = {
          {VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,         10},
          {VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, 10},
          {VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,         10},
          {VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 10},
  };

  VkDescriptorPoolCreateInfo descriptorPoolInfo = {};
  descriptorPoolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
  descriptorPoolInfo.flags = 0;
  // holds the maximum number of descriptor sets
  descriptorPoolInfo.maxSets = 10;
  // holds the maximum number of descriptors of each type
  descriptorPoolInfo.poolSizeCount = ArrayCount(descriptorPoolSizes);
  descriptorPoolInfo.pPoolSizes = descriptorPoolSizes;

  vkCreateDescriptorPool(device, &descriptorPoolInfo, nullptr, &descriptorPool);

  // global descriptor set
  VkDescriptorSetLayoutBinding cameraBufferBinding = vkinit::descriptorSetLayoutBinding(
          VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC,
          VK_SHADER_STAGE_VERTEX_BIT,
          0);
  VkDescriptorSetLayoutBinding sceneBinding = vkinit::descriptorSetLayoutBinding(
          VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC,
          VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT,
          1);

  VkDescriptorSetLayoutBinding globalBindings[] = {cameraBufferBinding, sceneBinding};

  // Collection of bindings within our descriptor set
  VkDescriptorSetLayoutCreateInfo descSetCreateInfo = {};
  descSetCreateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
  descSetCreateInfo.pNext = nullptr;
  descSetCreateInfo.flags = 0;
  descSetCreateInfo.bindingCount = ArrayCount(globalBindings);
  descSetCreateInfo.pBindings = globalBindings;

  vkCreateDescriptorSetLayout(device, &descSetCreateInfo, nullptr, &globalDescSetLayout);

  // object descriptor set
  VkDescriptorSetLayoutBinding objectBufferBinding = vkinit::descriptorSetLayoutBinding(
          VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
          VK_SHADER_STAGE_VERTEX_BIT,
          0);

  VkDescriptorSetLayoutBinding objectBindings[] = {objectBufferBinding};

  descSetCreateInfo.bindingCount = ArrayCount(objectBindings);
  descSetCreateInfo.pBindings = objectBindings;

  vkCreateDescriptorSetLayout(device, &descSetCreateInfo, nullptr, &objectDescSetLayout);

  VkDescriptorSetAllocateInfo globalDescSetAllocInfo = {};
  globalDescSetAllocInfo.pNext = nullptr;
  globalDescSetAllocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
  globalDescSetAllocInfo.descriptorPool = descriptorPool;
  globalDescSetAllocInfo.descriptorSetCount = 1;
  globalDescSetAllocInfo.pSetLayouts = &globalDescSetLayout;

  VkDescriptorSetAllocateInfo objectDescSetAllocInfo = {};
  objectDescSetAllocInfo.pNext = nullptr;
  objectDescSetAllocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
  objectDescSetAllocInfo.descriptorPool = descriptorPool;
  objectDescSetAllocInfo.descriptorSetCount = 1;
  objectDescSetAllocInfo.pSetLayouts = &objectDescSetLayout;

  u64 paddedGPUCameraDataSize = vkutil::padUniformBufferSize(gpuProperties, sizeof(GPUCameraData));
  u64 paddedGPUSceneDataSize = vkutil::padUniformBufferSize(gpuProperties, sizeof(GPUSceneData));
  u64 globalBufferSize = FRAME_OVERLAP * (paddedGPUCameraDataSize + paddedGPUSceneDataSize);
  globalBuffer.cameraOffset = 0;
  globalBuffer.sceneOffset = FRAME_OVERLAP * paddedGPUCameraDataSize;
  globalBuffer.buffer = vkutil::createBuffer(vmaAllocator, globalBufferSize, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VMA_MEMORY_USAGE_CPU_TO_GPU, 0);
  vkAllocateDescriptorSets(device, &globalDescSetAllocInfo, &globalDescriptorSet);

  // info about the buffer the descriptor will point at
  VkDescriptorBufferInfo cameraDescBufferInfo;
  cameraDescBufferInfo.buffer = globalBuffer.buffer.vkBuffer;
  cameraDescBufferInfo.offset = globalBuffer.cameraOffset;
  cameraDescBufferInfo.range = sizeof(GPUCameraData);

  VkDescriptorBufferInfo sceneDescBufferInfo;
  sceneDescBufferInfo.buffer = globalBuffer.buffer.vkBuffer;
  sceneDescBufferInfo.offset = globalBuffer.sceneOffset; // NOTE: This is not going to work due to alignment
  sceneDescBufferInfo.range = sizeof(GPUSceneData);

  VkWriteDescriptorSet cameraWrite = vkinit::writeDescriptorBuffer(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, globalDescriptorSet, &cameraDescBufferInfo, 0);
  VkWriteDescriptorSet sceneWrite = vkinit::writeDescriptorBuffer(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, globalDescriptorSet, &sceneDescBufferInfo, 1);
  VkWriteDescriptorSet descSetWrites[] = {cameraWrite, sceneWrite};

  // Note: This function links descriptor sets to the buffers that back their data.
  vkUpdateDescriptorSets(device, ArrayCount(descSetWrites), descSetWrites, 0/*descriptorCopyCount*/, nullptr/*pDescriptorCopies*/);

  const u64 objectBufferSize = sizeof(GPUObjectData) * MAX_OBJECTS;

  for(u32 i = 0; i < FRAME_OVERLAP; i++) {
    FrameData& frame = frames[i];

    frame.objectBuffer = vkutil::createBuffer(vmaAllocator, objectBufferSize, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, VMA_MEMORY_USAGE_CPU_TO_GPU, 0);
    vkAllocateDescriptorSets(device, &objectDescSetAllocInfo, &frame.objectDescriptorSet);

    VkDescriptorBufferInfo objectDescBufferInfo;
    objectDescBufferInfo.buffer = frame.objectBuffer.vkBuffer;
    objectDescBufferInfo.offset = 0;
    objectDescBufferInfo.range = objectBufferSize;

    VkWriteDescriptorSet objectWrite = vkinit::writeDescriptorBuffer(VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, frame.objectDescriptorSet, &objectDescBufferInfo, 0);

    VkWriteDescriptorSet descSetWrites[] = {objectWrite};

    vkUpdateDescriptorSets(device, ArrayCount(descSetWrites), descSetWrites, 0/*descriptorCopyCount*/, nullptr/*pDescriptorCopies*/);
  }

  // single texture descriptor set layout
  VkDescriptorSetLayoutBinding textureBind = vkinit::descriptorSetLayoutBinding(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT, 0);

  descSetCreateInfo.bindingCount = 1;
  descSetCreateInfo.pBindings = &textureBind;

  vkCreateDescriptorSetLayout(device, &descSetCreateInfo, nullptr, &singleTextureSetLayout);

  //allocate the descriptor set for single-texture to use on the material
  singleTexDescSetAllocInfo.pNext = nullptr;
  singleTexDescSetAllocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
  singleTexDescSetAllocInfo.descriptorPool = descriptorPool;
  singleTexDescSetAllocInfo.descriptorSetCount = 1;
  singleTexDescSetAllocInfo.pSetLayouts = &singleTextureSetLayout;

  mainDeletionQueue.pushFunction([=]() {
    // free buffers
    vmaDestroyBuffer(vmaAllocator, globalBuffer.buffer.vkBuffer, globalBuffer.buffer.vmaAllocation);
    for(u32 i = 0; i < FRAME_OVERLAP; i++) {
      FrameData& frame = frames[i];
      vmaDestroyBuffer(vmaAllocator, frame.objectBuffer.vkBuffer, frame.objectBuffer.vmaAllocation);
    }

    // freeing descriptor pool frees descriptor layouts
    vkDestroyDescriptorSetLayout(device, globalDescSetLayout, nullptr);
    vkDestroyDescriptorSetLayout(device, objectDescSetLayout, nullptr);
    vkDestroyDescriptorSetLayout(device, singleTextureSetLayout, nullptr);
    vkDestroyDescriptorPool(device, descriptorPool, nullptr);
  });
}

void VulkanEngine::initPipelines() {
  createFragmentShaderPipeline();

  for(u32 i = 0; i < ArrayCount(materialInfos); i++) {
    createPipeline(materialInfos[i]);
  }
}

void VulkanEngine::createFragmentShaderPipeline() {
  // TODO: merge with every other materials and pipeline creation
  ShaderMetadata shaderMetadata;
  materialManager.loadShaderMetadata(device, SHADER_DIR"hard_coded_fullscreen_quad.vert.spv", SHADER_DIR"fragment_shader_test.frag.spv", shaderMetadata);

  VkPipelineLayoutCreateInfo pipelineLayoutInfo = vkinit::pipelineLayoutCreateInfo();
  pipelineLayoutInfo.pPushConstantRanges = &fragmentShaderPushConstantsRange;
  pipelineLayoutInfo.pushConstantRangeCount = 1;

  VK_CHECK(vkCreatePipelineLayout(device, &pipelineLayoutInfo, nullptr, &fragmentShaderPipelineLayout));

  PipelineBuilder pipelineBuilder;
  pipelineBuilder.shaderStages.push_back(vkinit::pipelineShaderStageCreateInfo(VK_SHADER_STAGE_VERTEX_BIT, shaderMetadata.vertModule));
  pipelineBuilder.shaderStages.push_back(vkinit::pipelineShaderStageCreateInfo(VK_SHADER_STAGE_FRAGMENT_BIT, shaderMetadata.fragModule));
  pipelineBuilder.vertexInputInfo = vkinit::vertexInputStateCreateInfo();
  pipelineBuilder.inputAssembly = vkinit::inputAssemblyCreateInfo(VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST);
  pipelineBuilder.viewport.x = 0.0f;
  pipelineBuilder.viewport.y = 0.0f;
  pipelineBuilder.viewport.width = (float)windowExtent.width;
  pipelineBuilder.viewport.height = (float)windowExtent.height;
  pipelineBuilder.viewport.minDepth = 0.0f;
  pipelineBuilder.viewport.maxDepth = 1.0f;
  pipelineBuilder.scissor.offset = {0, 0};
  pipelineBuilder.scissor.extent = windowExtent;
  pipelineBuilder.rasterizer = vkinit::rasterizationStateCreateInfo(VK_POLYGON_MODE_FILL);
  pipelineBuilder.multisampling = vkinit::multisamplingStateCreateInfo();
  pipelineBuilder.colorBlendAttachment = vkinit::colorBlendAttachmentState();
  pipelineBuilder.depthStencil = vkinit::depthStencilCreateInfo(true, true, VK_COMPARE_OP_LESS_OR_EQUAL);
  pipelineBuilder.pipelineLayout = fragmentShaderPipelineLayout;

  fragmentShaderPipeline = pipelineBuilder.buildPipeline(device, renderPass);
}

void VulkanEngine::createPipeline(MaterialCreateInfo matInfo) {
  ShaderMetadata shaderMetadata;
  materialManager.loadShaderMetadata(device, matInfo.vertFileName, matInfo.fragFileName, shaderMetadata);

  // TODO: Pipeline layout can be reused for several pipelines/materials
  // TODO: Reuse of pipeline layouts will allows for push constants to be used for several pipelines
  // build mesh pipeline layout
  VkPipelineLayoutCreateInfo pipelineLayoutInfo = vkinit::pipelineLayoutCreateInfo();
  pipelineLayoutInfo.pushConstantRangeCount = 0;
  pipelineLayoutInfo.pPushConstantRanges = nullptr;
  VkDescriptorSetLayout descSetLayouts[] = {globalDescSetLayout, objectDescSetLayout, singleTextureSetLayout};
  pipelineLayoutInfo.setLayoutCount = ArrayCount(descSetLayouts);
  pipelineLayoutInfo.pSetLayouts = descSetLayouts;

  VkPipelineLayout pipelineLayout;
  VK_CHECK(vkCreatePipelineLayout(device, &pipelineLayoutInfo, nullptr, &pipelineLayout));

  PipelineBuilder pipelineBuilder;
  pipelineBuilder.shaderStages.push_back(vkinit::pipelineShaderStageCreateInfo(VK_SHADER_STAGE_VERTEX_BIT, shaderMetadata.vertModule));
  pipelineBuilder.shaderStages.push_back(vkinit::pipelineShaderStageCreateInfo(VK_SHADER_STAGE_FRAGMENT_BIT, shaderMetadata.fragModule));
  pipelineBuilder.vertexInputInfo = vkinit::vertexInputStateCreateInfo();
  // input assembly is the configuration for drawing triangle lists, strips, or points
  pipelineBuilder.inputAssembly = vkinit::inputAssemblyCreateInfo(VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST);
  //build viewport and scissor from the swapchain extents
  pipelineBuilder.viewport.x = 0.0f;
  pipelineBuilder.viewport.y = 0.0f;
  pipelineBuilder.viewport.width = (float)windowExtent.width;
  pipelineBuilder.viewport.height = (float)windowExtent.height;
  pipelineBuilder.viewport.minDepth = 0.0f;
  pipelineBuilder.viewport.maxDepth = 1.0f;
  pipelineBuilder.scissor.offset = {0, 0};
  pipelineBuilder.scissor.extent = windowExtent;
  pipelineBuilder.rasterizer = vkinit::rasterizationStateCreateInfo(VK_POLYGON_MODE_FILL);
  pipelineBuilder.multisampling = vkinit::multisamplingStateCreateInfo();
  pipelineBuilder.colorBlendAttachment = vkinit::colorBlendAttachmentState();
  pipelineBuilder.depthStencil = vkinit::depthStencilCreateInfo(true, true, VK_COMPARE_OP_LESS_OR_EQUAL);
  pipelineBuilder.pipelineLayout = pipelineLayout;

  //connect the pipeline builder vertex input info to the one we get from Vertex
  VertexInputDescription vertexDescription = Vertex::getVertexDescriptions();
  pipelineBuilder.vertexInputInfo.pVertexAttributeDescriptions = vertexDescription.attributes.data();
  pipelineBuilder.vertexInputInfo.vertexAttributeDescriptionCount = (u32)vertexDescription.attributes.size();
  pipelineBuilder.vertexInputInfo.pVertexBindingDescriptions = &vertexDescription.bindingDesc;
  pipelineBuilder.vertexInputInfo.vertexBindingDescriptionCount = 1;

  VkPipeline pipeline = pipelineBuilder.buildPipeline(device, renderPass);

  createMaterial(pipeline, pipelineLayout, matInfo.name);
}

void VulkanEngine::initSamplers() {
  VkSamplerCreateInfo blockySamplerCI = vkinit::samplerCreateInfo(VK_FILTER_NEAREST);
  vkCreateSampler(device, &blockySamplerCI, nullptr, &blockySampler);

  VkSamplerCreateInfo linearSamplerCI = vkinit::samplerCreateInfo(VK_FILTER_LINEAR);
  vkCreateSampler(device, &linearSamplerCI, nullptr, &linearSampler);

  mainDeletionQueue.pushFunction([=]() {
    vkDestroySampler(device, blockySampler, nullptr);
    vkDestroySampler(device, linearSampler, nullptr);
  });
}

void VulkanEngine::attachSingleLoadedTexToNewDescSet(VkSampler sampler, const char* loadedTex, VkDescriptorSet* outDescSet) {
  vkAllocateDescriptorSets(device, &singleTexDescSetAllocInfo, outDescSet);

  //write to the descriptor set so that it points to our minecraft_diffuse texture
  VkDescriptorImageInfo imageBufferInfo;
  imageBufferInfo.sampler = blockySampler;
  imageBufferInfo.imageView = loadedTextures[loadedTex].imageView;
  imageBufferInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

  VkWriteDescriptorSet texture = vkinit::writeDescriptorImage(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, *outDescSet, &imageBufferInfo, 0);

  vkUpdateDescriptorSets(device, 1, &texture, 0, nullptr);
}

void VulkanEngine::initScene() {

  // Renderables //
//  // Mr. Saturn
//	RenderObject mrSaturnObject;
//  mrSaturnObject.mesh = getMesh(bakedMeshAssetData.mr_saturn.name);
//  mrSaturnObject.materialName = materialDefaultLit.name;
//  mrSaturnObject.material = getMaterial(mrSaturnObject.materialName);
//	f32 mrSaturnScale = 20.0f;
//	mat4 mrSaturnScaleMat = scale_mat4(vec3{mrSaturnScale, mrSaturnScale, mrSaturnScale});
//	mat4 mrSaturnTranslationMat = translate_mat4(vec3{0.0f, 0.0f, -mrSaturnScale * 2.0f});
//	mat4 mrSaturnTransform = mrSaturnTranslationMat * mrSaturnScaleMat;
//  mrSaturnObject.modelMatrix = mrSaturnTransform;
//  mrSaturnObject.defaultColor = vec4{155.0f / 255.0f, 115.0f / 255.0f, 96.0f / 255.0f, 1.0f};
//  attachTexture(blockySampler, bakedTextureAssetData.single_white_pixel.name, &mrSaturnObject.textureSet);
//	renderables.push_back(mrSaturnObject);
//
//  // Cubes //
//	RenderObject cubeObject;
//  cubeObject.mesh = getMesh(bakedMeshAssetData.cube.name);
//  cubeObject.materialName = materialDefaulColor.name;
//  cubeObject.material = getMaterial(cubeObject.materialName);
//  attachTexture(blockySampler, bakedTextureAssetData.single_white_pixel.name, &cubeObject.textureSet);
//	f32 envScale = 0.2f;
//	mat4 envScaleMat = scale_mat4(vec3{envScale, envScale, envScale});
//	for (s32 x = -16; x <= 16; ++x)
//	for (s32 y = -16; y <= 16; ++y)
//	for (s32 z = 0; z <= 32; ++z) {
//		vec3 pos{ (f32)x, (f32)y, (f32)z };
//		vec3 color = (pos + vec3{16.0f, 16.0f, 0.0f}) / vec3{32.0f, 32.0f, 32.0f};
//		mat4 translationMat = translate_mat4(pos);
//    cubeObject.modelMatrix = translationMat * envScaleMat;
//    cubeObject.defaultColor = vec4{color.r, color.g, color.b, 1.0f}; // TODO: lookup how glm accomplishes vec4{vec3, f32} construction
//		renderables.push_back(cubeObject);
//	}

  // Minecraft World
  RenderObject minecraftObject;
  minecraftObject.mesh = getMesh(bakedMeshAssetData.lost_empire.name);
  minecraftObject.materialName = materialTextured.name;
  minecraftObject.material = getMaterial(minecraftObject.materialName);
  minecraftObject.defaultColor = vec4{50.0f, 0.0f, 0.0f, 1.0f};
  attachSingleLoadedTexToNewDescSet(blockySampler, bakedTextureAssetData.lost_empire_RGBA.name, &minecraftObject.textureSet);

  f32 minecraftScale = 1.0f;
  mat4 minecraftScaleMat = scale_mat4(vec3{minecraftScale, minecraftScale, minecraftScale});
  mat4 minecraftRotationMat = rotate_mat4(RadiansPerDegree * 90.0f, vec3{1.0f, 0.0f, 0.0f});
  mat4 minecraftTranslationMat = translate_mat4(vec3{0.0f, 0.0f, 0.0f});
  mat4 minecraftTransform = minecraftTranslationMat * minecraftRotationMat * minecraftScaleMat;
  minecraftObject.modelMatrix = minecraftTransform;

  renderables.push_back(minecraftObject);

  // TODO: sort objects by material to minimize binding pipelines
  //std::sort(renderables.begin(), renderables.end(), [](const RenderObject& a, const RenderObject& b) {
  //	return a.material->pipeline > b.material->pipeline;
  //});
}

void VulkanEngine::loadMeshes() {
  // Note: Currently just loading all meshes, not sustainable in long run
  BakedAssetData* bakedMeshes = (BakedAssetData*)(&bakedMeshAssetData);
  u32 meshCount = bakedMeshAssetCount();
  for(u32 i = 0; i < meshCount; i++) {
    const BakedAssetData& bakedMeshData = bakedMeshes[i];

    Mesh mesh{};
    mesh.loadFromAsset(bakedMeshData.filePath);
    mesh.uploadMesh(vmaAllocator, uploadContext);
    meshes[bakedMeshData.name] = mesh;
  }

  mainDeletionQueue.pushFunction([=]() {
    u32 meshCount = bakedMeshAssetCount();
    for(u32 i = 0; i < meshCount; i++) {
      const BakedAssetData& bakedMeshData = bakedMeshes[i];
      const Mesh& mesh = meshes[bakedMeshData.name];
      vmaDestroyBuffer(vmaAllocator, mesh.vertexBuffer.vkBuffer, mesh.vertexBuffer.vmaAllocation);
      vmaDestroyBuffer(vmaAllocator, mesh.indexBuffer.vkBuffer, mesh.indexBuffer.vmaAllocation);
      meshes.erase(bakedMeshData.name);
    }
  });
}

void VulkanEngine::cleanupSwapChain() {
  // NOTE: Pipelines depend on swap chain due to its dependencies on the window extent and the renderpass
  vkDestroyPipeline(device, fragmentShaderPipeline, nullptr);
  vkDestroyPipelineLayout(device, fragmentShaderPipelineLayout, nullptr);
  for(std::pair<std::string, Material> element: materials) {
    Material& mat = element.second;
    vkDestroyPipeline(device, mat.pipeline, nullptr);
    vkDestroyPipelineLayout(device, mat.pipelineLayout, nullptr);
  }
  materials.clear();

  // NOTE: Framebuffers depend on swap chain due to it holding attachments for the swap chain and depth image views
  u64 swapchainImageCount = swapchainImageViews.size();
  for(u32 i = 0; i < swapchainImageCount; i++) {
    vkDestroyFramebuffer(device, framebuffers[i], nullptr);
  }
  // NOTE: Renderpass depends on the swap chain due to the swap chain's image format
  vkDestroyRenderPass(device, renderPass, nullptr);

  // NOTE: Depth image view depends on the swap chain due to it's dependency on the window extent
  vkDestroyImageView(device, depthImageView, nullptr);
  vmaDestroyImage(vmaAllocator, depthImage.vkImage, depthImage.vmaAllocation);

  for(u32 i = 0; i < swapchainImageCount; i++) {
    vkDestroyImageView(device, swapchainImageViews[i], nullptr);
  }
  swapchainImageViews.clear();
  vkDestroySwapchainKHR(device, swapchain, nullptr);
}

void VulkanEngine::recreateSwapChain() {
  WindowManager::getInstance().getExtent(&windowExtent.width, &windowExtent.height);

  vkDeviceWaitIdle(device);

  cleanupSwapChain();

  initSwapchain();
  initDefaultRenderpass();
  initFramebuffers();
  initPipelines();

  for(RenderObject& object: renderables) {
    object.material = getMaterial(object.materialName);
  }
}

Material* VulkanEngine::createMaterial(VkPipeline pipeline, VkPipelineLayout layout, const char* name) {
  Material material;
  material.pipeline = pipeline;
  material.pipelineLayout = layout;
  materials[name] = material;
  return &materials[name];
}

Material* VulkanEngine::getMaterial(const char* name) {
  //search for the object, and return nullptr if not found
  auto it = materials.find(name);
  if(it == materials.end()) {
    return nullptr;
  } else {
    return &(*it).second;
  }
}

Mesh* VulkanEngine::getMesh(const std::string& name) {
  auto it = meshes.find(name);
  if(it == meshes.end()) {
    return nullptr;
  } else {
    return &(*it).second;
  }
}

void VulkanEngine::drawFragmentShader(VkCommandBuffer cmd) {
  vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, fragmentShaderPipeline);
  // vkCmdBindDescriptorSets() not needed as descriptor count set is 0

  VkViewport viewport{};
  viewport.x = 0;
  viewport.y = (f32)windowExtent.height;
  viewport.width = (f32)windowExtent.width;
  viewport.height = -viewport.y;
  viewport.minDepth = 0.0f;
  viewport.maxDepth = 1.0f;

  vkCmdSetViewport(cmd, 0, 1, &viewport);

  FragmentShaderPushConstants pushConstants;
  pushConstants.time = (frameNumber / 60.0f);
  pushConstants.resolutionX = (f32)windowExtent.width;
  pushConstants.resolutionY = (f32)windowExtent.height;
  vkCmdPushConstants(cmd, fragmentShaderPipelineLayout, VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(FragmentShaderPushConstants), &pushConstants);

  vkCmdDraw(cmd, 6, 1, 0, 0);
}

void VulkanEngine::drawObjects(VkCommandBuffer cmd, const Camera& camera, RenderObject* firstObject, u32 objectCount) {
  FrameData& frame = getCurrentFrame();

  // camera data
  GPUCameraData cameraData;
  cameraData.view = camera.getViewMatrix();
  cameraData.projection = perspective(radians(70.f), 1700.f / 900.f, 0.1f, 200.0f);
  cameraData.viewproj = cameraData.projection * cameraData.view;
  // scene data
  GPUSceneData sceneData;
  float magicNum = (frameNumber / 120.f);
  sceneData.ambientColor = {sin(magicNum), 0, cos(magicNum), 1};

  int frameIndex = frameNumber % FRAME_OVERLAP;

  // copy data to scene buffer
  u64 sceneDataOffset = (u32)vkutil::padUniformBufferSize(gpuProperties, sizeof(GPUSceneData)) * frameIndex;
  u64 cameraDataOffset = (u32)vkutil::padUniformBufferSize(gpuProperties, sizeof(GPUCameraData)) * frameIndex;
  char* globalDataBufferPtr;
  vmaMapMemory(vmaAllocator, globalBuffer.buffer.vmaAllocation, (void**)&globalDataBufferPtr);
  {
    // copy camera data
    char* cameraPtr = (globalDataBufferPtr + globalBuffer.cameraOffset);
    cameraPtr += cameraDataOffset;
    memcpy(cameraPtr, &cameraData, sizeof(GPUCameraData));

    // copy scene data
    char* scenePtr = (globalDataBufferPtr + globalBuffer.sceneOffset);
    scenePtr += sceneDataOffset;
    memcpy(scenePtr, &sceneData, sizeof(GPUSceneData));
    globalDataBufferPtr = nullptr;
  }
  vmaUnmapMemory(vmaAllocator, globalBuffer.buffer.vmaAllocation);

  // copy data to object buffer
  local_access Timer ssboUploadTimer;
  StartTimer(ssboUploadTimer);
  GPUObjectData* objectData;
  vmaMapMemory(vmaAllocator, frame.objectBuffer.vmaAllocation, (void**)&objectData);
  // Note: if my data was organized differently, possibly SoA or AoSoA instead of simply AoS, I could use memcpy for large chunks of data instead of iterating through a loop
  for(u32 i = 0; i < objectCount; i++) {
    RenderObject& object = firstObject[i];
    objectData[i].modelMatrix = object.modelMatrix;
    objectData[i].defaultColor = object.defaultColor;
  }
  objectData = nullptr;
  vmaUnmapMemory(vmaAllocator, frame.objectBuffer.vmaAllocation);
  f64 ssboUploadTimeMs = StopTimer(ssboUploadTimer);
  vkImguiQuickDebugText(renderDebugInfoRequestedForFrame, "Uploading SSBO data: %5.5f ms", ssboUploadTimeMs);

  VkViewport viewport{};
  viewport.x = 0;
  viewport.y = (f32)windowExtent.height;
  viewport.width = (f32)windowExtent.width;
  viewport.height = -viewport.y;
  viewport.minDepth = 0.0f;
  viewport.maxDepth = 1.0f;

  vkCmdSetViewport(cmd, 0, 1, &viewport);

  local_access Timer objectCmdBufferFillTimer;
  StartTimer(objectCmdBufferFillTimer);
  Mesh* lastMesh = nullptr;
  Material* lastMaterial = nullptr;
  for(u32 i = 0; i < objectCount; i++) {
    RenderObject& object = firstObject[i];

    //only bind the pipeline if it doesn't match with the already bound one
    Assert(object.material != nullptr)
    if(object.material != lastMaterial) {
      vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, object.material->pipeline);
      lastMaterial = object.material;

      // Note: It is only necessary to rebind descriptor sets if the desciptor layouts change between pipelines
      // Or if the dynamic uniform buffer offset needs to be updated
      u32 dynamicUniformOffsets[] = {(u32)cameraDataOffset, (u32)sceneDataOffset};

      if(object.textureSet == VK_NULL_HANDLE) {
        VkDescriptorSet descSets[] = {globalDescriptorSet, frame.objectDescriptorSet};
        // vkCmdBindDescriptorSets causes the sets numbered [firstSet, firstSet+descriptorSetCount-1] to use the binding information stored in pDescriptorSets[0..descriptorSetCount-1]
        // dynamic uniform offsets must be provided for every dynamic uniform buffer descriptor in the descriptor set(s)
        // the dynamic offsets are ordered based firstly on the order of the descriptor set array and then on their binding index within that descriptor set
        vkCmdBindDescriptorSets(cmd,
                                VK_PIPELINE_BIND_POINT_GRAPHICS,
                                object.material->pipelineLayout,
                                0 /*first set*/,
                                ArrayCount(descSets),
                                descSets,
                                ArrayCount(dynamicUniformOffsets),
                                dynamicUniformOffsets);
      } else {
        VkDescriptorSet descSetsWithTex[] = {globalDescriptorSet, frame.objectDescriptorSet, object.textureSet};
        vkCmdBindDescriptorSets(cmd,
                                VK_PIPELINE_BIND_POINT_GRAPHICS,
                                object.material->pipelineLayout,
                                0 /*first set*/,
                                ArrayCount(descSetsWithTex),
                                descSetsWithTex,
                                ArrayCount(dynamicUniformOffsets),
                                dynamicUniformOffsets);
      }
    }

    //only bind the mesh if it's a different one from last bind
    if(object.mesh != lastMesh) {
      //bind the mesh vertex buffer with offset 0
      VkDeviceSize offset = 0;
      vkCmdBindVertexBuffers(cmd, 0, 1, &object.mesh->vertexBuffer.vkBuffer, &offset);
      vkCmdBindIndexBuffer(cmd, object.mesh->indexBuffer.vkBuffer, 0, VK_INDEX_TYPE_UINT32);
      lastMesh = object.mesh;
    }

    // if material AND mesh are the same, use instancing where objects will be differentiated by the SSBO object data using the instance index in the shader
    u32 drawCount = 1;
    u32 nextIndex = i + 1;
    while(nextIndex < objectCount &&
          firstObject[nextIndex].material == object.material &&
          firstObject[nextIndex].mesh == object.mesh) {
      nextIndex++;
      drawCount++;
    }

    vkCmdDrawIndexed(cmd, (u32)object.mesh->indices.size(), drawCount, 0, 0, i);
    i += drawCount - 1;
  }

  f64 objectCmdBufferFillMs = StopTimer(objectCmdBufferFillTimer);
  vkImguiQuickDebugText(renderDebugInfoRequestedForFrame, "Filling command buffer for object draws: %5.5f ms", objectCmdBufferFillMs);
}

void VulkanEngine::loadImages() {
  // Note: Currently just loading all textures, not sustainable in long run
  BakedAssetData* bakedTextures = (BakedAssetData*)(&bakedTextureAssetData);
  u32 textureCount = bakedTextureAssetCount();
  std::vector<AllocatedImage> allocatedImageTextures;
  allocatedImageTextures.resize(textureCount);
  std::vector<const char*> filePaths;
  filePaths.resize(textureCount);

  for(u32 i = 0; i < textureCount; i++) {
    filePaths[i] = bakedTextures[i].filePath;
  }

  vkutil::loadImagesFromAssetFiles(vmaAllocator, uploadContext, filePaths.data(), allocatedImageTextures.data(), textureCount);

  for(u32 i = 0; i < textureCount; i++) {
    const BakedAssetData& bakedTextureData = bakedTextures[i];

    Texture tex{};
    tex.image = allocatedImageTextures[i];
    VkImageViewCreateInfo imageCreateInfo = vkinit::imageViewCreateInfo(tex.image.vkFormat, tex.image.vkImage, VK_IMAGE_ASPECT_COLOR_BIT);
    vkCreateImageView(device, &imageCreateInfo, nullptr, &tex.imageView);

    loadedTextures[bakedTextureData.name] = tex;
  }

  mainDeletionQueue.pushFunction([=]() {
    for(u32 i = 0; i < textureCount; i++) {
      const BakedAssetData& bakedTextureData = bakedTextures[i];
      Texture texture = loadedTextures[bakedTextureData.name];
      vkDestroyImageView(device, texture.imageView, nullptr);
      vmaDestroyImage(vmaAllocator, texture.image.vkImage, texture.image.vmaAllocation);
      loadedTextures.erase(bakedTextureData.name);
    }
  });
}

void VulkanEngine::initImgui() {
  VkImguiCreateInfo imguiCI;
  imguiCI.instance = instance;
  imguiCI.device = device;
  imguiCI.chosenGPU = chosenGPU;
  imguiCI.fontTextUploadCommandBuffer = frames[0].mainCommandBuffer;
  imguiCI.graphicsQueue = graphicsQueue;
  imguiCI.graphicsQueueFamily = graphicsQueueFamily;
  imguiCI.renderPass = renderPass;
  imguiCI.swapChainImageCount = (u32)swapchainImageViews.size();

  vkImguiCreateInstance(&imguiCI, &imguiInstance);
}
