#include "vkutils.h"
#include "glfw3.h"
#include <assert.h>
#include "Logger/Logger.h"
#include <string>
#include <set>

#pragma warning(disable : 4100)
#pragma warning(disable : 4702)

namespace vkut {
	namespace {


		template <class T>
		T min(const T &a, const T &b)
		{
			return (a < b) ? a : b;
		}

		template <class T>
		T max(const T &a, const T &b)
		{
			return (a > b) ? a : b;
		}

#pragma region VALIDATION_LAYERS

#ifdef NDEBUG
		constexpr bool enableValidationLayers = false;
#else
		constexpr bool enableValidationLayers = true;
#endif
		constexpr unsigned int validationLayersCount = 1;
		constexpr const char *validationLayers[validationLayersCount] = { "VK_LAYER_KHRONOS_validation" };

		std::vector<const char *> deviceExtensions;
		VkDebugUtilsMessengerEXT debugMessenger;



		std::vector<const char *> getRequiredExtensions()
		{
			assert(glfwVulkanSupported());


			uint32_t glfwExtensionCount = 0;
			glfwGetRequiredInstanceExtensions(&glfwExtensionCount);
			assert(glfwExtensionCount != 0);

			std::vector<const char *> extensions = {};

			uint32_t count;
			const char **extPtr;
			extPtr = glfwGetRequiredInstanceExtensions(&count);

			for (size_t i = 0; i < count; i++) {
				extensions.push_back(extPtr[i]);
			}

			if constexpr (enableValidationLayers) {
				extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
			}

			count = glfwExtensionCount;

			return extensions;
		}



		bool checkValidationLayerSupport()
		{
			uint32_t layerCount;
			vkEnumerateInstanceLayerProperties(&layerCount, nullptr);

			std::vector<VkLayerProperties> availableLayers;
			availableLayers.resize(layerCount);
			vkEnumerateInstanceLayerProperties(&layerCount, availableLayers.data());

			for (unsigned int i = 0; i < validationLayersCount; i++) {
				bool layerFound = false;

				for (unsigned int j = 0; j < availableLayers.size(); j++)
				{
					if (strcmp(availableLayers[j].layerName, validationLayers[i]) == 0) {
						layerFound = true;
						break;
					}
				}

				if (!layerFound) {
					return false;
				}
			}

			return true;
		}

		VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity, VkDebugUtilsMessageTypeFlagsEXT messageType, const VkDebugUtilsMessengerCallbackDataEXT *pCallbackData, void *pUserData)
		{
			Logger::LogErrorFormatted("[validation]: %s\n", pCallbackData->pMessage);
			return VK_FALSE;
		}

		void DestroyDebugUtilsMessengerEXT(VkInstance givenInstance, VkDebugUtilsMessengerEXT givenDebugMessenger, const VkAllocationCallbacks *pAllocator) {
			auto func = (PFN_vkDestroyDebugUtilsMessengerEXT)vkGetInstanceProcAddr(givenInstance, "vkDestroyDebugUtilsMessengerEXT");
			if (func != nullptr) {
				func(givenInstance, givenDebugMessenger, pAllocator);
			}
		}

		VkResult CreateDebugUtilsMessengerEXT(VkInstance givenInstance, const VkDebugUtilsMessengerCreateInfoEXT *pCreateInfo, const VkAllocationCallbacks *pAllocator, VkDebugUtilsMessengerEXT *pDebugMessenger) {
			auto func = (PFN_vkCreateDebugUtilsMessengerEXT)vkGetInstanceProcAddr(givenInstance, "vkCreateDebugUtilsMessengerEXT");
			if (func != nullptr) {
				return func(givenInstance, pCreateInfo, pAllocator, pDebugMessenger);
			}
			else {
				return VK_ERROR_EXTENSION_NOT_PRESENT;
			}
		}

		void populateDebugMessengerCreateInfo(VkDebugUtilsMessengerCreateInfoEXT &createInfo)
		{
			createInfo = {
				.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT,
				.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT
					| VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT
					| VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT,
				.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT
					| VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT
					| VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT,
				.pfnUserCallback = debugCallback
			};
		}

#pragma endregion

		struct QueueFamilyIndices
		{
			Optional<uint32_t> graphicsFamily;
			Optional<uint32_t> presentFamily;

