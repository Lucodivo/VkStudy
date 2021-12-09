
#include "vk_engine.h"

#include <iostream>
#include <fstream>

#include <SDL.h>
#include <SDL_vulkan.h>

#include <vk_types.h>
#include <vk_initializers.h>

#include "VkBootstrap.h"

#define VMA_IMPLEMENTATION
#include "vk_mem_alloc.h"
#include <glm/gtx/transform.hpp>

#define DEFAULT_NANOSEC_TIMEOUT 1'000'000'000

#define VK_CHECK(x)                                                 \
	do                                                              \
	{                                                               \
		VkResult err = x;                                           \
		if (err)                                                    \
		{                                                           \
			std::cout <<"Detected Vulkan error: " << err << std::endl; \
			abort();                                                \
		}                                                           \
	} while (0)

void VulkanEngine::init()
{
	SDL_Init(SDL_INIT_VIDEO);

	SDL_WindowFlags window_flags = (SDL_WindowFlags)(SDL_WINDOW_VULKAN);
	
	window = SDL_CreateWindow(
		"Vulkan Engine",
		SDL_WINDOWPOS_UNDEFINED,
		SDL_WINDOWPOS_UNDEFINED,
		windowExtent.width,
		windowExtent.height,
		window_flags
	);

	initCamera();

	initVulkan();
	initSwapchain();
	initCommands();
	initDefaultRenderpass();
	initFramebuffers();
	initSyncStructures();
	initPipelines();
	loadMeshes();
	initScene();

	isInitialized = true;
}

void VulkanEngine::cleanup()
{	
	VK_CHECK(vkWaitForFences(device, 1, &renderFence, true, DEFAULT_NANOSEC_TIMEOUT));

	if (isInitialized) 
	{
		mainDeletionQueue.flush();
		vmaDestroyAllocator(allocator);
		vkDestroyDevice(device, nullptr);
		vkDestroySurfaceKHR(instance, surface, nullptr);
		vkb::destroy_debug_utils_messenger(instance, debugMessenger);
		vkDestroyInstance(instance, nullptr);
		SDL_DestroyWindow(window);
	}
}

