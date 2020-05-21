#include "Raytracer.h"

namespace {
	std::vector<VkAttachmentDescription> getColorAttachmentDescriptions()
	{
		return { 
			VkAttachmentDescription
			{
				.format = vkut::swapChainImageFormat,
				.samples = VK_SAMPLE_COUNT_1_BIT,
				.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
				.storeOp = VK_ATTACHMENT_STORE_OP_STORE,
				.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
				.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
				.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
				.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,
			}
		};
	}

	VkAttachmentDescription getDepthAttachmentDescription()
	{
		return VkAttachmentDescription 
		{
				.format = vkut::findDepthFormat(),
				.samples = VK_SAMPLE_COUNT_1_BIT,
				.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
				.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
				.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
				.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
				.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
				.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL
		};
	}
}

void Raytracer::cleanupSwapchainDependents()
{
	for (size_t i = 0; i < framebuffers.size(); i++)
	{
		vkut::destroyFramebuffer(framebuffers[i]);
	}

	vkut::destroyImageView(depthView);
	vkut::destroyImage(depthImage);
	vkut::destroyRenderPass(renderPass);
	vkut::destroySwapchainImageViews();
	vkut::destroySwapChain();
}

void Raytracer::recreateSwapchainDependents()
{
	vkut::createSwapChain(width, height);
	vkut::createSwapchainImageViews();

	std::vector<VkAttachmentDescription> colorDescriptions = getColorAttachmentDescriptions();
	VkAttachmentDescription depthDescription = getDepthAttachmentDescription();
	renderPass = vkut::createRenderPass(colorDescriptions, Optional<VkAttachmentDescription>(depthDescription));

	createDepthResources();
	createFramebuffers();
}

void Raytracer::createDepthResources()
{
	VkImageCreateInfo depthImageCreateInfo
	{
		.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
		.imageType = VK_IMAGE_TYPE_2D,
		.format = vkut::findDepthFormat(),
		.extent = {
			.width = static_cast<uint32_t>(width),
			.height = static_cast<uint32_t>(height),
			.depth = 1,
		},
		.mipLevels = 1,
		.arrayLayers = 1,
		.samples = VK_SAMPLE_COUNT_1_BIT,
		.tiling = VK_IMAGE_TILING_OPTIMAL,
		.usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT,
		.sharingMode = VK_SHARING_MODE_EXCLUSIVE,
		.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
	};

	constexpr VkMemoryPropertyFlags flags = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
	depthImage = vkut::createImage(depthImageCreateInfo, flags);
	depthView = vkut::createImageView(depthImage.image, depthImageCreateInfo.format, VK_IMAGE_ASPECT_DEPTH_BIT, 1);

	vkut::transitionImageLayout(
		commandPool,
		depthImage.image,
		depthImageCreateInfo.format,
		VK_IMAGE_LAYOUT_UNDEFINED,
		VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
		1
	);
}

void Raytracer::createFramebuffers()
{
	for (size_t i = 0; i < vkut::swapChainImages.size(); i++)
	{
		std::vector<VkImageView> colorViews = { vkut::swapChainImageViews[i] };
		VkFramebuffer frameBuffer = vkut::createRenderPassFramebuffer(
			renderPass,
			vkut::swapChainExtent.width,
			vkut::swapChainExtent.height,
			colorViews,
			Optional<VkImageView>(depthView)
		);
		framebuffers.push_back(frameBuffer);
	}
}


void Raytracer::run()
{
	window = vkut::createWindow(title, width, height);
	vkut::createInstance(title);
	vkut::createDebugMessenger();
	vkut::createSurface(window);
	vkut::choosePhysicalDevice(requiredExtensions);
	vkut::createLogicalDevice();
	vkut::createSwapChain(width, height);
	vkut::createSwapchainImageViews();

	std::vector<VkAttachmentDescription> colorDescriptions = getColorAttachmentDescriptions();
	VkAttachmentDescription depthDescription = getDepthAttachmentDescription();
	renderPass = vkut::createRenderPass(colorDescriptions, Optional<VkAttachmentDescription>(depthDescription));
	
	commandPool = vkut::createGraphicsCommandPool();
	
	createDepthResources();
	createFramebuffers();
	vkut::createSyncObjects(maxFramesInFlight);
	commandBuffers = vkut::createCommandBuffers(commandPool, framebuffers.size());

	do
	{
		glfwPollEvents();
	} while (!glfwWindowShouldClose(window));


	vkut::destroyCommandBuffers(commandPool, commandBuffers);
	vkut::destroySyncObjects();
	for (size_t i = 0; i < framebuffers.size(); i++)
	{
		vkut::destroyFramebuffer(framebuffers[i]);
	}

	vkut::destroyImageView(depthView);
	vkut::destroyImage(depthImage);
	vkut::destroyCommandPool(commandPool);
	vkut::destroyRenderPass(renderPass);
	vkut::destroySwapchainImageViews();
	vkut::destroySwapChain();
	vkut::destroyLogicalDevice();
	vkut::destroySurface();
	vkut::destroyDebugMessenger();
	vkut::destroyInstance();
	vkut::destroyWindow(window);
}