			bool isComplete() {
				return graphicsFamily.isSet() && presentFamily.isSet();
			}
		};

		QueueFamilyIndices findQueueFamilies(VkPhysicalDevice givenDevice) {
			QueueFamilyIndices indices;

			uint32_t queueFamilyCount = 0;
			vkGetPhysicalDeviceQueueFamilyProperties(givenDevice, &queueFamilyCount, nullptr);

			std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
			vkGetPhysicalDeviceQueueFamilyProperties(givenDevice, &queueFamilyCount, queueFamilies.data());

			for (unsigned int i = 0; i < queueFamilyCount; i++) {
				VkQueueFamilyProperties queueFamily = queueFamilies[i];

				VkBool32 presentSupport = false;
				vkGetPhysicalDeviceSurfaceSupportKHR(givenDevice, i, vkut::surface, &presentSupport);
				if (queueFamily.queueCount > 0 && presentSupport) {
					indices.presentFamily.setValue(i);
				}

				if (queueFamily.queueCount > 0 && queueFamily.queueFlags & VK_QUEUE_GRAPHICS_BIT) {
					indices.graphicsFamily.setValue(i);
					break;
				}
			}
			return indices;
		}

		struct SwapChainSupportDetails
		{
			VkSurfaceCapabilitiesKHR capabilities = {};
			std::vector<VkSurfaceFormatKHR> formats = {};
			std::vector<VkPresentModeKHR> presentModes = {};
		};

		SwapChainSupportDetails querySwapChainSupport(VkPhysicalDevice givenPhysicalDevice)
		{
			SwapChainSupportDetails details;

			vkGetPhysicalDeviceSurfaceCapabilitiesKHR(givenPhysicalDevice, vkut::surface, &details.capabilities);

			uint32_t formatCount;
			vkGetPhysicalDeviceSurfaceFormatsKHR(givenPhysicalDevice, vkut::surface, &formatCount, nullptr);

			if (formatCount != 0) {
				details.formats.resize(formatCount);
				vkGetPhysicalDeviceSurfaceFormatsKHR(givenPhysicalDevice, vkut::surface, &formatCount, details.formats.data());
			}

			uint32_t presentModeCount;
			vkGetPhysicalDeviceSurfacePresentModesKHR(givenPhysicalDevice, vkut::surface, &presentModeCount, nullptr);

			if (presentModeCount != 0) {
				details.presentModes.resize(presentModeCount);
				vkGetPhysicalDeviceSurfacePresentModesKHR(givenPhysicalDevice, vkut::surface, &presentModeCount, details.presentModes.data());
			}

			return details;
		}

		VkFormat findSupportedFormat(const std::vector<VkFormat> &candidates, VkImageTiling tiling, VkFormatFeatureFlags features)
		{
			for (size_t i = 0; i < candidates.size(); i++) {
				VkFormatProperties props;
				vkGetPhysicalDeviceFormatProperties(physicalDevice, candidates[i], &props);

				if (tiling == VK_IMAGE_TILING_LINEAR && (props.linearTilingFeatures & features) == features) {
					return candidates[i];
				}
				else if (tiling == VK_IMAGE_TILING_OPTIMAL && (props.optimalTilingFeatures & features) == features) {
					return candidates[i];
				}
			}

			Logger::LogError("Couldn't find format!");
			assert(false);
			return VkFormat::VK_FORMAT_UNDEFINED;
		}