void VulkanEngine::draw()
{
	VK_CHECK(vkWaitForFences(device, 1, &renderFence, true, DEFAULT_NANOSEC_TIMEOUT));
	VK_CHECK(vkResetFences(device, 1, &renderFence));

	// present semaphore is signaled when the presentation engine has finished using the image and
	// it may now be used as a target for drawing
	u32 swapchainImageIndex;
	VK_CHECK(vkAcquireNextImageKHR(device, swapchain, DEFAULT_NANOSEC_TIMEOUT, presentSemaphore, nullptr, &swapchainImageIndex));

	VK_CHECK(vkResetCommandBuffer(mainCommandBuffer, 0));
	VkCommandBuffer cmd = mainCommandBuffer;

	VkCommandBufferBeginInfo cmdBeginInfo = vkinit::commandBufferBeginInfo(VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT);

	VK_CHECK(vkBeginCommandBuffer(cmd, &cmdBeginInfo));
	{
		VkClearValue colorClearValue{};
		colorClearValue.color = { { 0.1f, 0.1f, 0.1f, 1.0f } };

		VkClearValue depthClearValue{};
		depthClearValue.depthStencil.depth = 1.f;

		VkClearValue clearValues[] = { colorClearValue, depthClearValue };

		VkRenderPassBeginInfo rpInfo = vkinit::renderPassBeginInfo(renderPass, windowExtent, framebuffers[swapchainImageIndex]);
		rpInfo.clearValueCount = 2;
		rpInfo.pClearValues = clearValues;

		// binds the framebuffers, clears the color & depth images and puts the image in the layout specified in the renderpass
		vkCmdBeginRenderPass(cmd, &rpInfo, VK_SUBPASS_CONTENTS_INLINE);

		drawFragmentShader(cmd);
		drawObjects(cmd, renderables.data(), renderables.size());

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
	submitInfo.pWaitSemaphores = &presentSemaphore;
	//we will signal the renderSemaphore, to signal that rendering has finished
	submitInfo.signalSemaphoreCount = 1;
	submitInfo.pSignalSemaphores = &renderSemaphore;
	submitInfo.commandBufferCount = 1;
	submitInfo.pCommandBuffers = &cmd;

	// renderFence will now block until the graphic commands finish execution
	VK_CHECK(vkQueueSubmit(graphicsQueue, 1, &submitInfo, renderFence));

	VkPresentInfoKHR presentInfo = {};
	presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
	presentInfo.pNext = nullptr;
	presentInfo.pSwapchains = &swapchain;
	presentInfo.swapchainCount = 1;
	presentInfo.pWaitSemaphores = &renderSemaphore;
	presentInfo.waitSemaphoreCount = 1;
	presentInfo.pImageIndices = &swapchainImageIndex;

	VK_CHECK(vkQueuePresentKHR(graphicsQueue, &presentInfo));

	frameNumber++;
}

void VulkanEngine::run()
{
	SDL_Event e;
	bool bQuit = false;

	while (!bQuit)
	{
		while (SDL_PollEvent(&e) != 0)
		{
			switch (e.type) {
			case SDL_QUIT: bQuit = true; break;
			case SDL_KEYDOWN:
				switch (e.key.keysym.sym)
				{
				case SDLK_ESCAPE: bQuit = true; break;
				case SDLK_SPACE: break;
				case SDLK_a:
				case SDLK_LEFT:  
					camera.pos.x -= 0.1f;
					break;
				case SDLK_d:
				case SDLK_RIGHT:
					camera.pos.x += 0.1f;
					break;
				case SDLK_w:
				case SDLK_UP:
					camera.pos.z += 0.1f;
					break;
				case SDLK_s:
				case SDLK_DOWN:
					camera.pos.z -= 0.1f;
					break;
				case SDLK_q:
					camera.pos.y -= 0.1f;
					break;
				case SDLK_e:
					camera.pos.y += 0.1f;
					break;
				}
				break;
			case SDL_KEYUP:
				break;
			default: break;
			}
		}

		draw();
	}
}

void VulkanEngine::initVulkan()
{
	vkb::InstanceBuilder builder;

	vkb::Instance vkbInst = builder
		.set_app_name("Example Vulkan Application")
		.request_validation_layers(true)
		.require_api_version(1, 1, 0)
		.use_default_debug_messenger()
		.build()
		.value();

	instance = vkbInst.instance;
	debugMessenger = vkbInst.debug_messenger;

	SDL_Vulkan_CreateSurface(window, instance, &surface);

	VkPhysicalDeviceFeatures physicalDeviceFeatures{};
	physicalDeviceFeatures.fillModeNonSolid = VK_TRUE;

	vkb::PhysicalDeviceSelector selector{ vkbInst };
	vkb::PhysicalDevice physicalDevice = selector
		.set_minimum_version(1, 1)
		.set_surface(surface)
		.require_present()
		.set_required_features(physicalDeviceFeatures) // TODO: probably won't need this long term
		.select()
		.value();

	vkb::DeviceBuilder deviceBuilder{ physicalDevice };

	vkb::Device vkbDevice = deviceBuilder
		.build()
		.value();

	device = vkbDevice.device;
	chosenGPU = physicalDevice.physical_device;

	graphicsQueue = vkbDevice.get_queue(vkb::QueueType::graphics).value();
	graphicsQueueFamily = vkbDevice.get_queue_index(vkb::QueueType::graphics).value();

	//initialize the memory allocator
	VmaAllocatorCreateInfo allocatorInfo = {};
	allocatorInfo.physicalDevice = chosenGPU;
	allocatorInfo.device = device;
	allocatorInfo.instance = instance;
	vmaCreateAllocator(&allocatorInfo, &allocator);
}

void VulkanEngine::initSwapchain()
{
	vkb::SwapchainBuilder swapchainBuilder{ chosenGPU, device, surface };

	vkb::Swapchain vkbSwapchain = swapchainBuilder
		.use_default_format_selection()
		.set_desired_present_mode(VK_PRESENT_MODE_FIFO_KHR)
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
	vmaCreateImage(allocator, &depthImageInfo, &depthImageAllocInfo, &depthImage.image, &depthImage.allocation, nullptr);

	// build an image-view for the depth image to use in framebuffers for rendering
	VkImageViewCreateInfo depthImageViewInfo = vkinit::imageViewCreateInfo(depthFormat, depthImage.image, VK_IMAGE_ASPECT_DEPTH_BIT);

	VK_CHECK(vkCreateImageView(device, &depthImageViewInfo, nullptr, &depthImageView));

	mainDeletionQueue.pushFunction([=]() {
		vkDestroyImageView(device, depthImageView, nullptr);
		vmaDestroyImage(allocator, depthImage.image, depthImage.allocation);
		u32 swapchainImageCount = swapchainImageViews.size();
		for (u32 i = 0; i < swapchainImageCount; i++) {
			vkDestroyImageView(device, swapchainImageViews[i], nullptr);
		}
		vkDestroySwapchainKHR(device, swapchain, nullptr);
	});
}

void VulkanEngine::initCommands()
{
	VkCommandPoolCreateInfo commandPoolInfo = vkinit::commandPoolCreateInfo(graphicsQueueFamily, VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT);
	VK_CHECK(vkCreateCommandPool(device, &commandPoolInfo, nullptr, &commandPool));

	VkCommandBufferAllocateInfo cmdAllocInfo = vkinit::commandBufferAllocateInfo(commandPool, 1, VK_COMMAND_BUFFER_LEVEL_PRIMARY);
	VK_CHECK(vkAllocateCommandBuffers(device, &cmdAllocInfo, &mainCommandBuffer));

	mainDeletionQueue.pushFunction([=]() {
		vkDestroyCommandPool(device, commandPool, nullptr);
	});
}

void VulkanEngine::initDefaultRenderpass()
{
	VkAttachmentDescription colorAttachment = {};
	colorAttachment.format = swapchainImageFormat;
	colorAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
	colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
	colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
	colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	colorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	colorAttachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

	// Renderpasses hold VkAttachmentDescriptions but subpasses hold VkAttachmentReferences to those descriptions,
	// this allows two subpasses to simply reference the same attachment instead of holding the entire description
	VkAttachmentReference colorAttachmentRef = {};
	//attachment number will index into the pAttachments array in the parent renderpass
	colorAttachmentRef.attachment = 0;
	colorAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

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

	VkAttachmentReference depthAttachmentRef = {};
	depthAttachmentRef.attachment = 1;
	depthAttachmentRef.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

	//we are going to create 1 subpass, which is the minimum you can do
	VkSubpassDescription subpass = {};
	subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
	subpass.colorAttachmentCount = 1;
	subpass.pColorAttachments = &colorAttachmentRef;
	subpass.pDepthStencilAttachment = &depthAttachmentRef;

	VkAttachmentDescription attachmentDescriptions[] = { colorAttachment, depthAttachment };

	VkRenderPassCreateInfo renderPassInfo = {};
	renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
	renderPassInfo.attachmentCount = 2;
	renderPassInfo.pAttachments = attachmentDescriptions;
	renderPassInfo.subpassCount = 1;
	renderPassInfo.pSubpasses = &subpass;

	VK_CHECK(vkCreateRenderPass(device, &renderPassInfo, nullptr, &renderPass));

	mainDeletionQueue.pushFunction([=]() {
		vkDestroyRenderPass(device, renderPass, nullptr);
	});
}

void VulkanEngine::initFramebuffers()
{
	//create the framebuffers for the swapchain images. This will connect the render-pass to the images for rendering
	VkFramebufferCreateInfo fbInfo = {};
	fbInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
	fbInfo.pNext = nullptr;
	fbInfo.renderPass = renderPass;
	fbInfo.attachmentCount = 2;
	fbInfo.width = windowExtent.width;
	fbInfo.height = windowExtent.height;
	fbInfo.layers = 1;

	const u32 swapchainImageCount = swapchainImages.size();
	framebuffers = std::vector<VkFramebuffer>(swapchainImageCount);
	for (u32 i = 0; i < swapchainImageCount; i++) {
		VkImageView attachments[] = { swapchainImageViews[i], depthImageView };
		fbInfo.pAttachments = attachments;
		VK_CHECK(vkCreateFramebuffer(device, &fbInfo, nullptr, &framebuffers[i]));

		mainDeletionQueue.pushFunction([=]() {
			vkDestroyFramebuffer(device, framebuffers[i], nullptr);
		});
	}
}

void VulkanEngine::initSyncStructures()
{
	VkFenceCreateInfo fenceCreateInfo = {};
	fenceCreateInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
	fenceCreateInfo.pNext = nullptr;
	fenceCreateInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;
	VK_CHECK(vkCreateFence(device, &fenceCreateInfo, nullptr, &renderFence));

	VkSemaphoreCreateInfo semaphoreCreateInfo = {};
	semaphoreCreateInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
	semaphoreCreateInfo.pNext = nullptr;
	semaphoreCreateInfo.flags = 0;
	VK_CHECK(vkCreateSemaphore(device, &semaphoreCreateInfo, nullptr, &presentSemaphore));
	VK_CHECK(vkCreateSemaphore(device, &semaphoreCreateInfo, nullptr, &renderSemaphore));

	mainDeletionQueue.pushFunction([=]() {
		vkDestroyFence(device, renderFence, nullptr);
		vkDestroySemaphore(device, renderSemaphore, nullptr);
		vkDestroySemaphore(device, presentSemaphore, nullptr);
	});
}

void VulkanEngine::initPipelines() {
	MaterialInfo matInfos[] = {
		materialNormalAsRGB,
		materialRedOutline
	};

	createFragmentShaderPipeline("fragment_shader_test.frag");

	for (u32 i = 0; i < ArrayCount(matInfos); i++) {
		createPipeline(matInfos[i]);
	}
}

void VulkanEngine::createFragmentShaderPipeline(const char* fragmentShader) {
	VkShaderModule vertShader;
	loadShaderModule("hard_coded_fullscreen_quad.vert", &vertShader);

	VkShaderModule fragShader;
	loadShaderModule(fragmentShader, &fragShader);

	VkPipelineLayoutCreateInfo pipelineLayoutInfo = vkinit::pipelineLayoutCreateInfo();
	pipelineLayoutInfo.pPushConstantRanges = &fragmentShaderPushConstantsRange;
	pipelineLayoutInfo.pushConstantRangeCount = 1;

	VK_CHECK(vkCreatePipelineLayout(device, &pipelineLayoutInfo, nullptr, &fragmentShaderPipelineLayout));

	PipelineBuilder pipelineBuilder;
	pipelineBuilder.shaderStages.push_back(vkinit::pipelineShaderStageCreateInfo(VK_SHADER_STAGE_VERTEX_BIT, vertShader));
	pipelineBuilder.shaderStages.push_back(vkinit::pipelineShaderStageCreateInfo(VK_SHADER_STAGE_FRAGMENT_BIT, fragShader));
	pipelineBuilder.vertexInputInfo = vkinit::vertexInputStateCreateInfo();
	pipelineBuilder.inputAssembly = vkinit::inputAssemblyCreateInfo(VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST);
	pipelineBuilder.viewport.x = 0.0f;
	pipelineBuilder.viewport.y = 0.0f;
	pipelineBuilder.viewport.width = (float)windowExtent.width;
	pipelineBuilder.viewport.height = (float)windowExtent.height;
	pipelineBuilder.viewport.minDepth = 0.0f;
	pipelineBuilder.viewport.maxDepth = 1.0f;
	pipelineBuilder.scissor.offset = { 0, 0 };
	pipelineBuilder.scissor.extent = windowExtent;
	pipelineBuilder.rasterizer = vkinit::rasterizationStateCreateInfo(VK_POLYGON_MODE_FILL);
	pipelineBuilder.multisampling = vkinit::multisamplingStateCreateInfo();
	pipelineBuilder.colorBlendAttachment = vkinit::colorBlendAttachmentState();
	pipelineBuilder.depthStencil = vkinit::depthStencilCreateInfo(true, true, VK_COMPARE_OP_LESS_OR_EQUAL);
	pipelineBuilder.pipelineLayout = fragmentShaderPipelineLayout;

	fragmentShaderPipeline = pipelineBuilder.buildPipeline(device, renderPass);

	vkDestroyShaderModule(device, fragShader, nullptr);
	vkDestroyShaderModule(device, vertShader, nullptr);

	mainDeletionQueue.pushFunction([=]() {
		vkDestroyPipeline(device, fragmentShaderPipeline, nullptr);
		vkDestroyPipelineLayout(device, fragmentShaderPipelineLayout, nullptr);
		});
}

void VulkanEngine::createPipeline(MaterialInfo matInfo) {
	// TODO: vertex shader modules can be cached in case of reuse in multiple pipelines?
	VkShaderModule vertShader;
	loadShaderModule(matInfo.vertFileName, &vertShader);

	VkShaderModule fragShader;
	loadShaderModule(matInfo.fragFileName, &fragShader);

	// TODO: Pipeline layout can be reused for several pipelines/materials
	// TODO: Reuse of pipeline layouts will allows for push constants to be used for several pipelines
	// build mesh pipeline layout
	VkPipelineLayoutCreateInfo pipelineLayoutInfo = vkinit::pipelineLayoutCreateInfo();
	pipelineLayoutInfo.pPushConstantRanges = &matInfo.pushConstantRange;
	pipelineLayoutInfo.pushConstantRangeCount = 1;

	VkPipelineLayout pipelineLayout;
	VK_CHECK(vkCreatePipelineLayout(device, &pipelineLayoutInfo, nullptr, &pipelineLayout));

	PipelineBuilder pipelineBuilder;
	pipelineBuilder.shaderStages.push_back(vkinit::pipelineShaderStageCreateInfo(VK_SHADER_STAGE_VERTEX_BIT, vertShader));
	pipelineBuilder.shaderStages.push_back(vkinit::pipelineShaderStageCreateInfo(VK_SHADER_STAGE_FRAGMENT_BIT, fragShader));
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
	pipelineBuilder.scissor.offset = { 0, 0 };
	pipelineBuilder.scissor.extent = windowExtent;
	pipelineBuilder.rasterizer = vkinit::rasterizationStateCreateInfo(matInfo.polygonMode);
	pipelineBuilder.multisampling = vkinit::multisamplingStateCreateInfo();
	pipelineBuilder.colorBlendAttachment = vkinit::colorBlendAttachmentState();
	pipelineBuilder.depthStencil = vkinit::depthStencilCreateInfo(true, true, VK_COMPARE_OP_LESS_OR_EQUAL);
	pipelineBuilder.pipelineLayout = pipelineLayout;

	//connect the pipeline builder vertex input info to the one we get from Vertex
	VertexInputDescription vertexDescription = Vertex::getVertexDescriptions();
	pipelineBuilder.vertexInputInfo.pVertexAttributeDescriptions = vertexDescription.attributes.data();
	pipelineBuilder.vertexInputInfo.vertexAttributeDescriptionCount = vertexDescription.attributes.size();
	pipelineBuilder.vertexInputInfo.pVertexBindingDescriptions = vertexDescription.bindings.data();
	pipelineBuilder.vertexInputInfo.vertexBindingDescriptionCount = vertexDescription.bindings.size();

	VkPipeline pipeline = pipelineBuilder.buildPipeline(device, renderPass);

	createMaterial(pipeline, pipelineLayout, matInfo.name);

	vkDestroyShaderModule(device, fragShader, nullptr);
	vkDestroyShaderModule(device, vertShader, nullptr);

	mainDeletionQueue.pushFunction([=]() {
		vkDestroyPipeline(device, pipeline, nullptr);
		vkDestroyPipelineLayout(device, pipelineLayout, nullptr);
	});
}

void VulkanEngine::initScene()
{
	RenderObject focusObject;
	focusObject.mesh = getMesh("mrSaturn");
	focusObject.material = getMaterial(materialNormalAsRGB.name);
	glm::mat4 saturnTransform = glm::mat4{ 1.0f };
	saturnTransform = glm::rotate(saturnTransform, 90.0f * RadiansPerDegree, glm::vec3(0.f, 0.f, 1.f));
	saturnTransform = glm::rotate(saturnTransform, -90.0f * RadiansPerDegree, glm::vec3(0.f, 1.f, 0.f));
	saturnTransform = glm::scale(saturnTransform, glm::vec3(4.f));
	focusObject.transformMatrix = saturnTransform;

	renderables.push_back(focusObject);

	for (s32 x = -20; x <= 20; x++) {
		for (s32 y = -20; y <= 20; y++) {

			RenderObject environmentObject;
			environmentObject.mesh = getMesh("cube");
			environmentObject.material = getMaterial(materialRedOutline.name);
			glm::mat4 translation = glm::translate(glm::mat4{ 1.0 }, glm::vec3(x, 0, y));
			glm::mat4 scale = glm::scale(glm::mat4{ 1.0 }, glm::vec3(0.2, 0.2, 0.2));
			environmentObject.transformMatrix = translation * scale;

			renderables.push_back(environmentObject);
		}
	}

	// TODO: sort objects by material to minimize binding pipelines
	//std::sort(renderables.begin(), renderables.end(), [](const RenderObject& a, const RenderObject& b) {
	//	return a.material->pipeline > b.material->pipeline;
	//});
}

void VulkanEngine::initCamera()
{
	camera = {};
	camera.pos = { 0.f, 0.f, 10.f };
	camera.setForward({0.f, 1.f, 0.f});
}

void VulkanEngine::loadMeshes()
{
	Mesh triangleMesh = {};
	triangleMesh.vertices.resize(3);
	triangleMesh.vertices[0].position = { 1.f, 1.f, 0.0f };
	triangleMesh.vertices[1].position = { -1.f, 1.f, 0.0f };
	triangleMesh.vertices[2].position = { 0.f,-1.f, 0.0f };
	triangleMesh.vertices[0].normal = { 0.f, 1.f, 0.0f };
	triangleMesh.vertices[1].normal = { 0.f, 1.f, 0.0f };
	triangleMesh.vertices[2].normal = { 0.f, 1.f, 0.0f };

	Mesh monkeyMesh = {};
	monkeyMesh.loadFromObj("../assets/monkey_smooth.obj", "../assets/");

	Mesh cubeMesh = {};
	cubeMesh.loadFromGltf("../assets/cube.glb", true);

	Mesh mrSaturnMesh = {};
	mrSaturnMesh.loadFromGltf("../assets/mr_saturn.glb", true);

	uploadMesh(triangleMesh);
	uploadMesh(monkeyMesh);
	uploadMesh(cubeMesh);
	uploadMesh(mrSaturnMesh);

	meshes["monkey"] = monkeyMesh;
	meshes["triangle"] = triangleMesh;
	meshes["cube"] = cubeMesh;
	meshes["mrSaturn"] = mrSaturnMesh;
}

void VulkanEngine::uploadMesh(Mesh& mesh)
{
	//allocate vertex buffer
	VkBufferCreateInfo bufferInfo = {};
	bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
	bufferInfo.size = mesh.vertices.size() * sizeof(Vertex);
	bufferInfo.usage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;

	//let the VMA library know that this data should be writeable by CPU, but also readable by GPU
	VmaAllocationCreateInfo vmaallocInfo = {};
	vmaallocInfo.usage = VMA_MEMORY_USAGE_CPU_TO_GPU;

	VK_CHECK(vmaCreateBuffer(allocator, &bufferInfo, &vmaallocInfo,
		&mesh.vertexBuffer.buffer,
		&mesh.vertexBuffer.allocation,
		nullptr));

	mainDeletionQueue.pushFunction([=]() {
		vmaDestroyBuffer(allocator, mesh.vertexBuffer.buffer, mesh.vertexBuffer.allocation);
	});

	void* data;
	vmaMapMemory(allocator, mesh.vertexBuffer.allocation, &data);
		memcpy(data, mesh.vertices.data(), mesh.vertices.size() * sizeof(Vertex));
	vmaUnmapMemory(allocator, mesh.vertexBuffer.allocation);
}

Material* VulkanEngine::createMaterial(VkPipeline pipeline, VkPipelineLayout layout, const std::string& name)
{
	Material material;
	material.pipeline = pipeline;
	material.pipelineLayout = layout;
	materials[name] = material;
	return &materials[name];
}

Material* VulkanEngine::getMaterial(const std::string& name)
{
	//search for the object, and return nullptr if not found
	auto it = materials.find(name);
	if (it == materials.end()) {
		return nullptr;
	}
	else {
		return &(*it).second;
	}
}

Mesh* VulkanEngine::getMesh(const std::string& name)
{
	auto it = meshes.find(name);
	if (it == meshes.end()) {
		return nullptr;
	}
	else {
		return &(*it).second;
	}
}

void VulkanEngine::drawFragmentShader(VkCommandBuffer cmd) {
	vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, fragmentShaderPipeline);
	
	FragmentShaderPushConstants pushConstants;
	pushConstants.frameNumber = (f32)frameNumber;
	pushConstants.resolutionX = (f32)windowExtent.width;
	pushConstants.resolutionY = (f32)windowExtent.height;
	vkCmdPushConstants(cmd, fragmentShaderPipelineLayout, VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(FragmentShaderPushConstants), &pushConstants);

	vkCmdDraw(cmd, 6, 1, 0, 0);
}

