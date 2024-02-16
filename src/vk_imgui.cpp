struct VkImguiInstance_T {
  VMA_CLASS_NO_COPY(VkImguiInstance_T)
public:
  VkImguiInstance_T(){}
  ~VkImguiInstance_T(){}
  VkDevice device = VK_NULL_HANDLE;
  VkDescriptorPool descriptorPool = VK_NULL_HANDLE;
};

void vkImguiTextWindow(const char* title, const CStringRingBuffer& ringBuffer, bool& show) {
  // debug text window
  if(show) {
    ImGui::Begin(title, &show, ImGuiWindowFlags_None);
    {
      ImGui::BeginChild("Scrolling");
      {
        u32 firstRoundIter = ringBuffer.first;
        while(firstRoundIter < ringBuffer.count) {
          ImGui::Text(ringBuffer.buffer + (firstRoundIter * ringBuffer.cStringMaxSize));
          firstRoundIter++;
        }

        u32 secondRoundIter = 0;
        while(secondRoundIter != ringBuffer.first) {
          ImGui::Text(ringBuffer.buffer + (secondRoundIter * ringBuffer.cStringMaxSize));
          secondRoundIter++;
        }
      }
      ImGui::EndChild();
    }
    ImGui::End();
  }
}

void vkImguiCreateInstance(const VkImguiCreateInfo* ci, VkImguiInstance* pImguiInstance) {
  *pImguiInstance = new VkImguiInstance_T();
  (*pImguiInstance)->device = ci->device;

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

//  ImguiState imguiState;

  // Create Descriptor Pool
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
    VK_CHECK(vkCreateDescriptorPool(ci->device, &poolInfo, nullptr, &((*pImguiInstance)->descriptorPool)));
  }

  WindowManager::getInstance().initImguiForWindow();
  ImGui_ImplVulkan_InitInfo initInfo = {};
  initInfo.Instance = ci->instance;
  initInfo.PhysicalDevice = ci->chosenGPU;
  initInfo.Device = ci->device;
  initInfo.QueueFamily = ci->graphicsQueueFamily;
  initInfo.Queue = ci->graphicsQueue;
  initInfo.PipelineCache = VK_NULL_HANDLE;
  initInfo.DescriptorPool = (*pImguiInstance)->descriptorPool;
  initInfo.MinImageCount = 3;
  initInfo.ImageCount = ci->swapChainImageCount;
  initInfo.MSAASamples = VK_SAMPLE_COUNT_1_BIT;
  ImGui_ImplVulkan_Init(&initInfo, ci->renderPass);

  // Upload Fonts
  {
    // Use any command queue
//    VK_CHECK(vkResetCommandPool(device, commandPool, 0));
    VkCommandBufferBeginInfo beginInfo = {};
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    beginInfo.flags |= VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
    VK_CHECK(vkBeginCommandBuffer(ci->fontTextUploadCommandBuffer, &beginInfo));

    ImGui_ImplVulkan_CreateFontsTexture(ci->fontTextUploadCommandBuffer);

    VkSubmitInfo endInfo = {};
    endInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    endInfo.commandBufferCount = 1;
    endInfo.pCommandBuffers = &ci->fontTextUploadCommandBuffer;
    VK_CHECK(vkEndCommandBuffer(ci->fontTextUploadCommandBuffer));
    VK_CHECK(vkQueueSubmit(ci->graphicsQueue, 1, &endInfo, VK_NULL_HANDLE));

    VK_CHECK(vkDeviceWaitIdle(ci->device));
    ImGui_ImplVulkan_DestroyFontUploadObjects();
  }
}

void vkImguiDeinit(VkImguiInstance imguiInstance) {
  vkDestroyDescriptorPool(imguiInstance->device, imguiInstance->descriptorPool, nullptr);
  ImGui_ImplVulkan_Shutdown();
}

void vkImguiStartFrame() {
  ImGui_ImplVulkan_NewFrame();
  ImGui_ImplSDL2_NewFrame();
  ImGui::NewFrame();
}

void vkImguiRender(VkCommandBuffer cmd) {
  // Rendering
  ImGui::Render();

  // Record dear imgui primitives into command buffer
  ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(), cmd);
}

void vkImguiQuickDebugText(bool showQuickDebug, const char* fmt, ...) {
  if(showQuickDebug) {
    va_list args;
            va_start(args, fmt);
    ImGui::TextV(fmt, args);
            va_end(args);
  }
}

void vkImguiQuickDebugFloat(bool showQuickDebug, const char* label, float* v, float v_min, float v_max, const char* format, ImGuiSliderFlags flags) {
  if(showQuickDebug) {
    ImGui::SliderFloat(label, v, v_min, v_max, format, flags);
  }
}