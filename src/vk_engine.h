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
#include "types.h"
#include "camera.h"

#include "imgui.h"
#include "imgui_impl_sdl.h"
#include "imgui_impl_vulkan.h"

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
		for(s32 i = deletors.size() - 1; i >= 0; i--) {
			deletors[i]();
		}
		deletors.clear();
	}
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

	VkPipeline fragmentShaderPipeline;
	VkPipelineLayout fragmentShaderPipelineLayout;

	Camera camera;

private:

	struct ImguiData {
		VkPipelineCache PipelineCache;
		VkDescriptorPool DescriptorPool;
		ImGui_ImplVulkanH_Window window;
	} imguiData;
	
	void initImgui();
	
	void initVulkan();
	void initSwapchain();
	void initCommands();
	void initDefaultRenderpass();
	void initFramebuffers();
	void initSyncStructures();
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

	void drawImgui(VkCommandBuffer cmd);

	void loadShaderModule(std::string filePath, VkShaderModule* outShaderModule);
};