void VulkanEngine::drawObjects(VkCommandBuffer cmd, RenderObject* first, u32 count)
{
	glm::mat4 view = glm::translate(glm::mat4(1.f), -camera.pos);
	glm::mat4 projection = glm::perspective(glm::radians(70.f), 1700.f / 900.f, 0.1f, 200.0f);

	Mesh* lastMesh = nullptr;
	Material* lastMaterial = nullptr;
	for (u32 i = 0; i < count; i++)
	{
		RenderObject& object = first[i];

		//only bind the pipeline if it doesn't match with the already bound one
		if (object.material != lastMaterial) {
			vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, object.material->pipeline);
			lastMaterial = object.material;
		}

		glm::mat4 model = object.transformMatrix;
		glm::mat4 meshMatrix = projection * view * model;

		Mat4x4PushConstants constants{};
		constants.renderMatrix = meshMatrix;

		vkCmdPushConstants(cmd, object.material->pipelineLayout, VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(Mat4x4PushConstants), &constants);

		//only bind the mesh if it's a different one from last bind
		if (object.mesh != lastMesh) {
			//bind the mesh vertex buffer with offset 0
			VkDeviceSize offset = 0;
			vkCmdBindVertexBuffers(cmd, 0, 1, &object.mesh->vertexBuffer.buffer, &offset);
			lastMesh = object.mesh;
		}

		vkCmdDraw(cmd, object.mesh->vertices.size(), 1, 0, 0);
	}
}

