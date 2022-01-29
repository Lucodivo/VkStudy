
#include "vk_initializers.h"

VkCommandPoolCreateInfo vkinit::commandPoolCreateInfo(uint32_t queueFamilyIndex, VkCommandPoolCreateFlags flags /*= 0*/) {
  VkCommandPoolCreateInfo info = {};
  info.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
  info.pNext = nullptr;

  info.queueFamilyIndex = queueFamilyIndex;
  info.flags = flags;
  return info;
}

VkCommandBufferAllocateInfo vkinit::commandBufferAllocateInfo(VkCommandPool pool, uint32_t count /*= 1*/, VkCommandBufferLevel level /*= VK_COMMAND_BUFFER_LEVEL_PRIMARY*/) {
  VkCommandBufferAllocateInfo info = {};
  info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
  info.pNext = nullptr;

  info.commandPool = pool;
  info.commandBufferCount = count;
  info.level = level;
  return info;
}

VkPipelineShaderStageCreateInfo vkinit::pipelineShaderStageCreateInfo(VkShaderStageFlagBits stage, VkShaderModule shaderModule) {

  VkPipelineShaderStageCreateInfo info{};
  info.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
  info.pNext = nullptr;
  info.stage = stage;
  //module containing the code for this shader stage
  info.module = shaderModule;
  //the entry point of the shader
  info.pName = "main";
  return info;
}

VkPipelineVertexInputStateCreateInfo vkinit::vertexInputStateCreateInfo() {
  VkPipelineVertexInputStateCreateInfo info = {};
  info.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
  info.pNext = nullptr;

  //no vertex bindings or attributes
  info.vertexBindingDescriptionCount = 0;
  info.vertexAttributeDescriptionCount = 0;
  return info;
}

VkPipelineInputAssemblyStateCreateInfo vkinit::inputAssemblyCreateInfo(VkPrimitiveTopology topology) {
  VkPipelineInputAssemblyStateCreateInfo info = {};
  info.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
  info.pNext = nullptr;
  info.topology = topology;
  info.primitiveRestartEnable = VK_FALSE;
  return info;
}

VkPipelineRasterizationStateCreateInfo vkinit::rasterizationStateCreateInfo(VkPolygonMode polygonMode) {
  VkPipelineRasterizationStateCreateInfo info = {};
  info.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
  info.pNext = nullptr;
  info.depthClampEnable = VK_FALSE;
  //discards all primitives before the rasterization stage if enabled which we don't want
  info.rasterizerDiscardEnable = VK_FALSE;
  info.polygonMode = polygonMode;
  info.lineWidth = 1.0f;
  info.cullMode = VK_CULL_MODE_NONE;
  info.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
  info.depthBiasEnable = VK_FALSE;
  info.depthBiasConstantFactor = 0.0f;
  info.depthBiasClamp = 0.0f;
  info.depthBiasSlopeFactor = 0.0f;
  return info;
}

VkPipelineMultisampleStateCreateInfo vkinit::multisamplingStateCreateInfo() {
  VkPipelineMultisampleStateCreateInfo info = {};
  info.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
  info.pNext = nullptr;
  info.sampleShadingEnable = VK_FALSE;
  info.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
  info.minSampleShading = 1.0f;
  info.pSampleMask = nullptr;
  info.alphaToCoverageEnable = VK_FALSE;
  info.alphaToOneEnable = VK_FALSE;
  return info;
}

VkPipelineColorBlendAttachmentState vkinit::colorBlendAttachmentState() {
  VkPipelineColorBlendAttachmentState colorBlendAttachment = {};
  colorBlendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT |
                                        VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
  colorBlendAttachment.blendEnable = VK_TRUE;
  colorBlendAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
  colorBlendAttachment.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
  colorBlendAttachment.colorBlendOp = VK_BLEND_OP_ADD;
  colorBlendAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
  colorBlendAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
  colorBlendAttachment.alphaBlendOp = VK_BLEND_OP_SUBTRACT;
  return colorBlendAttachment;
}

VkPipelineLayoutCreateInfo vkinit::pipelineLayoutCreateInfo() {
  VkPipelineLayoutCreateInfo info{};
  info.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
  info.pNext = nullptr;
  info.flags = 0;
  info.setLayoutCount = 0;
  info.pSetLayouts = nullptr;
  info.pushConstantRangeCount = 0;
  info.pPushConstantRanges = nullptr;
  return info;
}

VkImageCreateInfo vkinit::imageCreateInfo(VkFormat format, VkImageUsageFlags usageFlags, VkExtent3D extent) {
  VkImageCreateInfo info = {};
  info.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
  info.pNext = nullptr;

  info.imageType = VK_IMAGE_TYPE_2D;

  info.format = format;
  info.extent = extent;

  info.mipLevels = 1;
  info.arrayLayers = 1;
  info.samples = VK_SAMPLE_COUNT_1_BIT;
  info.tiling = VK_IMAGE_TILING_OPTIMAL;
  info.usage = usageFlags;

  return info;
}

VkImageViewCreateInfo vkinit::imageViewCreateInfo(VkFormat format, VkImage image, VkImageAspectFlags aspectFlags) {
  VkImageViewCreateInfo info = {};
  info.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
  info.pNext = nullptr;

  info.viewType = VK_IMAGE_VIEW_TYPE_2D;
  info.image = image;
  info.format = format;
  info.subresourceRange.baseMipLevel = 0;
  info.subresourceRange.levelCount = 1;
  info.subresourceRange.baseArrayLayer = 0;
  info.subresourceRange.layerCount = 1;
  info.subresourceRange.aspectMask = aspectFlags;

  return info;
}