		uint32_t findMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties)
		{
			VkPhysicalDeviceMemoryProperties memProperties;
			vkGetPhysicalDeviceMemoryProperties(vkut::physicalDevice, &memProperties);

			for (uint32_t i = 0; i < memProperties.memoryTypeCount; i++) {
				if ((typeFilter & (1 << i)) && (memProperties.memoryTypes[i].propertyFlags & properties) == properties) {
					return i;
				}
			}

			Logger::LogError("Failed to find suitable memory type!");
			assert(false);
			return ~0U;
		}

		bool hasStencilComponent(VkFormat format)
		{
			return format == VK_FORMAT_D32_SFLOAT_S8_UINT || format == VK_FORMAT_D24_UNORM_S8_UINT;
		}

		VkCommandBuffer initSingleTimeCommands(VkCommandPool commandPool) 
		{
			VkCommandBufferAllocateInfo allocInfo 
			{
				.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
				.commandPool = commandPool,
				.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
				.commandBufferCount = 1,
			};

			VkCommandBuffer commandBufferData;
			vkAllocateCommandBuffers(vkut::device, &allocInfo, &commandBufferData);

			VkCommandBufferBeginInfo beginInfo 
			{
				.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
				.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT
			};

			vkBeginCommandBuffer(commandBufferData, &beginInfo);

			return commandBufferData;
		}

		void submitSingleTimeCommands(VkCommandPool commandPool, VkCommandBuffer commandBuffer) {
			vkEndCommandBuffer(commandBuffer);

			VkSubmitInfo submitInfo = {};
			submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
			submitInfo.commandBufferCount = 1;
			submitInfo.pCommandBuffers = &commandBuffer;

			vkQueueSubmit(vkut::graphicsQueue, 1, &submitInfo, VK_NULL_HANDLE);
			vkQueueWaitIdle(vkut::graphicsQueue);

			vkFreeCommandBuffers(vkut::device, commandPool, 1, &commandBuffer);
		}

#pragma region PHYSICAL_DEVICE

		bool checkDeviceExtensionSupport(VkPhysicalDevice givenPhysicalDevice) {

			uint32_t availableExtensionCount = 0;
			vkEnumerateDeviceExtensionProperties(givenPhysicalDevice, nullptr, &availableExtensionCount, nullptr);

			std::vector<VkExtensionProperties> availableExtensions(availableExtensionCount);
			vkEnumerateDeviceExtensionProperties(givenPhysicalDevice, nullptr, &availableExtensionCount, availableExtensions.data());

			std::vector<std::string> requiredExtensions;
			for (unsigned int i = 0; i < deviceExtensions.size(); i++)
			{
				requiredExtensions.push_back(deviceExtensions[i]);
			}

			for (unsigned int i = 0; i < deviceExtensions.size(); i++)
			{
				for (unsigned int j = 0; j < availableExtensionCount; j++)
				{
					if (requiredExtensions[i] == availableExtensions[j].extensionName)
					{
						return true;
					}
				}
			}

			return false;
		}

		bool isPhysicalDeviceSuitable(VkPhysicalDevice givenPhysicalDevice)
		{
			QueueFamilyIndices indices = findQueueFamilies(givenPhysicalDevice);
			bool extensionsSupported = checkDeviceExtensionSupport(givenPhysicalDevice);

			bool swapChainAdequate = false;
			if (extensionsSupported) {
				SwapChainSupportDetails swapChainSupport = querySwapChainSupport(givenPhysicalDevice);
				swapChainAdequate = swapChainSupport.formats.size() != 0 && swapChainSupport.presentModes.size() != 0;
			}

			VkPhysicalDeviceFeatures supportedFeatures;
			vkGetPhysicalDeviceFeatures(givenPhysicalDevice, &supportedFeatures);

			return indices.isComplete() && extensionsSupported && swapChainAdequate && supportedFeatures.samplerAnisotropy;
		}

#pragma endregion

#pragma region SWAPCHAIN

		VkSurfaceFormatKHR chooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR> &availableFormats)
		{
			for (unsigned int i = 0; i < availableFormats.size(); i++) {
				const VkSurfaceFormatKHR &availableFormat = availableFormats[i];
				if (availableFormat.format == VK_FORMAT_B8G8R8A8_UNORM && availableFormat.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) {
					return availableFormat;
				}
			}

			return availableFormats[0];
		}

		VkPresentModeKHR chooseSwapPresentMode(const std::vector<VkPresentModeKHR> &availablePresentModes)
		{
			VkPresentModeKHR bestMode = VK_PRESENT_MODE_FIFO_KHR;

			for (unsigned int i = 0; i < availablePresentModes.size(); i++) {
				const VkPresentModeKHR &availablePresentMode = availablePresentModes[i];
				if (availablePresentMode == VK_PRESENT_MODE_MAILBOX_KHR) {
					return availablePresentMode;
				}
				else if (availablePresentMode == VK_PRESENT_MODE_IMMEDIATE_KHR)
				{
					bestMode = availablePresentMode;
				}
			}

			return bestMode;
		}

		VkExtent2D chooseSwapExtent(const VkSurfaceCapabilitiesKHR &capabilities, uint32_t width, uint32_t height)
		{
			if (capabilities.currentExtent.width != std::numeric_limits<uint32_t>::max()) {
				return capabilities.currentExtent;
			}
			else {
				VkExtent2D actualExtent = { width, height };

				actualExtent.width = max(capabilities.minImageExtent.width, min(capabilities.maxImageExtent.width, actualExtent.width));
				actualExtent.height = max(capabilities.minImageExtent.height, min(capabilities.maxImageExtent.height, actualExtent.height));

				return actualExtent;
			}

		}