void VulkanEngine::loadShaderModule(std::string filePath, VkShaderModule* outShaderModule)
{
	//open the file. With cursor at the end
	filePath = SHADER_DIR + filePath + ".spv";
	std::ifstream file(filePath.c_str(), std::ios::ate | std::ios::binary);

	if (!file.is_open()) {
		std::cout << "Could not open shader file: " << filePath << std::endl;
	}

	//find what the size of the file is by looking up the location of the cursor
	size_t fileSize = (size_t)file.tellg();

	//spirv expects the buffer to be on uint32
	std::vector<u32> buffer(fileSize / sizeof(u32));

	//put file cursor at beginning
	file.seekg(0);
	file.read((char*)buffer.data(), fileSize);
	file.close();

	VkShaderModuleCreateInfo createInfo = {};
	createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
	createInfo.pNext = nullptr;
	createInfo.codeSize = buffer.size() * sizeof(u32);
	createInfo.pCode = buffer.data();

	//check that the creation goes well
	VkShaderModule shaderModule;
	if (vkCreateShaderModule(device, &createInfo, nullptr, &shaderModule) != VK_SUCCESS) {
		std::cout << "Opened file but couldn't create shader: " << filePath << std::endl;
	}

	std::cout << "Successfully created shader: " << filePath << std::endl;
	*outShaderModule = shaderModule;
}

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

	VkGraphicsPipelineCreateInfo pipelineInfo = {};
	pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
	pipelineInfo.pNext = nullptr;
	pipelineInfo.stageCount = shaderStages.size();
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

	//it's easy to error out on create graphics pipeline, so we handle it a bit better than the common VK_CHECK case
	VkPipeline newPipeline;
	if (vkCreateGraphicsPipelines(device, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &newPipeline) != VK_SUCCESS) {
		std::cout << "failed to create pipeline\n";
		return VK_NULL_HANDLE; // failed to create graphics pipeline
	}

	return newPipeline;
}