// vulkan_guide.h : Include file for standard system include files,
// or project specific include files.

#pragma once

#include <vk_types.h>
#include <vector>

#include <glm/glm.hpp>
#include "vk_mesh.h"
#include "materials.h"
#include "types.h"
#include "camera.h"

#include "imgui.h"
#include "imgui_impl_sdl.h"
#include "imgui_impl_vulkan.h"

const u32 FRAME_OVERLAP = 2;

struct Material {
	VkPipeline pipeline;
	VkPipelineLayout pipelineLayout;
};

struct RenderObject {
	Mesh* mesh;
	Material* material;
	glm::mat4 transformMatrix;
};

struct GPUObjectData {
	glm::mat4 modelMatrix;
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

	AllocatedBuffer cameraBuffer;
	VkDescriptorSet globalDescriptorSet;
};

class VulkanEngine {
public:

	bool isInitialized{ false };
	u32 frameNumber {0};

	VkExtent2D windowExtent{ 1700 , 900 };

	struct SDL_Window* window{ nullptr };

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

	VmaAllocator allocator;

	VkImageView depthImageView;
	AllocatedImage depthImage;
	VkFormat depthFormat;

	//default array of renderable objects
	std::vector<RenderObject> renderables;

	std::unordered_map<std::string, Material> materials;
	std::unordered_map<std::string, Mesh> meshes;

	VkPipeline fragmentShaderPipeline;
	VkPipelineLayout fragmentShaderPipelineLayout;

	Camera camera;

	VkDescriptorSetLayout globalDescSetLayout;
	VkDescriptorPool descriptorPool;

	VkPhysicalDeviceProperties gpuProperties;

	GPUSceneData sceneParameters;
	AllocatedBuffer sceneParameterBuffer;

	struct {
		bool up, down, forward, back, left, right;
		bool button1, button2, button3;
		bool switch1;
		bool quit;
		f32 mouseDeltaX, mouseDeltaY;
	} input = {};

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
	void createPipeline(MaterialInfo matInfo);
	void initPipelines();
	void createFragmentShaderPipeline(const char* fragmentShader);
	void initScene();

	void initCamera();
	void loadMeshes();
	void uploadMesh(Mesh& mesh);

	//create material and add it to the map
	Material* createMaterial(VkPipeline pipeline, VkPipelineLayout layout, const std::string& name);
	//returns nullptr if it can't be found
	Material* getMaterial(const std::string& name);
	//returns nullptr if it can't be found
	Mesh* getMesh(const std::string& name);

	void drawFragmentShader(VkCommandBuffer cmd);

	void drawObjects(VkCommandBuffer cmd, RenderObject* first, u32 count);

	void startImguiFrame();
	void renderImgui(VkCommandBuffer cmd);

	void loadShaderModule(std::string filePath, VkShaderModule* outShaderModule);

	void processInput();

	void updateWorld();
	FrameData& getCurrentFrame();

	AllocatedBuffer createBuffer(size_t allocSize, VkBufferUsageFlags usage, VmaMemoryUsage memUsage);
	size_t padUniformBufferSize(size_t originalSize);
};