#pragma endregion

	}

#pragma region COMMON

	void transitionImageLayout(VkCommandPool commandPool, VkImage image, VkFormat format, VkImageLayout oldLayout, VkImageLayout newLayout, uint32_t mipLevels)
	{
		VkCommandBuffer commandBufferData = initSingleTimeCommands(commandPool);
		
		VkImageMemoryBarrier barrier 
		{
			.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
			.oldLayout = oldLayout,
			.newLayout = newLayout,

			.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
			.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,

			.image = image,
			.subresourceRange = {
				.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
				.baseMipLevel = 0,
				.levelCount = mipLevels,
				.baseArrayLayer = 0,
				.layerCount = 1,
			}
		};


		VkPipelineStageFlags sourceStage = {};
		VkPipelineStageFlags destinationStage = {};

		if (newLayout == VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL) {
			barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;

			if (hasStencilComponent(format)) {
				barrier.subresourceRange.aspectMask |= VK_IMAGE_ASPECT_STENCIL_BIT;
			}
		}
		else {
			barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		}

		if (oldLayout == VK_IMAGE_LAYOUT_UNDEFINED && newLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL) {
			barrier.srcAccessMask = 0;
			barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;

			sourceStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
			destinationStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
		}
		else if (oldLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL && newLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL) {
			barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
			barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

			sourceStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
			destinationStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
		}
		else if (oldLayout == VK_IMAGE_LAYOUT_UNDEFINED && newLayout == VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL) {
			barrier.srcAccessMask = 0;
			barrier.dstAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;

			sourceStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
			destinationStage = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
		}
		else {
			Logger::LogError("Unsupported layout transition!");
			assert(false);
		}


		vkCmdPipelineBarrier(
			commandBufferData,
			sourceStage, 
			destinationStage,
			0,
			0, nullptr,
			0, nullptr,
			1, &barrier
		);

		submitSingleTimeCommands(commandPool, commandBufferData);
	}

	Image createImage(VkImageCreateInfo imageCreateInfo, VkMemoryPropertyFlags properties) 
	{
		Image image = {};

		VK_CHECK(vkCreateImage(device, &imageCreateInfo, nullptr, &image.image));

		VkMemoryRequirements memRequirements;
		vkGetImageMemoryRequirements(device, image.image, &memRequirements);

		VkMemoryAllocateInfo allocInfo{
			.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
			.allocationSize = memRequirements.size,
			.memoryTypeIndex = findMemoryType(memRequirements.memoryTypeBits, properties)
		};

		VK_CHECK(vkAllocateMemory(device, &allocInfo, nullptr, &image.memory));
		VK_CHECK(vkBindImageMemory(device, image.image, image.memory, 0));

		Logger::LogMessageFormatted("Created image %u with memory %u!\n", image.image, image.memory);
		return image;
	}

	void destroyImage(Image image)
	{
		vkDestroyImage(vkut::device, image.image, nullptr);
		vkFreeMemory(vkut::device, image.memory, nullptr);
		Logger::LogMessageFormatted("Destroyed image %u with memory %u!\n", image.image, image.memory);
	}

	VkFormat findDepthFormat()
	{
		return findSupportedFormat(
			std::vector<VkFormat> { VK_FORMAT_D32_SFLOAT, VK_FORMAT_D32_SFLOAT_S8_UINT, VK_FORMAT_D24_UNORM_S8_UINT },
			VK_IMAGE_TILING_OPTIMAL,
			VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT
		);
	}

	VkRenderPass createRenderPass(const std::vector<VkAttachmentDescription> &colorDescriptions, Optional<VkAttachmentDescription> depthDescription)
	{
		assert(colorDescriptions.size() > 0);
		std::vector<VkAttachmentReference> colorReferences;
		for (size_t i = 0; i < colorDescriptions.size(); i++) 
		{
			colorReferences.push_back(
				VkAttachmentReference
				{
					.attachment = static_cast<uint32_t>(i),
					.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
				}
			);
		}

		VkAttachmentReference depthReference
		{
			.attachment = static_cast<uint32_t>(colorReferences.size()),
			.layout = (depthDescription.isSet()) ? depthDescription.getValue().finalLayout : VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL
		};

		VkSubpassDescription subpass
		{
			.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS,
			.colorAttachmentCount = static_cast<uint32_t>(colorReferences.size()),
			.pColorAttachments = colorReferences.data(),
			.pDepthStencilAttachment = depthDescription.isSet() ? &depthReference : nullptr
		};
		
		VkSubpassDependency dependency
		{
			.srcSubpass = VK_SUBPASS_EXTERNAL,
			.dstSubpass = 0,
			.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
			.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
			.srcAccessMask = 0,
			.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
		};

		std::vector<VkAttachmentDescription> descriptions = std::vector<VkAttachmentDescription>(colorDescriptions);
		if (depthDescription.isSet()) descriptions.push_back(depthDescription.getValue());

		VkRenderPassCreateInfo renderPassInfo
		{
			.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO,
			.attachmentCount = static_cast<uint32_t>(descriptions.size()),
			.pAttachments = descriptions.data(),
			.subpassCount = 1,
			.pSubpasses = &subpass,
			.dependencyCount = 1,
			.pDependencies = &dependency
		};
		
		VkRenderPass renderPass;
		VK_CHECK(vkCreateRenderPass(device, &renderPassInfo, nullptr, &renderPass));
		Logger::LogMessageFormatted("Created render pass %u!\n", renderPass);

		return renderPass;
	}

	void destroyRenderPass(VkRenderPass renderPass)
	{
		vkDestroyRenderPass(vkut::device, renderPass, nullptr);
		Logger::LogMessageFormatted("Destroyed render pass %u!\n", renderPass);
	}

	VkImageView createImageView(VkImage image, VkFormat format, VkImageAspectFlags aspectFlags, uint32_t mipLevels)
	{
		VkImageView returnImageView;
		VkImageViewCreateInfo viewInfo
		{
			.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
			.image = image,
			.viewType = VK_IMAGE_VIEW_TYPE_2D,
			.format = format,
			.subresourceRange = {
				.aspectMask = aspectFlags,
				.baseMipLevel = 0,
				.levelCount = mipLevels,
				.baseArrayLayer = 0,
				.layerCount = 1
			}
		};

		VK_CHECK(vkCreateImageView(vkut::device, &viewInfo, nullptr, &returnImageView));
		Logger::LogMessageFormatted("Created image view %u!\n", returnImageView);

		return returnImageView;
	}

	void destroyImageView(VkImageView view) 
	{
		vkDestroyImageView(vkut::device, view, nullptr);
		Logger::LogMessageFormatted("Destroyed image view %u!\n", view);
	}

	std::vector<VkCommandBuffer> createCommandBuffers(VkCommandPool commandPool, size_t amount, VkCommandBufferLevel level) 
	{
		assert(amount != 0);
		std::vector<VkCommandBuffer> commandBuffers = std::vector<VkCommandBuffer>(amount);

		VkCommandBufferAllocateInfo allocInfo = {};
		allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
		allocInfo.commandPool = commandPool;
		allocInfo.level = level;
		allocInfo.commandBufferCount = static_cast<uint32_t>(commandBuffers.size());

		VK_CHECK(vkAllocateCommandBuffers(device, &allocInfo, commandBuffers.data()));

		Logger::LogMessageFormatted("Created %u command buffers!\n", amount);

		return commandBuffers;
	}

	void destroyCommandBuffers(VkCommandPool commandPool, const std::vector<VkCommandBuffer> &commandBuffers)
	{
		vkFreeCommandBuffers(vkut::device, commandPool, static_cast<uint32_t>(commandBuffers.size()), commandBuffers.data());
		Logger::LogMessageFormatted("Destroyed %u command buffers!\n", commandBuffers.size());
	}

