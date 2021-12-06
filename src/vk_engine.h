// vulkan_guide.h : Include file for standard system include files,
// or project specific include files.

#pragma once

#include <vk_types.h>
#include <vector>
#include <functional>
#include <deque>

#include <glm/glm.hpp>
#include "vk_mesh.h"
#include "materials.h"

#define ArrayCount(Array) (sizeof(Array) / sizeof((Array)[0]))

struct Material {
	VkPipeline pipeline;
	VkPipelineLayout pipelineLayout;
};

struct RenderObject {
	Mesh* mesh;
	Material* material;
	glm::mat4 transformMatrix;
};

class PipelineBuilder {
public:

	std::vector<VkPipelineShaderStageCreateInfo> shaderStages;
	VkPipelineVertexInputStateCreateInfo vertexInputInfo;
	VkPipelineInputAssemblyStateCreateInfo inputAssembly;
	VkViewport viewport;
	VkRect2D scissor;
	VkPipelineRasterizationStateCreateInfo rasterizer;
	VkPipelineColorBlendAttachmentState colorBlendAttachment;
	VkPipelineMultisampleStateCreateInfo multisampling;
	VkPipelineLayout pipelineLayout;
	VkPipelineDepthStencilStateCreateInfo depthStencil;

	VkPipeline buildPipeline(VkDevice device, VkRenderPass pass);
};

struct DeletionQueue
{
	std::vector<std::function<void()>> deletors;

	void pushFunction(std::function<void()>&& function) {
		deletors.push_back(function);
	}

	void flush() {
		for (int i = deletors.size() - 1; i >= 0; i--) {
			deletors[i]();
		}
		deletors.clear();
	}
};

class VulkanEngine {
public:

	bool isInitialized{ false };
	int frameNumber {0};

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
	VkCommandPool commandPool; //the command pool for our commands
	VkCommandBuffer mainCommandBuffer; //the buffer we will record into

	VkRenderPass renderPass;
	std::vector<VkFramebuffer> framebuffers;

	VkSemaphore presentSemaphore, renderSemaphore;
	VkFence renderFence;

	DeletionQueue mainDeletionQueue;

	VmaAllocator allocator;

	VkImageView depthImageView;
	AllocatedImage depthImage;
	VkFormat depthFormat;


	//default array of renderable objects
	std::vector<RenderObject> renderables;

	std::unordered_map<std::string, Material> materials;
	std::unordered_map<std::string, Mesh> meshes;

	glm::vec3 cameraPos = { 0.f,-6.f,-10.f };

private:
	
	void initVulkan();
	void initSwapchain();
	void initCommands();
	void initDefaultRenderpass();
	void initFramebuffers();
	void initSyncStructures();
	void createPipeline(MaterialInfo matInfo);
	void initPipelines();
	void initScene();

	void createPipeline(const char* vertexShader, const char* fragmentShader);

	void loadMeshes();
	void uploadMesh(Mesh& mesh);

	//create material and add it to the map
	Material* createMaterial(VkPipeline pipeline, VkPipelineLayout layout, const std::string& name);
	//returns nullptr if it can't be found
	Material* getMaterial(const std::string& name);
	//returns nullptr if it can't be found
	Mesh* getMesh(const std::string& name);

	void drawObjects(VkCommandBuffer cmd, RenderObject* first, int count);

	void loadShaderModule(std::string filePath, VkShaderModule* outShaderModule);
};
