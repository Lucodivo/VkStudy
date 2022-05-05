#pragma once

const u32 FRAME_OVERLAP = 2;

struct Material {
  VkPipeline pipeline;
  VkPipelineLayout pipelineLayout;
};

struct RenderObject {
  Mesh* mesh;
  Material* material;
  const char* materialName;
  VkDescriptorSet textureSet{VK_NULL_HANDLE}; //texture defaulted to null
  mat4 modelMatrix;
  vec4 defaultColor;
};

struct GPUObjectData {
  mat4 modelMatrix;
  vec4 defaultColor;
};

struct GPUCameraData {
  mat4 view;
  mat4 projection;
  mat4 viewproj;
};

struct GPUSceneData {
  vec4 fogColor; // w is for exponent
  vec4 fogDistances; //x for min, y for max, zw unused.
  vec4 ambientColor;
  vec4 sunlightDirection; //w for sun power
  vec4 sunlightColor;
};

struct FrameData {
  VkSemaphore presentSemaphore, renderSemaphore;
  VkFence renderFence;

  VkCommandPool commandPool;
  VkCommandBuffer mainCommandBuffer;

  // shader storage buffer
  AllocatedBuffer objectBuffer;
  VkDescriptorSet objectDescriptorSet;
};

class VulkanEngine {
public:

  bool isInitialized{false};
  u32 frameNumber{0};

  VkExtent2D windowExtent{0, 0};

  //initializes everything in the engine
  void init();

  //shuts down the engine
  void cleanup();

  // update based on input
  void initFrame(const bool showRenderDebugInfo);

  //draw loop
  void draw(const Camera& camera);

  VkInstance instance; // Vulkan library handle
  VkDebugUtilsMessengerEXT debugMessenger; // Vulkan debug output handle
  VkPhysicalDevice chosenGPU; // GPU chosen as the default device
  VkDevice device; // Vulkan device for commands
  VkSurfaceKHR surface; // Vulkan window surface
  VkPhysicalDeviceProperties gpuProperties;
  VkSwapchainKHR swapchain;
  VkFormat swapchainImageFormat;
  std::vector<VkImage> swapchainImages;
  std::vector<VkImageView> swapchainImageViews;// ---- other code -----

  VkQueue graphicsQueue; //queue we will submit to
  uint32_t graphicsQueueFamily; //family of that queue

  VkRenderPass renderPass;
  std::vector<VkFramebuffer> framebuffers;

  FrameData frames[FRAME_OVERLAP];

  DeletionQueue mainDeletionQueue;

  VmaAllocator vmaAllocator;

  VkImageView depthImageView;
  AllocatedImage depthImage;
  VkFormat depthFormat;

  //default array of renderable objects
  std::vector<RenderObject> renderables;

  MaterialManager materialManager;

  std::unordered_map<std::string, Material> materials;
  std::unordered_map<std::string, Mesh> meshes;
  std::unordered_map<std::string, Texture> loadedTextures;

  VkPipeline fragmentShaderPipeline;
  VkPipelineLayout fragmentShaderPipelineLayout;

  VkDescriptorSetLayout globalDescSetLayout;
  VkDescriptorSetLayout objectDescSetLayout;
  VkDescriptorSetLayout singleTextureSetLayout;
  VkDescriptorPool descriptorPool;

  VkDescriptorSet globalDescriptorSet;
  // a single dynamic uniform buffer used for all frames
  struct {
    AllocatedBuffer buffer;
    u64 sceneOffset;
    u32 cameraOffset;
  } globalBuffer;

  MaterialCreateInfo materialInfos[3] = {
          materialDefaultLit,
          materialDefaulColor,
          materialTextured
  };

  UploadContext uploadContext;

private:

  // == initializations ==
  void initVulkan();
  void initSwapchain();
  void initCommands();
  void initDefaultRenderpass();
  void initFramebuffers();
  void initSyncStructures();
  void initDescriptors();
  void createPipeline(MaterialCreateInfo matInfo);
  void initPipelines();
  void createFragmentShaderPipeline();

  void initScene();

  void cleanupSwapChain();
  void recreateSwapChain();

  void loadImages();
  void loadMeshes();
  Mesh* getMesh(const std::string& name); //returns nullptr if it can't be found

  Material* createMaterial(VkPipeline pipeline, VkPipelineLayout layout, const char* name); //create material and add it to the map
  Material* getMaterial(const char* name); //returns nullptr if it can't be found

  void drawFragmentShader(VkCommandBuffer cmd);
  void drawObjects(VkCommandBuffer cmd, const Camera& camera, RenderObject* first, u32 count);

  FrameData& getCurrentFrame();

  void initImgui();
  VkImguiInstance imguiInstance;
  bool renderDebugInfoRequestedForFrame = false; // NOTE: Do NOT modify this value outside of initFrame()
};