#pragma endregion

#pragma region SETUP

	void createSyncObjects(size_t maxFramesInFlight)
	{
		imageAvailableSemaphores.resize(maxFramesInFlight);
		inFlightFences.resize(maxFramesInFlight);

		VkSemaphoreCreateInfo semaphoreInfo = {};
		semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

		VkFenceCreateInfo fenceInfo = {};
		fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
		fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

		for (size_t i = 0; i < maxFramesInFlight; i++) 
		{
			VK_CHECK(vkCreateSemaphore(device, &semaphoreInfo, nullptr, &imageAvailableSemaphores.data()[i]));
			VK_CHECK(vkCreateFence(device, &fenceInfo, nullptr, &inFlightFences.data()[i]));
		};

		Logger::LogMessage("Created sync objects!");
	}

	void destroySyncObjects()
	{
		for (size_t i = 0; i < imageAvailableSemaphores.size(); i++)
		{
			vkDestroySemaphore(vkut::device, imageAvailableSemaphores[i], nullptr);
		};

		for (size_t i = 0; i < inFlightFences.size(); i++)
		{
			vkDestroyFence(vkut::device, inFlightFences[i], nullptr);
		};
		Logger::LogMessage("Destroyed sync objects!");
	}


	VkFramebuffer createRenderPassFramebuffer(VkRenderPass renderPass, uint32_t width, uint32_t height, const std::vector<VkImageView> &colorViews, Optional<VkImageView> depthAttachment)
	{
		assert(colorViews.size() != 0);
		VkFramebuffer framebuffer = {};
		
		std::vector<VkImageView> attachments = std::vector<VkImageView>(colorViews);
		if (depthAttachment.isSet()) attachments.push_back(depthAttachment.getValue());

		VkFramebufferCreateInfo framebufferInfo{
			.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO,
			.renderPass = renderPass,
			.attachmentCount = static_cast<uint32_t>(attachments.size()),
			.pAttachments = attachments.data(),
			.width = width,
			.height = height,
			.layers = 1
		};
		VK_CHECK(vkCreateFramebuffer(vkut::device, &framebufferInfo, nullptr, &framebuffer));
		Logger::LogMessageFormatted("Created framebuffer %u with renderpass %u!\n", framebuffer, renderPass);
		return framebuffer;
	}

	void destroyFramebuffer(VkFramebuffer framebuffer)
	{
		vkDestroyFramebuffer(vkut::device, framebuffer, nullptr);
		Logger::LogMessageFormatted("Destroyed framebuffer %u!\n", framebuffer);
	}

	VkCommandPool createGraphicsCommandPool()
	{
		QueueFamilyIndices queueFamilyIndices = findQueueFamilies(vkut::physicalDevice);
		assert(queueFamilyIndices.graphicsFamily.isSet());

		VkCommandPoolCreateInfo poolInfo
		{
			.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
			.queueFamilyIndex = queueFamilyIndices.graphicsFamily.getValue(),
		};

		VkCommandPool commandPool;
		VK_CHECK(vkCreateCommandPool(vkut::device, &poolInfo, nullptr, &commandPool));
		
		Logger::LogMessageFormatted("Created graphics command pool %u!\n", commandPool);
		
		return commandPool;
	}

	void destroyCommandPool(VkCommandPool commandPool)
	{
		vkDestroyCommandPool(vkut::device, commandPool, nullptr);
		Logger::LogMessageFormatted("Destroyed command pool %u!\n", commandPool);
	}
	
	void createSwapchainImageViews()
	{
		vkut::swapChainImageViews.resize(swapChainImages.size());
		for (size_t i = 0; i < swapChainImageViews.size(); i++)
		{
			swapChainImageViews[i] = createImageView(swapChainImages[i], swapChainImageFormat, VK_IMAGE_ASPECT_COLOR_BIT, 1);
		}
		Logger::LogMessage("Created swapchain image views!");
	}

	void destroySwapchainImageViews() 
	{
		for (size_t i = 0; i < swapChainImageViews.size(); i++)
		{
			vkut::destroyImageView(swapChainImageViews[i]);
		}
		Logger::LogMessage("Destroyed swapchain image views!");
	}

	void createSwapChain(uint32_t width, uint32_t height)
	{
		SwapChainSupportDetails swapChainSupport = querySwapChainSupport(physicalDevice);

		VkSurfaceFormatKHR surfaceFormat = chooseSwapSurfaceFormat(swapChainSupport.formats);
		VkPresentModeKHR presentMode = chooseSwapPresentMode(swapChainSupport.presentModes);
		VkExtent2D extent = chooseSwapExtent(swapChainSupport.capabilities, width, height);

		uint32_t imageCount = swapChainSupport.capabilities.minImageCount + 1;

		if (swapChainSupport.capabilities.maxImageCount > 0 
			&& imageCount > swapChainSupport.capabilities.maxImageCount) 
		{
			imageCount = swapChainSupport.capabilities.maxImageCount;
		}

		VkSwapchainCreateInfoKHR createInfo
		{
			.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR,
			.surface = surface,
			.minImageCount = imageCount,
			.imageFormat = surfaceFormat.format,
			.imageColorSpace = surfaceFormat.colorSpace,
			.imageExtent = extent,
			.imageArrayLayers = 1,
			.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
			.preTransform = swapChainSupport.capabilities.currentTransform,
			.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR,
			.presentMode = presentMode,
			.clipped = VK_TRUE,
			.oldSwapchain = VK_NULL_HANDLE,
		};

		QueueFamilyIndices indices = findQueueFamilies(physicalDevice);
		uint32_t queueFamilyIndices[] = { indices.graphicsFamily.getValue(), indices.presentFamily.getValue() };

		if (indices.graphicsFamily.getValue() != indices.presentFamily.getValue()) 
		{
			createInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
			createInfo.queueFamilyIndexCount = 2;
			createInfo.pQueueFamilyIndices = queueFamilyIndices;
		}
		else 
		{
			createInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
		}


		VK_CHECK(vkCreateSwapchainKHR(vkut::device, &createInfo, nullptr, &vkut::swapChain));
		Logger::LogMessage("Created swapchain!");

		vkGetSwapchainImagesKHR(vkut::device, vkut::swapChain, &imageCount, nullptr);
		swapChainImages.resize(imageCount);
		vkGetSwapchainImagesKHR(vkut::device, vkut::swapChain, &imageCount, vkut::swapChainImages.data());

		vkut::swapChainImageFormat = surfaceFormat.format;
		vkut::swapChainExtent = extent;
	}

	void destroySwapChain()
	{
		vkDestroySwapchainKHR(device, swapChain, nullptr);
		Logger::LogMessage("Destroyed swapchain!");
	}

	void createLogicalDevice()
	{
		QueueFamilyIndices indices = findQueueFamilies(vkut::physicalDevice);


		std::vector<VkDeviceQueueCreateInfo> queueCreateInfos;

		std::set<uint32_t> uniqueQueueFamilies = { indices.graphicsFamily.getValue(), indices.presentFamily.getValue() };

		float queuePriority = 1.0f;
		for (uint32_t queueFamily : uniqueQueueFamilies)
		{
			VkDeviceQueueCreateInfo queueCreateInfo {
				.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
				.queueFamilyIndex = queueFamily,
				.queueCount = 1,
				.pQueuePriorities = &queuePriority
			};

			queueCreateInfos.push_back(queueCreateInfo);
		}


		VkPhysicalDeviceFeatures deviceFeatures {
			.samplerAnisotropy = VK_TRUE
		};

		VkDeviceCreateInfo createInfo {
			.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,
			.queueCreateInfoCount = static_cast<uint32_t>(queueCreateInfos.size()),
			.pQueueCreateInfos = queueCreateInfos.data(),
			.enabledExtensionCount = static_cast<uint32_t>(deviceExtensions.size()),
			.ppEnabledExtensionNames = deviceExtensions.data(),
			.pEnabledFeatures = &deviceFeatures,
		};

		if (enableValidationLayers)
		{
			createInfo.enabledLayerCount = static_cast<uint32_t>(validationLayersCount);
			createInfo.ppEnabledLayerNames = validationLayers;
		}
		else
		{
			createInfo.enabledLayerCount = 0;
		}

		VK_CHECK(vkCreateDevice(vkut::physicalDevice, &createInfo, nullptr, &vkut::device));
		Logger::LogMessage("Created logical device!");

		vkGetDeviceQueue(vkut::device, indices.graphicsFamily.getValue(), 0, &vkut::graphicsQueue);
		vkGetDeviceQueue(vkut::device, indices.presentFamily.getValue(), 0, &vkut::presentQueue);
	}

	void destroyLogicalDevice() 
	{
		vkDestroyDevice(vkut::device, nullptr);
		Logger::LogMessage("Destroyed logical device!");
	}

	void choosePhysicalDevice(std::vector<const char *> requiredDeviceExtensions)
	{
		deviceExtensions = requiredDeviceExtensions;
		uint32_t deviceCount = 0;
		vkEnumeratePhysicalDevices(vkut::instance, &deviceCount, nullptr);

		assert(deviceCount != 0);

		std::vector<VkPhysicalDevice> devices(deviceCount);
		vkEnumeratePhysicalDevices(vkut::instance, &deviceCount, devices.data());

		for (unsigned int i = 0; i < deviceCount; i++) 
		{
			VkPhysicalDevice dev = devices[i];
			if (isPhysicalDeviceSuitable(dev)) 
			{
				Logger::LogMessage("Chose physical device!");
				vkut::physicalDevice = dev;
				return;
			}
		}

		assert(false);
	}

	void createSurface(GLFWwindow *window)
	{
		VK_CHECK(glfwCreateWindowSurface(vkut::instance, window, nullptr, &vkut::surface));
		Logger::LogMessage("Created surface!");
	}

	void destroySurface()
	{
		vkDestroySurfaceKHR(vkut::instance, vkut::surface, nullptr);
		Logger::LogMessage("Destroyed surface!");
	}

	void createDebugMessenger()
	{
		if constexpr (!enableValidationLayers) 
			return;

		VkDebugUtilsMessengerCreateInfoEXT createInfo;
		populateDebugMessengerCreateInfo(createInfo);
		assert(CreateDebugUtilsMessengerEXT(vkut::instance, &createInfo, nullptr, &debugMessenger) == VK_SUCCESS);

		Logger::LogMessage("Created debug messenger!");
	}

	void destroyDebugMessenger() {
		if constexpr (enableValidationLayers) 
		{
			DestroyDebugUtilsMessengerEXT(vkut::instance, debugMessenger, nullptr);
			Logger::LogMessage("Destroyed debug messenger!");
		}
	}

	void createInstance(const char *title) 
	{

		assert(enableValidationLayers && checkValidationLayerSupport());


		VkApplicationInfo appInfo 
		{
			.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO,
			.pApplicationName = "Raytracer!",
			.applicationVersion = VK_MAKE_VERSION(1, 0, 0),
			.pEngineName = "No Engine",
			.engineVersion = VK_MAKE_VERSION(1, 0, 0),
			.apiVersion = VK_API_VERSION_1_2
		};

		VkInstanceCreateInfo createInfo {
			.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
			.pApplicationInfo = &appInfo
		};

		VkDebugUtilsMessengerCreateInfoEXT debugCreateInfo;
		if constexpr (enableValidationLayers) 
		{
			createInfo.enabledLayerCount = static_cast<uint32_t>(validationLayersCount);
			createInfo.ppEnabledLayerNames = validationLayers;

			populateDebugMessengerCreateInfo(debugCreateInfo);
			createInfo.pNext = (VkDebugUtilsMessengerCreateInfoEXT *)&debugCreateInfo;
		}
		else 
		{
			createInfo.enabledLayerCount = 0;
			createInfo.pNext = nullptr;
		}


		std::vector<const char *> glfwExtensions = getRequiredExtensions();
		createInfo.enabledExtensionCount = static_cast<uint32_t>(glfwExtensions.size());
		createInfo.ppEnabledExtensionNames = glfwExtensions.data();

		VK_CHECK(vkCreateInstance(&createInfo, nullptr, &vkut::instance));
		Logger::LogMessage("Created instance!");
	}

	void destroyInstance()
	{
		vkDestroyInstance(vkut::instance, nullptr);
		Logger::LogMessage("Destroyed instance!");
	}

	GLFWwindow *createWindow(const char *title, int width, int height) 
	{
		glfwInit();
		glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
		GLFWwindow *window = glfwCreateWindow(width, height, title, NULL, NULL);
		assert(window != NULL);
		Logger::LogMessage("Created window!");
		return window;
	}

	void destroyWindow(GLFWwindow *window) 
	{
		glfwDestroyWindow(window);
		glfwTerminate();
		Logger::LogMessage("Destroyed window!");
	}

#pragma endregion
}