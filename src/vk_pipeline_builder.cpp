VkPipeline PipelineBuilder::buildPipeline(VkDevice device, VkRenderPass pass) {
  VkPipelineViewportStateCreateInfo viewportState = {};
  viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
  viewportState.pNext = nullptr;
  viewportState.viewportCount = 1;
  viewportState.pViewports = &viewport;
  viewportState.scissorCount = 1;
  viewportState.pScissors = &scissor;

  VkPipelineColorBlendStateCreateInfo colorBlending = {};
  colorBlending.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
  colorBlending.pNext = nullptr;
  colorBlending.logicOpEnable = VK_FALSE;
  colorBlending.logicOp = VK_LOGIC_OP_COPY;
  colorBlending.attachmentCount = 1;
  colorBlending.pAttachments = &colorBlendAttachment;

  VkPipelineDynamicStateCreateInfo dynamicStateCreateInfo = {};
  VkDynamicState negativeViewPortDynamicState = VK_DYNAMIC_STATE_VIEWPORT;
  dynamicStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
  dynamicStateCreateInfo.pNext = nullptr;
  dynamicStateCreateInfo.pDynamicStates = &negativeViewPortDynamicState;
  dynamicStateCreateInfo.dynamicStateCount = 1;
  dynamicStateCreateInfo.flags = 0;

  VkGraphicsPipelineCreateInfo pipelineInfo = {};
  pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
  pipelineInfo.pNext = nullptr;
  pipelineInfo.stageCount = (u32)shaderStages.size();
  pipelineInfo.pStages = shaderStages.data();
  pipelineInfo.pVertexInputState = &vertexInputInfo;
  pipelineInfo.pInputAssemblyState = &inputAssembly;
  pipelineInfo.pViewportState = &viewportState;
  pipelineInfo.pRasterizationState = &rasterizer;
  pipelineInfo.pMultisampleState = &multisampling;
  pipelineInfo.pColorBlendState = &colorBlending;
  pipelineInfo.pDepthStencilState = &depthStencil;
  pipelineInfo.layout = pipelineLayout;
  pipelineInfo.renderPass = pass;
  pipelineInfo.subpass = 0;
  pipelineInfo.basePipelineHandle = VK_NULL_HANDLE;
  pipelineInfo.pDynamicState = &dynamicStateCreateInfo;

  //it's easy to error out on create graphics pipeline, so we handle it a bit better than the common VK_CHECK case
  VkPipeline newPipeline;
  if(vkCreateGraphicsPipelines(device, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &newPipeline) != VK_SUCCESS) {
    std::cout << "failed to create pipeline\n";
    return VK_NULL_HANDLE; // failed to create graphics pipeline
  }

  return newPipeline;
}