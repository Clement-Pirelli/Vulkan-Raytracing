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
				.format = vkut::common::findDepthFormat(),
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
		vkut::setup::destroyFramebuffer(framebuffers[i]);
	}

	vkut::common::destroyImageView(depthView);
	vkut::common::destroyImage(depthImage);
	vkut::common::destroyRenderPass(renderPass);
	vkut::setup::destroySwapchainImageViews();
	vkut::setup::destroySwapChain();
}

void Raytracer::recreateSwapchainDependents()
{
	vkut::setup::createSwapChain(width, height);
	vkut::setup::createSwapchainImageViews();

	std::vector<VkAttachmentDescription> colorDescriptions = getColorAttachmentDescriptions();
	VkAttachmentDescription depthDescription = getDepthAttachmentDescription();
	renderPass = vkut::common::createRenderPass(colorDescriptions, Optional<VkAttachmentDescription>(depthDescription));

	createDepthResources();
	createFramebuffers();
}

void Raytracer::createDepthResources()
{
	VkImageCreateInfo depthImageCreateInfo
	{
		.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
		.imageType = VK_IMAGE_TYPE_2D,
		.format = vkut::common::findDepthFormat(),
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
	depthImage = vkut::common::createImage(depthImageCreateInfo, flags);
	depthView = vkut::common::createImageView(depthImage.image, depthImageCreateInfo.format, VK_IMAGE_ASPECT_DEPTH_BIT, 1);

	vkut::common::transitionImageLayout(
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
		VkFramebuffer frameBuffer = vkut::setup::createRenderPassFramebuffer(
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
	window = vkut::setup::createWindow(title, width, height);
	vkut::setup::createInstance(title);
	vkut::setup::createDebugMessenger();
	vkut::setup::createSurface(window);
	vkut::setup::choosePhysicalDevice(requiredExtensions);
	vkut::setup::createLogicalDevice();
	vkut::setup::createSwapChain(width, height);
	vkut::setup::createSwapchainImageViews();

	std::vector<VkAttachmentDescription> colorDescriptions = getColorAttachmentDescriptions();
	VkAttachmentDescription depthDescription = getDepthAttachmentDescription();
	renderPass = vkut::common::createRenderPass(colorDescriptions, Optional<VkAttachmentDescription>(depthDescription));
	
	commandPool = vkut::setup::createGraphicsCommandPool();
	
	createDepthResources();
	createFramebuffers();
	vkut::setup::createSyncObjects(maxFramesInFlight);
	commandBuffers = vkut::common::createCommandBuffers(commandPool, framebuffers.size());

	do
	{
		glfwPollEvents();
	} while (!glfwWindowShouldClose(window));


	vkut::common::destroyCommandBuffers(commandPool, commandBuffers);
	vkut::setup::destroySyncObjects();
	for (size_t i = 0; i < framebuffers.size(); i++)
	{
		vkut::setup::destroyFramebuffer(framebuffers[i]);
	}

	vkut::common::destroyImageView(depthView);
	vkut::common::destroyImage(depthImage);
	vkut::setup::destroyCommandPool(commandPool);
	vkut::common::destroyRenderPass(renderPass);
	vkut::setup::destroySwapchainImageViews();
	vkut::setup::destroySwapChain();
	vkut::setup::destroyLogicalDevice();
	vkut::setup::destroySurface();
	vkut::setup::destroyDebugMessenger();
	vkut::setup::destroyInstance();
	vkut::setup::destroyWindow(window);
}
