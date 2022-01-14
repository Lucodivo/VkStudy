
#include "vk_engine.h"

#include <iostream>
#include <fstream>

#include <SDL.h>
#include <SDL_vulkan.h>

#include <vk_types.h>
#include <vk_initializers.h>
#include <vk_pipeline_builder.h>

#include "VkBootstrap.h"

#define VMA_IMPLEMENTATION
#include "vk_mem_alloc.h"
#include <glm/gtx/transform.hpp>

#define DEFAULT_NANOSEC_TIMEOUT 1'000'000'000

#define PRESENT_MODE VK_PRESENT_MODE_FIFO_KHR

#define MAX_OBJECTS 100'000

const VkClearValue colorClearValue{
	{ 0.1f, 0.1f, 0.1f, 1.0f }
};

const VkClearValue depthClearValue {
	{ 1.f, 0.f }
};

void VulkanEngine::init()
{

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
	loadMeshes();
	initScene();

	initImgui();

	isInitialized = true;
}

void VulkanEngine::cleanup()
{	
	VK_CHECK(vkDeviceWaitIdle(device));

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
	FrameData& frame = getCurrentFrame();
	VK_CHECK(vkWaitForFences(device, 1, &frame.renderFence, true, DEFAULT_NANOSEC_TIMEOUT));
	VK_CHECK(vkResetFences(device, 1, &frame.renderFence));

	// present semaphore is signaled when the presentation engine has finished using the image and
	// it may now be used as a target for drawing
	u32 swapchainImageIndex;
	VK_CHECK(vkAcquireNextImageKHR(device, swapchain, DEFAULT_NANOSEC_TIMEOUT, frame.presentSemaphore, nullptr, &swapchainImageIndex));

	VkCommandBuffer cmd = frame.mainCommandBuffer;
	VK_CHECK(vkResetCommandBuffer(cmd, 0));

	VkCommandBufferBeginInfo cmdBeginInfo = vkinit::commandBufferBeginInfo(VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT);

	VK_CHECK(vkBeginCommandBuffer(cmd, &cmdBeginInfo));
	{
		VkClearValue clearValues[] = { colorClearValue, depthClearValue };

		VkRenderPassBeginInfo rpInfo = vkinit::renderPassBeginInfo(renderPass, windowExtent, framebuffers[swapchainImageIndex]);
		rpInfo.clearValueCount = 2;
		rpInfo.pClearValues = clearValues; // Clear values for each attachment

		// binds the framebuffers, clears the color & depth images and puts the image in the layout specified in the renderpass
		vkCmdBeginRenderPass(cmd, &rpInfo, VK_SUBPASS_CONTENTS_INLINE);

		drawFragmentShader(cmd);
		drawObjects(cmd, renderables.data(), renderables.size());
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

	VK_CHECK(vkQueuePresentKHR(graphicsQueue, &presentInfo));

	frameNumber++;
}

void VulkanEngine::processInput() {
	SDL_Event e;
	bool bQuit = false;

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

void VulkanEngine::updateWorld() {
	local_access bool imguiOpen = true;
	local_access f32 moveSpeed = 0.2f;
	local_access f32 turnSpeed = 0.5f;
	local_access bool editMode = true; // NOTE: We assume bool initialized as false
	glm::vec3 cameraDelta = {};
	f32 yawDelta = 0.0f;
	f32 pitchDelta = 0.0f;

	if (editMode != input.switch1) {
		editMode = input.switch1;
		SDL_SetRelativeMouseMode(editMode ? SDL_FALSE : SDL_TRUE);
	}

	// Used to keep camera at constant speed regardless of direction
	const f32 invSqrt[4] = {
		0.0f,
		1.0f,
		0.7071067f,
		0.5773502f
	};
	u32 invSqrtIndex = 0;

	if (input.left ^ input.right) {
		invSqrtIndex++;
		if (input.left) {
			cameraDelta.x = -1.0f;
		}
		else {
			cameraDelta.x = 1.0f;
		}
	}

	if (input.back ^ input.forward) {
		invSqrtIndex++;
		if (input.back) {
			cameraDelta.y = -1.0f;
		}
		else {
			cameraDelta.y = 1.0f;
		}
	}

	if (input.down ^ input.up) {
		invSqrtIndex++;
		if (input.down) {
			cameraDelta.z = -1.0f;
		}
		else {
			cameraDelta.z = 1.0f;
		}
	}

	if (!editMode && (input.mouseDeltaX != 0.0f || input.mouseDeltaY != 0.0f)) {
		f32 turnSpd = turnSpeed * 10.0;
		camera.turn(input.mouseDeltaX * turnSpd, input.mouseDeltaY * turnSpd);
	}
	
	// reset some input variables
	{
		input.mouseDeltaX = 0.0f;
		input.mouseDeltaY = 0.0f;
	}

	ImGui::Begin("Variable Adjustments", &imguiOpen, 0);
	{
		ImGui::SliderFloat("move speed", &moveSpeed, 0.1f, 1.0f, "%.2f");
		ImGui::SliderFloat("turn speed", &turnSpeed, 0.1f, 1.0f, "%.2f");
	}
	ImGui::End();

	camera.move(cameraDelta * (invSqrt[invSqrtIndex] * moveSpeed));
}

FrameData& VulkanEngine::getCurrentFrame()
{
	return frames[frameNumber % FRAME_OVERLAP];
}

AllocatedBuffer VulkanEngine::createBuffer(size_t allocSize, VkBufferUsageFlags usage, VmaMemoryUsage memUsage)
{
	//allocate vertex buffer
	VkBufferCreateInfo bufferInfo = {};
	bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
	bufferInfo.pNext = nullptr;

	bufferInfo.size = allocSize;
	bufferInfo.usage = usage;


	VmaAllocationCreateInfo vmaallocInfo = {};
	vmaallocInfo.usage = memUsage;

	AllocatedBuffer newBuffer;

	//allocate the buffer
	VK_CHECK(vmaCreateBuffer(allocator, &bufferInfo, &vmaallocInfo,
		&newBuffer.vkBuffer,
		&newBuffer.vmaAllocation,
		nullptr));

	mainDeletionQueue.pushFunction([=]() {
		vmaDestroyBuffer(allocator, newBuffer.vkBuffer, newBuffer.vmaAllocation);
		});

	return newBuffer;
}

size_t VulkanEngine::padUniformBufferSize(size_t originalSize)
{
	// Calculate required alignment based on minimum device offset alignment
	size_t alignment = gpuProperties.limits.minUniformBufferOffsetAlignment;
	size_t alignedSize = originalSize;
	if (alignment > 0) {
		// As the alignment must be a power of 2...
		// (alignment - 1) creates a mask that is all 1s below the alignment
		// ~(alignment - 1) creates a mask that is all 1s at and above the alignment
		alignedSize = (originalSize + (alignment - 1)) & ~(alignment - 1);

		// Note: similar idea as above but less optimized below
		// size_t alignmentCount = (original + (alignment - 1)) / alignment;
		// alignedSize = alignmentCount * alignment;
	}
	return alignedSize;
}

void VulkanEngine::run()
{
	SDL_Event e;
	bool bQuit = false;

	while (!input.quit)
	{
		processInput();
		startImguiFrame();
		updateWorld();
		draw();
	}
}

void VulkanEngine::initSDL()
{
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

void VulkanEngine::initImgui()
{
	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	ImGuiIO& io = ImGui::GetIO(); (void)io;
	//io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;     // Enable Keyboard Controls
	//io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;      // Enable Gamepad Controls

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
	imguiWindow.ImageCount = swapchainImageViews.size();
	imguiWindow.SemaphoreIndex = 0;
	imguiWindow.Frames = nullptr;
	imguiWindow.FrameSemaphores = nullptr;

	// Create Descriptor Pool
	VkDescriptorPool imguiDescriptorPool;
	{
		VkResult err;
		VkDescriptorPoolSize pool_sizes[] =
		{
				{ VK_DESCRIPTOR_TYPE_SAMPLER, 1000 },
				{ VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1000 },
				{ VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, 1000 },
				{ VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 1000 },
				{ VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER, 1000 },
				{ VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER, 1000 },
				{ VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1000 },
				{ VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1000 },
				{ VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, 1000 },
				{ VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC, 1000 },
				{ VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT, 1000 }
		};
		VkDescriptorPoolCreateInfo pool_info = {};
		pool_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
		pool_info.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;
		pool_info.maxSets = 1000 * IM_ARRAYSIZE(pool_sizes);
		pool_info.poolSizeCount = (uint32_t)IM_ARRAYSIZE(pool_sizes);
		pool_info.pPoolSizes = pool_sizes;
		VK_CHECK(vkCreateDescriptorPool(device, &pool_info, nullptr, &imguiDescriptorPool));
	}

	ImGui_ImplSDL2_InitForVulkan(window);
	ImGui_ImplVulkan_InitInfo init_info = {};
	init_info.Instance = instance;
	init_info.PhysicalDevice = chosenGPU;
	init_info.Device = device;
	init_info.QueueFamily = graphicsQueueFamily;
	init_info.Queue = graphicsQueue;
	init_info.PipelineCache = VK_NULL_HANDLE;
	init_info.DescriptorPool = imguiDescriptorPool;
	init_info.MinImageCount = 3;
	init_info.ImageCount = swapchainImageViews.size();
	init_info.MSAASamples = VK_SAMPLE_COUNT_1_BIT;
	ImGui_ImplVulkan_Init(&init_info, imguiWindow.RenderPass);

	// Upload Fonts
	{
		// Use any command queue
		VK_CHECK(vkResetCommandPool(device, frames[0].commandPool, 0));
		VkCommandBufferBeginInfo begin_info = {};
		begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
		begin_info.flags |= VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
		VK_CHECK(vkBeginCommandBuffer(frames[0].mainCommandBuffer, &begin_info));

		ImGui_ImplVulkan_CreateFontsTexture(frames[0].mainCommandBuffer);

		VkSubmitInfo end_info = {};
		end_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
		end_info.commandBufferCount = 1;
		end_info.pCommandBuffers = &frames[0].mainCommandBuffer;
		VK_CHECK(vkEndCommandBuffer(frames[0].mainCommandBuffer));
		VK_CHECK(vkQueueSubmit(graphicsQueue, 1, &end_info, VK_NULL_HANDLE));

		VK_CHECK(vkDeviceWaitIdle(device));
		ImGui_ImplVulkan_DestroyFontUploadObjects();
	}

	mainDeletionQueue.pushFunction([=]() {
		vkDestroyDescriptorPool(device, imguiDescriptorPool, nullptr);
		ImGui_ImplVulkan_Shutdown();
	});
}

void VulkanEngine::initVulkan()
{
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

	vkGetPhysicalDeviceProperties(chosenGPU, &gpuProperties);

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
	vmaCreateImage(allocator, &depthImageInfo, &depthImageAllocInfo, &depthImage.vkImage, &depthImage.vmaAllocation, nullptr);

	// build an image-view for the depth image to use in framebuffers for rendering
	VkImageViewCreateInfo depthImageViewInfo = vkinit::imageViewCreateInfo(depthFormat, depthImage.vkImage, VK_IMAGE_ASPECT_DEPTH_BIT);

	VK_CHECK(vkCreateImageView(device, &depthImageViewInfo, nullptr, &depthImageView));

	mainDeletionQueue.pushFunction([=]() {
		vkDestroyImageView(device, depthImageView, nullptr);
		vmaDestroyImage(allocator, depthImage.vkImage, depthImage.vmaAllocation);
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

	for (u32 i = 0; i < FRAME_OVERLAP; i++) {
		VK_CHECK(vkCreateCommandPool(device, &commandPoolInfo, nullptr, &frames[i].commandPool));
		
		VkCommandBufferAllocateInfo cmdAllocInfo = vkinit::commandBufferAllocateInfo(frames[i].commandPool, 1, VK_COMMAND_BUFFER_LEVEL_PRIMARY);
		
		VK_CHECK(vkAllocateCommandBuffers(device, &cmdAllocInfo, &frames[i].mainCommandBuffer));

		mainDeletionQueue.pushFunction([=]() {
			vkDestroyCommandPool(device, frames[i].commandPool, nullptr);
		});
	}

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

	VkSemaphoreCreateInfo semaphoreCreateInfo = {};
	semaphoreCreateInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
	semaphoreCreateInfo.pNext = nullptr;
	semaphoreCreateInfo.flags = 0;

	for (u32 i = 0; i < FRAME_OVERLAP; i++) {
		VK_CHECK(vkCreateFence(device, &fenceCreateInfo, nullptr, &frames[i].renderFence));
		VK_CHECK(vkCreateSemaphore(device, &semaphoreCreateInfo, nullptr, &frames[i].presentSemaphore));
		VK_CHECK(vkCreateSemaphore(device, &semaphoreCreateInfo, nullptr, &frames[i].renderSemaphore));
		mainDeletionQueue.pushFunction([=]() {
			vkDestroyFence(device, frames[i].renderFence, nullptr);
			vkDestroySemaphore(device, frames[i].renderSemaphore, nullptr);
			vkDestroySemaphore(device, frames[i].presentSemaphore, nullptr);
			});
	}

}

void VulkanEngine::initDescriptors()
{
	// reserve 10 uniform buffer pointers/handles in the descriptor pool
	VkDescriptorPoolSize descriptorPoolSizes[] = { 
		{ VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 10 },
		{ VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, 10 },
		{ VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 10 },
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

	VkDescriptorSetLayoutBinding globalBindings[] = { cameraBufferBinding, sceneBinding };

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

	VkDescriptorSetLayoutBinding objectBindings[] = { objectBufferBinding };

	descSetCreateInfo.bindingCount = ArrayCount(objectBindings);
	descSetCreateInfo.pBindings = objectBindings;

	vkCreateDescriptorSetLayout(device, &descSetCreateInfo, nullptr, &objectDescSetLayout);

	// TODO: Can/Should I allocate both descriptor sets under one VkDescriptorSetAllocateInfo?
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

	size_t paddedGPUCameraDataSize = padUniformBufferSize(sizeof(GPUCameraData));
	size_t paddedGPUSceneDataSize = padUniformBufferSize(sizeof(GPUSceneData));
	u32 globalBufferSize = FRAME_OVERLAP * (paddedGPUCameraDataSize + paddedGPUSceneDataSize);
	globalBuffer.cameraOffset = 0;
	globalBuffer.sceneOffset = FRAME_OVERLAP * paddedGPUCameraDataSize;
	globalBuffer.buffer = createBuffer(globalBufferSize, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VMA_MEMORY_USAGE_CPU_TO_GPU);
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
	VkWriteDescriptorSet descSetWrites[] = { cameraWrite, sceneWrite };

	// Note: This function links descriptor sets to the buffers that back their data.
	vkUpdateDescriptorSets(device, ArrayCount(descSetWrites), descSetWrites, 0/*descriptorCopyCount*/, nullptr/*pDescriptorCopies*/);

	const size_t objectBufferSize = sizeof(GPUObjectData) * MAX_OBJECTS;

	for (u32 i = 0; i < FRAME_OVERLAP; i++)
	{
		FrameData& frame = frames[i];

		frame.objectBuffer = createBuffer(objectBufferSize, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, VMA_MEMORY_USAGE_CPU_TO_GPU);
		vkAllocateDescriptorSets(device, &objectDescSetAllocInfo, &frame.objectDescriptorSet);

		VkDescriptorBufferInfo objectDescBufferInfo;
		objectDescBufferInfo.buffer = frame.objectBuffer.vkBuffer;
		objectDescBufferInfo.offset = 0;
		objectDescBufferInfo.range = objectBufferSize;

		VkWriteDescriptorSet objectWrite = vkinit::writeDescriptorBuffer(VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, frame.objectDescriptorSet, &objectDescBufferInfo, 0);

		VkWriteDescriptorSet descSetWrites[] = { objectWrite };

		vkUpdateDescriptorSets(device, ArrayCount(descSetWrites), descSetWrites, 0/*descriptorCopyCount*/, nullptr/*pDescriptorCopies*/);
	}

	mainDeletionQueue.pushFunction([=]() {
		// freeing descriptor pool frees descriptor layouts
		vkDestroyDescriptorSetLayout(device, globalDescSetLayout, nullptr);
		vkDestroyDescriptorSetLayout(device, objectDescSetLayout, nullptr);
		vkDestroyDescriptorPool(device, descriptorPool, nullptr);
	});
}

void VulkanEngine::initPipelines() {
	MaterialInfo matInfos[] = {
		materialDefaultLit,
		materialDefaulColor
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
	pipelineLayoutInfo.pushConstantRangeCount = 0;
	pipelineLayoutInfo.pPushConstantRanges = nullptr;
	VkDescriptorSetLayout descSets[] = { globalDescSetLayout, objectDescSetLayout };
	pipelineLayoutInfo.setLayoutCount = ArrayCount(descSets);
	pipelineLayoutInfo.pSetLayouts = descSets;

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
	focusObject.material = getMaterial(materialDefaultLit.name);
	glm::mat4 saturnTransform = glm::mat4{ 1.0f };
	saturnTransform = glm::scale(saturnTransform, glm::vec3(4.f));
	focusObject.modelMatrix = saturnTransform;
	focusObject.defaultColor = glm::vec4(155.0f / 255.0f, 115.0f / 255.0f, 96.0f / 255.0f, 1.0f);
	//renderables.push_back(focusObject);

	RenderObject environmentObject;
	environmentObject.mesh = getMesh("cube");
	environmentObject.material = getMaterial(materialDefaulColor.name);
	f32 scale = 0.2f;
	glm::mat4 scaleMat = glm::scale(glm::mat4{ 1.0 }, glm::vec3(scale, scale, scale));
	for (s32 x = -16; x <= 16; ++x)
	for (s32 y = -16; y <= 16; ++y)
	for (s32 z = 0; z <= 32; ++z) {
		glm::vec3 pos{ x, y, z };
		glm::vec3 color = (pos + glm::vec3(16.0f, 16.0f, 0.0f)) / glm::vec3(32.0f, 32.0f, 32.0f);
		glm::mat4 translationMat = glm::translate(glm::mat4{ 1.0 }, pos);
		environmentObject.modelMatrix = translationMat * scaleMat;
		environmentObject.defaultColor = glm::vec4(color, 1.0f);
		renderables.push_back(environmentObject);
	}

	// TODO: sort objects by material to minimize binding pipelines
	//std::sort(renderables.begin(), renderables.end(), [](const RenderObject& a, const RenderObject& b) {
	//	return a.material->pipeline > b.material->pipeline;
	//});
}

void VulkanEngine::initCamera()
{
	camera = {};
	camera.pos = { 0.f, -15.f, 6.f };
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
	triangleMesh.uploadMesh(allocator, &mainDeletionQueue);
	meshes["triangle"] = triangleMesh;

	//Mesh monkeyMesh = {};
	//monkeyMesh.loadFromObj("../assets/monkey_smooth.obj", "../assets/");
	//monkeyMesh.uploadMesh(allocator, mainDeletionQueue);
	//meshes["monkey"] = monkeyMesh;

	Mesh cubeMesh = {};
	cubeMesh.loadFromGltf("../assets/cube.glb");
	cubeMesh.uploadMesh(allocator, &mainDeletionQueue);
	meshes["cube"] = cubeMesh;

	Mesh mrSaturnMesh = {};
	mrSaturnMesh.loadFromGltf("../assets/mr_saturn.glb");
	mrSaturnMesh.uploadMesh(allocator, &mainDeletionQueue);
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
		&mesh.vertexBuffer.vkBuffer,
		&mesh.vertexBuffer.vmaAllocation,
		nullptr));

	mainDeletionQueue.pushFunction([=]() {
		vmaDestroyBuffer(allocator, mesh.vertexBuffer.vkBuffer, mesh.vertexBuffer.vmaAllocation);
	});

	void* data;
	vmaMapMemory(allocator, mesh.vertexBuffer.vmaAllocation, &data);
		memcpy(data, mesh.vertices.data(), mesh.vertices.size() * sizeof(Vertex));
	vmaUnmapMemory(allocator, mesh.vertexBuffer.vmaAllocation);
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

void VulkanEngine::drawObjects(VkCommandBuffer cmd, RenderObject* firstObject, u32 count)
{
	FrameData& frame = getCurrentFrame();

	// camera data
	GPUCameraData cameraData;
	cameraData.view = camera.getViewMatrix();
	cameraData.projection = glm::perspective(glm::radians(70.f), 1700.f / 900.f, 0.1f, 200.0f);
	cameraData.viewproj = cameraData.projection * cameraData.view;
	// scene data
	GPUSceneData sceneData;
	float magicNum = (frameNumber / 120.f);
	sceneData.ambientColor = { sin(magicNum), 0, cos(magicNum), 1 };

	// copy data to scene buffer
	char* globalDataBufferPtr;
	vmaMapMemory(allocator, globalBuffer.buffer.vmaAllocation, (void**)&globalDataBufferPtr);
		int frameIndex = frameNumber % FRAME_OVERLAP;
		// copy camera data
		char* cameraPtr = (globalDataBufferPtr + globalBuffer.cameraOffset);
		u32 cameraDataOffset = padUniformBufferSize(sizeof(GPUCameraData)) * frameIndex;
		cameraPtr += cameraDataOffset;
		memcpy(cameraPtr, &cameraData, sizeof(GPUCameraData));
		// copy scene data
		char* scenePtr = (globalDataBufferPtr + globalBuffer.sceneOffset);
		u32 sceneDataOffset = padUniformBufferSize(sizeof(GPUSceneData)) * frameIndex;
		scenePtr += sceneDataOffset;
		memcpy(scenePtr, &sceneData, sizeof(GPUSceneData));
		globalDataBufferPtr = nullptr, cameraPtr = nullptr, scenePtr = nullptr;
	vmaUnmapMemory(allocator, globalBuffer.buffer.vmaAllocation);

	// copy data to object buffer
	GPUObjectData* objectData;
	vmaMapMemory(allocator, frame.objectBuffer.vmaAllocation, (void**)&objectData);
		// Note: if my data was organized differently, possibly SoA or AoSoA instead of simply AoS, I could use memcpy for large chunks of data instead of iterating through a loop
		for (u32 i = 0; i < count; i++) {
			RenderObject& object = firstObject[i];
			objectData[i].modelMatrix = object.modelMatrix;
			objectData[i].defaultColor = object.defaultColor;
		}
		objectData = nullptr;
	vmaUnmapMemory(allocator, frame.objectBuffer.vmaAllocation);

	Mesh* lastMesh = nullptr;
	Material* lastMaterial = nullptr;
	for (u32 i = 0; i < count; i++)
	{
		RenderObject& object = firstObject[i];

		//only bind the pipeline if it doesn't match with the already bound one
		if (object.material != lastMaterial) {
			vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, object.material->pipeline);
			lastMaterial = object.material;

			// Note: It is only necessary to rebind descriptor sets if the desciptor layouts change between pipelines
			// Or if the dynamic uniform buffer offset needs to be updated
			u32 dynamicUniformOffsets[] = { cameraDataOffset, sceneDataOffset };

			VkDescriptorSet descSets[] = { globalDescriptorSet, frame.objectDescriptorSet };

			// vkCmdBindDescriptorSets causes the sets numbered [firstSet, firstSet+descriptorSetCount-1] to use the binding information stored in pDescriptorSets[0..descriptorSetCount-1]
			// dynamic uniform offsets must be provided for every dynamic uniform buffer descriptor in the descriptor set(s)
			// the dynamic offsets are ordered based firstly on the order of the descriptor set array and then on their binding index within that descriptor set
			vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, object.material->pipelineLayout, 0 /*first set*/, ArrayCount(descSets), descSets, ArrayCount(dynamicUniformOffsets), dynamicUniformOffsets);
		}

		VkViewport viewport{};
		viewport.x = 0;
		viewport.y = (f32)windowExtent.height;
		viewport.width = (f32)windowExtent.width;
		viewport.height = -viewport.y;
		viewport.minDepth = 0.0f;
		viewport.maxDepth = 1.0f;

		vkCmdSetViewport(cmd, 0, 1, &viewport);

		//only bind the mesh if it's a different one from last bind
		if (object.mesh != lastMesh) {
			//bind the mesh vertex buffer with offset 0
			VkDeviceSize offset = 0;
			vkCmdBindVertexBuffers(cmd, 0, 1, &object.mesh->vertexBuffer.vkBuffer, &offset);
			lastMesh = object.mesh;
		}

		vkCmdDraw(cmd, object.mesh->vertices.size(), 1, 0, i);
	}
}

void VulkanEngine::startImguiFrame() {
	ImGui_ImplVulkan_NewFrame();
	ImGui_ImplSDL2_NewFrame(window);
	ImGui::NewFrame();
}

void VulkanEngine::renderImgui(VkCommandBuffer cmd) {
	// Rendering
	ImGui::Render();

	// Record dear imgui primitives into command buffer
	ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(), cmd);
}

void VulkanEngine::loadShaderModule(std::string filePath, VkShaderModule* outShaderModule)
{
	//open the file. With cursor at the end
	filePath = SHADER_DIR + filePath + ".spv";
	std::ifstream file(filePath.c_str(), std::ios::ate | std::ios::binary);

	Assert(file.is_open());
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