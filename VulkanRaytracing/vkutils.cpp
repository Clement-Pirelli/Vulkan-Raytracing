#include "vkutils.h"
#include "glfw3.h"
#include <assert.h>
#include "Logger/Logger.h"
#include "Span.h"
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

#pragma region VALIDATION_LAYERS

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
			.layout = VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL
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
		Logger::LogMessageFormatted("Destroyed render pass %u\n", renderPass);
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

	void destroyImageView(VkImageView view) {
		vkDestroyImageView(vkut::device, view, nullptr);
		Logger::LogMessageFormatted("Destroyed image view %u!\n", view);
	}

#pragma endregion

#pragma region SETUP

	
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

	//creates swapchain and its images
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
		Logger::LogMessage("Created Swapchain!");

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