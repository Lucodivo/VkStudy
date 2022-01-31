#pragma once

const u32 FRAME_OVERLAP = 2;

#define DEFAULT_WINDOW_WIDTH 1920
#define DEFAULT_WINDOW_HEIGHT 1080

struct Material {
  VkPipeline pipeline;
  VkPipelineLayout pipelineLayout;
};

struct RenderObject {
  Mesh* mesh;
  Material* material;
  const char* materialName;
  VkDescriptorSet textureSet{VK_NULL_HANDLE}; //texture defaulted to null
  glm::mat4 modelMatrix;
  glm::vec4 defaultColor;
};

struct GPUObjectData {
  glm::mat4 modelMatrix;
  glm::vec4 defaultColor;
};

struct GPUCameraData {
  glm::mat4 view;
  glm::mat4 projection;
  glm::mat4 viewproj;
};

struct GPUSceneData {
  glm::vec4 fogColor; // w is for exponent
  glm::vec4 fogDistances; //x for min, y for max, zw unused.
  glm::vec4 ambientColor;
  glm::vec4 sunlightDirection; //w for sun power
  glm::vec4 sunlightColor;
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

  VkExtent2D windowExtent{DEFAULT_WINDOW_WIDTH, DEFAULT_WINDOW_HEIGHT};

  struct SDL_Window* window{nullptr};

  //initializes everything in the engine
  void init();

  //shuts down the engine
  void cleanup();

  //draw loop
  void draw();

  //run main loop
  void run();

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

  Camera camera;

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

  struct {
    bool up, down, forward, back, left, right;
    bool button1, button2, button3;
    bool switch1;
    bool quit, fullscreen;
    f32 mouseDeltaX, mouseDeltaY;
  } input = {};

  MaterialCreateInfo materialInfos[3] = {
          materialDefaultLit,
          materialDefaulColor,
          materialTextured
  };

  UploadContext uploadContext;

  struct {
    bool generalDebug;
    bool mainMenu;
    bool fps;
  } imguiState;

private:

  void initSDL();

  void initImgui();

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

  void processInput();

  void update();

  void initScene();
  void initCamera();

  void cleanupSwapChain();
  void recreateSwapChain();

  void loadImages();
  void loadMeshes();
  void uploadMesh(Mesh& mesh);
  Mesh* getMesh(const std::string& name); //returns nullptr if it can't be found

  Material* createMaterial(VkPipeline pipeline, VkPipelineLayout layout, const char* name); //create material and add it to the map
  Material* getMaterial(const char* name); //returns nullptr if it can't be found

  void drawFragmentShader(VkCommandBuffer cmd);
  void drawObjects(VkCommandBuffer cmd, RenderObject* first, u32 count);

  void startImguiFrame();
  void renderImgui(VkCommandBuffer cmd);

  FrameData& getCurrentFrame();

  // ImGui functions for no thought debug window options
  // Not for anything but messily pushing info or adjusting values
  void quickDebugFloat(const char* label, float* v, float v_min, float v_max, const char* format = "%.3f", ImGuiSliderFlags flags = 0) const;
  void quickDebugText(const char* fmt, ...) const;
};
