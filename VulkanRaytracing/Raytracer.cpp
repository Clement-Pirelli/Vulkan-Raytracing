#include "Raytracer.h"
#include <assert.h>

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


void Raytracer::recreateSwapchainDependents()
{
	while (width == 0 || height == 0) {
		glfwGetFramebufferSize(window, &width, &height);
		glfwWaitEvents();
	}

	vkDeviceWaitIdle(vkut::device);

	//destruction
	vkut::common::destroyCommandBuffers(commandPool, commandBuffers);

	for (size_t i = 0; i < framebuffers.size(); i++)
	{
		vkut::setup::destroyFramebuffer(framebuffers[i]);
	}

	vkut::common::destroyImageView(depthView);
	vkut::common::destroyImage(depthImage);
	vkut::common::destroyRenderPass(renderPass);
	vkut::setup::destroySwapchainImageViews();
	vkut::setup::destroySwapChain();


	//recreation
	vkut::setup::createSwapChain(width, height);
	vkut::setup::createSwapchainImageViews();

	std::vector<VkAttachmentDescription> colorDescriptions = getColorAttachmentDescriptions();
	VkAttachmentDescription depthDescription = getDepthAttachmentDescription();
	renderPass = vkut::common::createRenderPass(colorDescriptions, Optional<VkAttachmentDescription>(depthDescription));

	createDepthResources();
	createFramebuffers();
	commandBuffers = vkut::common::createCommandBuffers(commandPool, framebuffers.size());
	recordCommandBuffers();
}

void Raytracer::recordCommandBuffers()
{

	constexpr uint32_t clearValuesCount = 2;
	VkClearValue clearValues[clearValuesCount] = {};
	clearValues[0].color = { 1.0f, .0f, .0f, 1.0f };
	clearValues[1].depthStencil = { 1.0f, 0 };

	VkRenderPassBeginInfo renderPassInfo
	{
		.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO,
		.renderPass = renderPass,
		.renderArea 
		{
			.offset = { 0, 0 },
			.extent = vkut::swapChainExtent,
		},
		.clearValueCount = static_cast<uint32_t>(clearValuesCount),
		.pClearValues = clearValues,
	};

	

	for (unsigned int i = 0; i < commandBuffers.size(); i++) {

		renderPassInfo.framebuffer = framebuffers[i];		
		vkut::common::startRecordCommandBuffer(commandBuffers[i]);
		vkCmdBeginRenderPass(commandBuffers[i], &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);


		vkCmdEndRenderPass(commandBuffers[i]);
		vkut::common::endRecordCommandBuffer(commandBuffers[i]);
	}

}

//todo: clean this up
void Raytracer::drawFrame()
{
	vkWaitForFences(vkut::device, 1, &vkut::inFlightFences[currentFrame], VK_TRUE, std::numeric_limits<uint64_t>::max());
	VkSemaphore currentSemaphore = vkut::imageAvailableSemaphores[currentFrame];

	uint32_t imageIndex;
	VkResult result = vkAcquireNextImageKHR(
		vkut::device,
		vkut::swapChain,
		std::numeric_limits<uint64_t>::max(),
		currentSemaphore,
		VK_NULL_HANDLE,
		&imageIndex);

	if (result == VK_ERROR_OUT_OF_DATE_KHR)
	{
		recreateSwapchainDependents(); return;
	}
	else
	{
		assert(result == VK_SUCCESS || result == VK_SUBOPTIMAL_KHR);
	}


	VkSubmitInfo submitInfo = {};
	submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

	VkPipelineStageFlags waitStages[] = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };
	submitInfo.waitSemaphoreCount = 1;
	submitInfo.pWaitSemaphores = &currentSemaphore;
	submitInfo.pWaitDstStageMask = waitStages;
	submitInfo.commandBufferCount = 1;

	submitInfo.pCommandBuffers = &commandBuffers[imageIndex];

	submitInfo.signalSemaphoreCount = 1;
	submitInfo.pSignalSemaphores = &currentSemaphore;

	VkFence *toReset = &vkut::inFlightFences[currentFrame];
	vkResetFences(vkut::device, 1, toReset);

	//todo: check this 
	vkQueueSubmit(vkut::graphicsQueue, 1, &submitInfo, *toReset);

	VkPresentInfoKHR presentInfo = {};
	presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
	presentInfo.waitSemaphoreCount = 1;
	presentInfo.pWaitSemaphores = &currentSemaphore;
	presentInfo.swapchainCount = 1;
	presentInfo.pSwapchains = &vkut::swapChain;
	presentInfo.pImageIndices = &imageIndex;
	presentInfo.pResults = nullptr; // Optional

	result = vkQueuePresentKHR(vkut::presentQueue, &presentInfo);
	if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR || windowResized)
	{
		windowResized = false;
		recreateSwapchainDependents();
	}
	else
	{
		assert(result == VK_SUCCESS);
	}

	currentFrame = (currentFrame + 1U) % maxFramesInFlight;

}

void Raytracer::framebufferResizeCallback(GLFWwindow *window, int width, int height)
{
	Raytracer *raytracer = reinterpret_cast<Raytracer *>(glfwGetWindowUserPointer(window));
	raytracer->width = width;
	raytracer->height = height;
	raytracer->windowResized = true;
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
	framebuffers.resize(vkut::swapChainImages.size());
	for (size_t i = 0; i < framebuffers.size(); i++)
	{
		std::vector<VkImageView> colorViews = { vkut::swapChainImageViews[i] };
		framebuffers[i] = vkut::setup::createRenderPassFramebuffer(
			renderPass,
			vkut::swapChainExtent.width,
			vkut::swapChainExtent.height,
			colorViews,
			Optional<VkImageView>(depthView)
		);
	}
}


void Raytracer::run()
{
	window = vkut::setup::createWindow(title, width, height);
	glfwSetWindowUserPointer(window, this);
	glfwSetFramebufferSizeCallback(window, Raytracer::framebufferResizeCallback);
	vkut::setup::createInstance(title);
	vkut::setup::createDebugMessenger();
	vkut::setup::createSurface(window);
	vkut::setup::choosePhysicalDevice(requiredExtensions);
	vkut::raytracing::getPhysicalDeviceRaytracingProperties();

	vkut::setup::createLogicalDevice();
	vkut::raytracing::initRaytracingFunctions();

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
	recordCommandBuffers();

	vkut::Buffer vertexBuffer = vkut::common::createBuffer(
		vertices.size() * sizeof(vertices[0]), 
		VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, 
		VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT
	);
	
	vkut::Buffer indexBuffer = vkut::common::createBuffer(
		indices.size() * sizeof(indices[0]), 
		VK_BUFFER_USAGE_INDEX_BUFFER_BIT, 
		VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT
	);

	do
	{
		glfwPollEvents();
		drawFrame();
	} while (!glfwWindowShouldClose(window));

	vkDeviceWaitIdle(vkut::device);

	vkut::common::destroyBuffer(indexBuffer);
	vkut::common::destroyBuffer(vertexBuffer);
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
