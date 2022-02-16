
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
  // needed for SDL to counteract the scaling Windows can do for windows
  SetProcessDPIAware();

  initSDL();

  initCamera();

  initVulkan();
  initSwapchain();
  initCommands();
  initDefaultRenderpass();
  initFramebuffers();
  initSyncStructures();
  initDescriptors();
  initPipelines();
  loadImages();
  loadMeshes();
  initScene();

  initImgui();

  isInitialized = true;
}

void VulkanEngine::cleanup() {
  VK_CHECK(vkDeviceWaitIdle(device));

  if(isInitialized) {
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
    SDL_DestroyWindow(window);
  }
}

void VulkanEngine::draw() {
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
    drawObjects(cmd, renderables.data(), (u32)renderables.size());
    renderImgui(cmd);

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

void VulkanEngine::processInput() {
  local_access bool altDown = false;
  bool altWasDown = altDown;

  SDL_Event e;
  while (SDL_PollEvent(&e) != 0)
  {
    switch (e.type) {
    case SDL_QUIT: input.quit = true; break;
    case SDL_KEYDOWN:
      switch (e.key.keysym.sym)
      {
        case SDLK_ESCAPE: input.quit = true; break;
        case SDLK_SPACE: break;
        case SDLK_LSHIFT: input.button1 = true; break;
        case SDLK_a: case SDLK_LEFT: input.left = true; break;
        case SDLK_d: case SDLK_RIGHT: input.right = true; break;
        case SDLK_w: case SDLK_UP: input.forward = true; break;
        case SDLK_s: case SDLK_DOWN: input.back = true; break;
        case SDLK_q: input.down = true; break;
        case SDLK_e: input.up = true; break;
        case SDLK_LALT: case SDLK_RALT: altDown = true; break;
      }
      break;
    case SDL_KEYUP:
      switch (e.key.keysym.sym)
      {
        case SDLK_SPACE: input.switch1 = !input.switch1; break;
        case SDLK_a: case SDLK_LEFT: input.left = false; break;
        case SDLK_d: case SDLK_RIGHT: input.right = false; break;
        case SDLK_w: case SDLK_UP: input.forward = false; break;
        case SDLK_s: case SDLK_DOWN: input.back = false; break;
        case SDLK_q: input.down = false; break;
        case SDLK_e: input.up = false;break;
        case SDLK_LALT: case SDLK_RALT: altDown = false; break;
        case SDLK_RETURN: if (altWasDown) { input.fullscreen = !input.fullscreen; } break;
        case SDLK_TAB: imguiState.showMainMenu = !imguiState.showMainMenu; break;
      }
      break;
    case SDL_MOUSEMOTION:
    {
      input.mouseDeltaX += (f32)e.motion.xrel / windowExtent.width;
      input.mouseDeltaY += (f32)e.motion.yrel / windowExtent.height;
      break;
    }
    default: break;
    }

    // also send input to ImGui
    ImGui_ImplSDL2_ProcessEvent(&e);
  }
}

void VulkanEngine::quickDebugText(const char* fmt, ...) const {
  if(imguiState.showGeneralDebug) {
    va_list args;
    va_start(args, fmt);
    ImGui::TextV(fmt, args);
    va_end(args);
  }
}

void VulkanEngine::quickDebugFloat(const char* label, float* v, float v_min, float v_max, const char* format, ImGuiSliderFlags flags) const {
  if(imguiState.showGeneralDebug) {
    ImGui::SliderFloat(label, v, v_min, v_max, format, flags);
  }
}

void VulkanEngine::update() {
  local_access f32 moveSpeed = 0.2f;
  local_access f32 turnSpeed = 0.5f;
  local_access bool editMode = true; // NOTE: We assume edit mode is enabled by default
  local_access bool fullscreened = false; // NOTE: We assume windowed by default
  vec3 cameraDelta = {};
  f32 yawDelta = 0.0f;
  f32 pitchDelta = 0.0f;

  if(fullscreened != input.fullscreen) {
    fullscreened = !fullscreened;

    if(fullscreened) {
      SDL_SetWindowFullscreen(window, SDL_WINDOW_FULLSCREEN_DESKTOP);
      SDL_GLContext sdlContext = SDL_GL_CreateContext(window);
      SDL_GL_MakeCurrent(window, sdlContext);
    } else {
      SDL_SetWindowFullscreen(window, 0);
      SDL_GLContext sdlContext = SDL_GL_CreateContext(window);
      SDL_GL_MakeCurrent(window, sdlContext);
    }
  }

  if(editMode != input.switch1) {
    editMode = input.switch1;
    SDL_SetRelativeMouseMode(editMode ? SDL_FALSE : SDL_TRUE);
  }

  // Used to keep camera at constant speed regardless of direction
  const vec4 invSqrt = {
          0.0f,
          1.0f,
          0.7071067f,
          0.5773502f
  };
  u32 invSqrtIndex = 0;

  if(input.left ^ input.right) {
    invSqrtIndex++;
    if(input.left) {
      cameraDelta.x = -1.0f;
    } else {
      cameraDelta.x = 1.0f;
    }
  }

  if(input.back ^ input.forward) {
    invSqrtIndex++;
    if(input.back) {
      cameraDelta.y = -1.0f;
    } else {
      cameraDelta.y = 1.0f;
    }
  }

  if(input.down ^ input.up) {
    invSqrtIndex++;
    if(input.down) {
      cameraDelta.z = -1.0f;
    } else {
      cameraDelta.z = 1.0f;
    }
  }

  if(!editMode && (input.mouseDeltaX != 0.0f || input.mouseDeltaY != 0.0f)) {
    f32 turnSpd = turnSpeed * 10.0f;
    camera.turn(input.mouseDeltaX * turnSpd, input.mouseDeltaY * turnSpd);
  }

  // reset some input variables
  {
    input.mouseDeltaX = 0.0f;
    input.mouseDeltaY = 0.0f;
  }

  quickDebugFloat("move speed", &moveSpeed, 0.1f, 1.0f, "%.2f");

  camera.move(cameraDelta * (invSqrt.val[invSqrtIndex] * moveSpeed));
}

FrameData& VulkanEngine::getCurrentFrame() {
  return frames[frameNumber % FRAME_OVERLAP];
}

void VulkanEngine::run() {
  while(!input.quit) {
    processInput();
    startImguiFrame();
    update();
    draw();
  }
}

void VulkanEngine::initSDL() {
  SDL_Init(SDL_INIT_VIDEO);
  SDL_WindowFlags window_flags = (SDL_WindowFlags)(SDL_WINDOW_VULKAN | SDL_WINDOW_RESIZABLE | SDL_WINDOW_ALLOW_HIGHDPI);
  window = SDL_CreateWindow(
          "Vulkan Engine",
          SDL_WINDOWPOS_UNDEFINED,
          SDL_WINDOWPOS_UNDEFINED,
          windowExtent.width,
          windowExtent.height,
          window_flags
  );
  SDL_CaptureMouse(SDL_TRUE);
  SDL_SetHintWithPriority(SDL_HINT_MOUSE_RELATIVE_MODE_WARP, "1", SDL_HINT_OVERRIDE);
}

void VulkanEngine::initImgui() {
  IMGUI_CHECKVERSION();
  ImGui::CreateContext();
  ImGuiIO& io = ImGui::GetIO();
  (void)io;
  //io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;     // Enable Keyboard Controls
  //io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;      // Enable Gamepad Controls

  io.FontGlobalScale = 2.0f;

  // Setup Dear ImGui style
  ImGui::StyleColorsDark();
  //ImGui::StyleColorsClassic();

  ImGui_ImplVulkanH_Window imguiWindow{};
  imguiWindow.Width = windowExtent.width;
  imguiWindow.Height = windowExtent.height;
  imguiWindow.Swapchain = swapchain;
  imguiWindow.Surface = surface;
  imguiWindow.SurfaceFormat.format = swapchainImageFormat;
  imguiWindow.SurfaceFormat.colorSpace = VK_COLOR_SPACE_SRGB_NONLINEAR_KHR;
  imguiWindow.PresentMode = PRESENT_MODE;
  imguiWindow.RenderPass = renderPass;
  imguiWindow.ClearEnable = true;
  imguiWindow.ClearValue = colorClearValue;
  imguiWindow.FrameIndex = 0;
  imguiWindow.ImageCount = (u32)swapchainImageViews.size();
  imguiWindow.SemaphoreIndex = 0;
  imguiWindow.Frames = nullptr;
  imguiWindow.FrameSemaphores = nullptr;

  // Create Descriptor Pool
  VkDescriptorPool imguiDescriptorPool;
  {
    VkDescriptorPoolSize poolSizes[] =
            {
                    {VK_DESCRIPTOR_TYPE_SAMPLER,                1000},
                    {VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1000},
                    {VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE,          1000},
                    {VK_DESCRIPTOR_TYPE_STORAGE_IMAGE,          1000},
                    {VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER,   1000},
                    {VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER,   1000},
                    {VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,         1000},
                    {VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,         1000},
                    {VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, 1000},
                    {VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC, 1000},
                    {VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT,       1000}
            };
    VkDescriptorPoolCreateInfo poolInfo = {};
    poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    poolInfo.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;
    poolInfo.maxSets = 1000 * IM_ARRAYSIZE(poolSizes);
    poolInfo.poolSizeCount = (uint32_t)IM_ARRAYSIZE(poolSizes);
    poolInfo.pPoolSizes = poolSizes;
    VK_CHECK(vkCreateDescriptorPool(device, &poolInfo, nullptr, &imguiDescriptorPool));
  }

  ImGui_ImplSDL2_InitForVulkan(window);
  ImGui_ImplVulkan_InitInfo initInfo = {};
  initInfo.Instance = instance;
  initInfo.PhysicalDevice = chosenGPU;
  initInfo.Device = device;
  initInfo.QueueFamily = graphicsQueueFamily;
  initInfo.Queue = graphicsQueue;
  initInfo.PipelineCache = VK_NULL_HANDLE;
  initInfo.DescriptorPool = imguiDescriptorPool;
  initInfo.MinImageCount = 3;
  initInfo.ImageCount = (u32)swapchainImageViews.size();
  initInfo.MSAASamples = VK_SAMPLE_COUNT_1_BIT;
  ImGui_ImplVulkan_Init(&initInfo, imguiWindow.RenderPass);

  // Upload Fonts
  {
    // Use any command queue
    VK_CHECK(vkResetCommandPool(device, frames[0].commandPool, 0));
    VkCommandBufferBeginInfo beginInfo = {};
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    beginInfo.flags |= VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
    VK_CHECK(vkBeginCommandBuffer(frames[0].mainCommandBuffer, &beginInfo));

    ImGui_ImplVulkan_CreateFontsTexture(frames[0].mainCommandBuffer);

    VkSubmitInfo endInfo = {};
    endInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    endInfo.commandBufferCount = 1;
    endInfo.pCommandBuffers = &frames[0].mainCommandBuffer;
    VK_CHECK(vkEndCommandBuffer(frames[0].mainCommandBuffer));
    VK_CHECK(vkQueueSubmit(graphicsQueue, 1, &endInfo, VK_NULL_HANDLE));

    VK_CHECK(vkDeviceWaitIdle(device));
    ImGui_ImplVulkan_DestroyFontUploadObjects();
  }

  imguiState = {};
  imguiState.showMainMenu = true;
  imguiState.showFPS = true;
  imguiState.stringRingBuffer = createCStringRingBuffer(50, 256);

  mainDeletionQueue.pushFunction([=]() {
    vkDestroyDescriptorPool(device, imguiDescriptorPool, nullptr);
    ImGui_ImplVulkan_Shutdown();
    deleteCStringRingBuffer(imguiState.stringRingBuffer);
  });
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

  SDL_Vulkan_CreateSurface(window, instance, &surface);

  VkPhysicalDeviceFeatures physicalDeviceFeatures{};
  physicalDeviceFeatures.fillModeNonSolid = VK_TRUE;

  vkb::PhysicalDeviceSelector selector{vkbInst};
  vkb::PhysicalDevice physicalDevice = selector
          .set_minimum_version(1, 1)
          .set_surface(surface)
          .require_present()
          .set_required_features(physicalDeviceFeatures) // TODO: probably won't need this long term
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
  globalBuffer.buffer = vkutil::createBuffer(vmaAllocator, globalBufferSize, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VMA_MEMORY_USAGE_CPU_TO_GPU);
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

    frame.objectBuffer = vkutil::createBuffer(vmaAllocator, objectBufferSize, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, VMA_MEMORY_USAGE_CPU_TO_GPU);
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
  VkDescriptorSetLayout descSets[] = {globalDescSetLayout, objectDescSetLayout, singleTextureSetLayout};
  pipelineLayoutInfo.setLayoutCount = ArrayCount(descSets);
  pipelineLayoutInfo.pSetLayouts = descSets;

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

void VulkanEngine::initScene() {

  // Samplers //
  VkSamplerCreateInfo samplerInfo = vkinit::samplerCreateInfo(VK_FILTER_NEAREST);

  VkSampler blockySampler;
  vkCreateSampler(device, &samplerInfo, nullptr, &blockySampler);

  //allocate the descriptor set for single-texture to use on the material
  VkDescriptorSetAllocateInfo allocInfo = {};
  allocInfo.pNext = nullptr;
  allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
  allocInfo.descriptorPool = descriptorPool;
  allocInfo.descriptorSetCount = 1;
  allocInfo.pSetLayouts = &singleTextureSetLayout;

  auto attachTexture = [&](VkSampler sampler, const char* loadedTexture, VkDescriptorSet* textureSet) -> void {
    vkAllocateDescriptorSets(device, &allocInfo, textureSet);

    //write to the descriptor set so that it points to our minecraft_diffuse texture
    VkDescriptorImageInfo imageBufferInfo;
    imageBufferInfo.sampler = blockySampler;
    imageBufferInfo.imageView = loadedTextures[loadedTexture].imageView;
    imageBufferInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

    VkWriteDescriptorSet texture = vkinit::writeDescriptorImage(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, *textureSet, &imageBufferInfo, 0);

    vkUpdateDescriptorSets(device, 1, &texture, 0, nullptr);
  };

  // Renderables //
  // Mr. Saturn
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
  attachTexture(blockySampler, bakedTextureAssetData.lost_empire_RGBA.name, &minecraftObject.textureSet);

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

  mainDeletionQueue.pushFunction([=]() {
    vkDestroySampler(device, blockySampler, nullptr);
  });
}

void VulkanEngine::initCamera() {
  camera = {};
  camera.pos = {0.f, -50.f, 20.f};
  camera.setForward({0.f, 1.f, 0.f});
}

void VulkanEngine::loadMeshes() {
  // Note: Currently just loading all meshes, not sustainable in long run
  BakedAssetData* bakedMeshes = (BakedAssetData*)(&bakedMeshAssetData);
  u32 meshCount = bakedMeshAssetCount();
  for(u32 i = 0; i < meshCount; i++) {
    const BakedAssetData& bakedMeshData = bakedMeshes[i];

    Mesh mesh{};
    mesh.loadFromAsset(bakedMeshData.filePath);
    mesh.uploadMesh(vmaAllocator);
    meshes[bakedMeshData.name] = mesh;
  }

  mainDeletionQueue.pushFunction([=]() {
    u32 meshCount = bakedMeshAssetCount();
    for(u32 i = 0; i < meshCount; i++) {
      const BakedAssetData& bakedMeshData = bakedMeshes[i];
      const Mesh& mesh = meshes[bakedMeshData.name];
      vmaDestroyBuffer(vmaAllocator, mesh.vertexBuffer.vkBuffer, mesh.vertexBuffer.vmaAllocation);
      meshes.erase(bakedMeshData.name);
    }
  });
}

void VulkanEngine::uploadMesh(Mesh& mesh) {
  const u64 bufferSize = mesh.vertices.size() * sizeof(Vertex);

  VkBufferCreateInfo stagingBufferInfo = {};
  stagingBufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
  stagingBufferInfo.pNext = nullptr;
  stagingBufferInfo.size = bufferSize;
  stagingBufferInfo.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT; // tells Vulkan this buffer is only used as a source for transfer commands

  VmaAllocationCreateInfo vmaStagingAllocInfo = {};
  vmaStagingAllocInfo.usage = VMA_MEMORY_USAGE_CPU_ONLY;

  AllocatedBuffer stagingBuffer; // tmp buffer
  VK_CHECK(vmaCreateBuffer(vmaAllocator, &stagingBufferInfo, &vmaStagingAllocInfo,
                           &stagingBuffer.vkBuffer,
                           &stagingBuffer.vmaAllocation,
                           nullptr));

  void* data;
  vmaMapMemory(vmaAllocator, stagingBuffer.vmaAllocation, &data);
  memcpy(data, mesh.vertices.data(), bufferSize);
  vmaUnmapMemory(vmaAllocator, mesh.vertexBuffer.vmaAllocation);

  VkBufferCreateInfo vertexBufferInfo = stagingBufferInfo;
  vertexBufferInfo.usage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT;

  VmaAllocationCreateInfo vmaVertexAllocInfo = {};
  vmaVertexAllocInfo.usage = VMA_MEMORY_USAGE_GPU_ONLY;

  VK_CHECK(vmaCreateBuffer(vmaAllocator, &vertexBufferInfo, &vmaVertexAllocInfo,
                           &mesh.vertexBuffer.vkBuffer,
                           &mesh.vertexBuffer.vmaAllocation,
                           nullptr));

  vkutil::immediateSubmit(uploadContext, [=](VkCommandBuffer cmd) -> void {
    VkBufferCopy copy;
    copy.dstOffset = 0;
    copy.srcOffset = 0;
    copy.size = bufferSize;
    vkCmdCopyBuffer(cmd, stagingBuffer.vkBuffer, mesh.vertexBuffer.vkBuffer, 1, &copy);
  });

  vmaDestroyBuffer(vmaAllocator, stagingBuffer.vkBuffer, stagingBuffer.vmaAllocation);

  mainDeletionQueue.pushFunction([=]() {
    vmaDestroyBuffer(vmaAllocator, mesh.vertexBuffer.vkBuffer, mesh.vertexBuffer.vmaAllocation);
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
  s32 w, h;
  SDL_GetWindowSize(window, &w, &h);
  while(w == 0 || h == 0) {
    // TODO: Does this actually work?
    SDL_WaitEvent(NULL);
    SDL_GetWindowSize(window, &w, &h);
  }
  windowExtent.width = w;
  windowExtent.height = h;

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

void VulkanEngine::drawObjects(VkCommandBuffer cmd, RenderObject* firstObject, u32 objectCount) {
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

  // copy data to scene buffer
  char* globalDataBufferPtr;
  vmaMapMemory(vmaAllocator, globalBuffer.buffer.vmaAllocation, (void**)&globalDataBufferPtr);
  int frameIndex = frameNumber % FRAME_OVERLAP;
  // copy camera data
  char* cameraPtr = (globalDataBufferPtr + globalBuffer.cameraOffset);
  u64 cameraDataOffset = (u32)vkutil::padUniformBufferSize(gpuProperties, sizeof(GPUCameraData)) * frameIndex;
  cameraPtr += cameraDataOffset;
  memcpy(cameraPtr, &cameraData, sizeof(GPUCameraData));
  // copy scene data
  char* scenePtr = (globalDataBufferPtr + globalBuffer.sceneOffset);
  u64 sceneDataOffset = (u32)vkutil::padUniformBufferSize(gpuProperties, sizeof(GPUSceneData)) * frameIndex;
  scenePtr += sceneDataOffset;
  memcpy(scenePtr, &sceneData, sizeof(GPUSceneData));
  globalDataBufferPtr = nullptr, cameraPtr = nullptr, scenePtr = nullptr;
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
  quickDebugText("Uploading SSBO data: %5.5f ms", ssboUploadTimeMs);

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

    vkCmdDraw(cmd, (u32)object.mesh->vertices.size(), drawCount, 0, i);
    i += drawCount - 1;
  }

  f64 objectCmdBufferFillMs = StopTimer(objectCmdBufferFillTimer);
  quickDebugText("Filling command buffer for object draws: %5.5f ms", objectCmdBufferFillMs);
}

void VulkanEngine::startImguiFrame() {
  ImGui_ImplVulkan_NewFrame();
  ImGui_ImplSDL2_NewFrame(window);
  ImGui::NewFrame();

  if(imguiState.showMainMenu) {
    if(ImGui::BeginMainMenuBar()) {
      if(ImGui::BeginMenu("Windows")) {
        ImGui::MenuItem("Quick Debug Log", NULL, &imguiState.showGeneralDebug);
        ImGui::MenuItem("Main Debug Menu", NULL, &imguiState.showMainMenu);
        ImGui::MenuItem("FPS", NULL, &imguiState.showFPS);
        ImGui::EndMenu();
      }ImGui::EndMainMenuBar();
    }
  }

  imguiTextWindow("General Debug", imguiState.stringRingBuffer, imguiState.showGeneralDebug);

  local_access Timer frameTimer;
  f64 frameTimeMs = StopTimer(frameTimer); // for last frame
  f64 frameFPS = 1000.0 / frameTimeMs;
  if(imguiState.showFPS) {
    const ImGuiWindowFlags textNoFrills = ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoBackground | ImGuiWindowFlags_NoResize;
    if(ImGui::Begin("FPS", &imguiState.showFPS, textNoFrills)) {
      ImGui::Text("%5.2f ms | %3.1f fps", frameTimeMs, frameFPS);
    }ImGui::End();
  }
  StartTimer(frameTimer); // for current frame
}

void VulkanEngine::renderImgui(VkCommandBuffer cmd) {
  // Rendering
  ImGui::Render();

  // Record dear imgui primitives into command buffer
  ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(), cmd);
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