VkPipelineDepthStencilStateCreateInfo vkinit::depthStencilCreateInfo(bool bDepthTest, bool bDepthWrite, VkCompareOp compareOp) {
  VkPipelineDepthStencilStateCreateInfo info = {};
  info.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
  info.pNext = nullptr;

  info.depthTestEnable = bDepthTest ? VK_TRUE : VK_FALSE;
  info.depthWriteEnable = bDepthWrite ? VK_TRUE : VK_FALSE;
  info.depthCompareOp = bDepthTest ? compareOp : VK_COMPARE_OP_ALWAYS;
  info.depthBoundsTestEnable = VK_FALSE;
  info.minDepthBounds = 0.0f; // Optional
  info.maxDepthBounds = 1.0f; // Optional
  info.stencilTestEnable = VK_FALSE;

  return info;
}

VkRenderPassBeginInfo vkinit::renderPassBeginInfo(VkRenderPass renderPass, VkExtent2D windowExtent, VkFramebuffer framebuffer) {
  VkRenderPassBeginInfo info = {};
  info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
  info.pNext = nullptr;

  info.renderPass = renderPass;
  info.renderArea.offset.x = 0;
  info.renderArea.offset.y = 0;
  info.renderArea.extent = windowExtent;
  info.framebuffer = framebuffer;
  return info;
}

VkCommandBufferBeginInfo vkinit::commandBufferBeginInfo(VkCommandBufferUsageFlags usageFlags) {
  VkCommandBufferBeginInfo cmdBeginInfo;
  cmdBeginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
  cmdBeginInfo.pNext = nullptr;
  cmdBeginInfo.pInheritanceInfo = nullptr;
  cmdBeginInfo.flags = usageFlags;
  return cmdBeginInfo;
}

VkSubmitInfo vkinit::submitInfo(VkCommandBuffer* cmd) {
  VkSubmitInfo info = {};
  info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
  info.pNext = nullptr;

  info.waitSemaphoreCount = 0;
  info.pWaitSemaphores = nullptr;
  info.pWaitDstStageMask = nullptr;
  info.commandBufferCount = 1;
  info.pCommandBuffers = cmd;
  info.signalSemaphoreCount = 0;
  info.pSignalSemaphores = nullptr;

  return info;
}

VkDescriptorSetLayoutBinding vkinit::descriptorSetLayoutBinding(VkDescriptorType descriptorType, VkShaderStageFlags pipelineStageFlags, u32 bindingIndex) {
  VkDescriptorSetLayoutBinding setbind = {};
  setbind.binding = bindingIndex;
  setbind.descriptorCount = 1;
  setbind.descriptorType = descriptorType;
  setbind.pImmutableSamplers = nullptr;
  setbind.stageFlags = pipelineStageFlags;

  return setbind;
}

VkWriteDescriptorSet vkinit::writeDescriptorBuffer(VkDescriptorType descriptorType, VkDescriptorSet descriptorSet, VkDescriptorBufferInfo* bufferInfo, u32 bindingIndex) {
  // NOTE: A descriptor set can have multiple bindings. And each binding can be an array of objects.
  VkWriteDescriptorSet write = {};
  write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
  write.pNext = nullptr;

  write.dstBinding = bindingIndex; // the binding being updated, as defined from a VkDescriptorSetLayoutBinding
  write.dstSet = descriptorSet; // descriptor set being written to
  write.descriptorCount = 1;// # of elements in our array of pImageInfo/pBufferInfo/pTexelBufferView descriptors at the binding point
  write.descriptorType = descriptorType;
  write.pBufferInfo = bufferInfo;

  return write;
}

VkFenceCreateInfo vkinit::fenceCreateInfo(VkFenceCreateFlags flags) {
  VkFenceCreateInfo fenceCreateInfo;
  fenceCreateInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
  fenceCreateInfo.pNext = nullptr;
  fenceCreateInfo.flags = flags;
  return fenceCreateInfo;
}

VkSemaphoreCreateInfo vkinit::semaphoreCreateInfo() {
  VkSemaphoreCreateInfo semaphoreCreateInfo;
  semaphoreCreateInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
  semaphoreCreateInfo.pNext = nullptr;
  semaphoreCreateInfo.flags = 0;
  return semaphoreCreateInfo;
}

VkWriteDescriptorSet vkinit::writeDescriptorImage(VkDescriptorType type, VkDescriptorSet dstSet, VkDescriptorImageInfo* imageInfo, u32 binding) {
  VkWriteDescriptorSet write = {};
  write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
  write.pNext = nullptr;

  write.dstBinding = binding;
  write.dstSet = dstSet;
  write.descriptorCount = 1;
  write.descriptorType = type;
  write.pImageInfo = imageInfo;

  return write;
}

VkSamplerCreateInfo vkinit::samplerCreateInfo(VkFilter filters, VkSamplerAddressMode samplerAddressMode) {
  VkSamplerCreateInfo info = {};
  info.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
  info.pNext = nullptr;

  info.magFilter = filters;
  info.minFilter = filters;
  info.addressModeU = samplerAddressMode;
  info.addressModeV = samplerAddressMode;
  info.addressModeW = samplerAddressMode;

  return info;
}
