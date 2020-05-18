#include "vkutils.h"
#include "glfw3.h"
#include <assert.h>
#include <vector>
#include "Logger/Logger.h"
#include "Span.h"
#include "Optional.h"
#include <string>
#include <set>

#pragma warning(disable : 4100)
#pragma warning(disable : 4702)

#define VK_CHECK(vkOp) 				  														\
{									  														\
	VkResult result = vkOp; 		  														\
	if(result!=VK_SUCCESS)				  														\
	{ 								  														\
		Logger::LogErrorFormatted("Vulkan result was not VK_SUCCESS! Code : %u\n", result);	\
	} 								  														\
	assert(result == VK_SUCCESS); 	  														\
	(void*)result;					  														\
}									  


namespace {

#ifdef NDEBUG
	constexpr bool enableValidationLayers = false;
#else
	constexpr bool enableValidationLayers = true;
#endif
	constexpr unsigned int validationLayersCount = 1;
	constexpr const char *validationLayers[validationLayersCount] = { "VK_LAYER_KHRONOS_validation" };

	static const unsigned int deviceExtensionsCount = 4;
	const char *deviceExtensions[deviceExtensionsCount] = { "VK_KHR_swapchain", "VK_KHR_ray_tracing", "VK_KHR_pipeline_library", "VK_KHR_deferred_host_operations" };

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

#pragma region PHYSICAL_DEVICE

	struct QueueFamilyIndices
	{
		Optional<uint32_t> graphicsFamily;
		Optional<uint32_t> presentFamily;

		bool isComplete() {
			return graphicsFamily.isSet() && presentFamily.isSet();
		}
	};

	QueueFamilyIndices findQueueFamilies(VkPhysicalDevice device) {
		QueueFamilyIndices indices;

		uint32_t queueFamilyCount = 0;
		vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, nullptr);

		std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
		vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, queueFamilies.data());

		for (unsigned int i = 0; i < queueFamilyCount; i++) {
			VkQueueFamilyProperties queueFamily = queueFamilies[i];

			VkBool32 presentSupport = false;
			vkGetPhysicalDeviceSurfaceSupportKHR(device, i, vkut::surface, &presentSupport);
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
		unsigned int formatsCount = 0;
		VkSurfaceFormatKHR *formats = nullptr;
		unsigned int presentModesCount = 0;
		VkPresentModeKHR *presentModes = nullptr;
	};

	SwapChainSupportDetails querySwapChainSupport(VkPhysicalDevice physicalDevice)
	{
		SwapChainSupportDetails details;

		vkGetPhysicalDeviceSurfaceCapabilitiesKHR(physicalDevice, vkut::surface, &details.capabilities);

		uint32_t formatCount;
		vkGetPhysicalDeviceSurfaceFormatsKHR(physicalDevice, vkut::surface, &formatCount, nullptr);

		if (formatCount != 0) {
			details.formatsCount = formatCount;
			details.formats = new VkSurfaceFormatKHR[formatCount];
			vkGetPhysicalDeviceSurfaceFormatsKHR(physicalDevice, vkut::surface, &formatCount, details.formats);
		}

		uint32_t presentModeCount;
		vkGetPhysicalDeviceSurfacePresentModesKHR(physicalDevice, vkut::surface, &presentModeCount, nullptr);

		if (presentModeCount != 0) {
			details.presentModesCount = presentModeCount;
			details.presentModes = new VkPresentModeKHR[presentModeCount];
			vkGetPhysicalDeviceSurfacePresentModesKHR(physicalDevice, vkut::surface, &presentModeCount, details.presentModes);
		}

		return details;
	}

	bool checkDeviceExtensionSupport(VkPhysicalDevice physicalDevice) {

		uint32_t availableExtensionCount = 0;
		vkEnumerateDeviceExtensionProperties(physicalDevice, nullptr, &availableExtensionCount, nullptr);

		std::vector<VkExtensionProperties> availableExtensions(availableExtensionCount);
		vkEnumerateDeviceExtensionProperties(physicalDevice, nullptr, &availableExtensionCount, availableExtensions.data());

		std::vector<std::string> requiredExtensions;
		for (unsigned int i = 0; i < deviceExtensionsCount; i++)
		{
			requiredExtensions.push_back(deviceExtensions[i]);
		}

		for (unsigned int i = 0; i < deviceExtensionsCount; i++)
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

	bool isPhysicalDeviceSuitable(VkPhysicalDevice physicalDevice)
	{
		QueueFamilyIndices indices = findQueueFamilies(physicalDevice);
		bool extensionsSupported = checkDeviceExtensionSupport(physicalDevice);

		bool swapChainAdequate = false;
		if (extensionsSupported) {
			SwapChainSupportDetails swapChainSupport = querySwapChainSupport(physicalDevice);
			swapChainAdequate = swapChainSupport.formatsCount != 0 && swapChainSupport.presentModesCount != 0;
		}

		VkPhysicalDeviceFeatures supportedFeatures;
		vkGetPhysicalDeviceFeatures(physicalDevice, &supportedFeatures);

		return indices.isComplete() && extensionsSupported && swapChainAdequate && supportedFeatures.samplerAnisotropy;
	}

#pragma endregion

}

void vkut::choosePhysicalDevice()
{
	uint32_t deviceCount = 0;
	vkEnumeratePhysicalDevices(vkut::instance, &deviceCount, nullptr);

	assert(deviceCount != 0);

	std::vector<VkPhysicalDevice> devices(deviceCount);
	vkEnumeratePhysicalDevices(vkut::instance, &deviceCount, devices.data());

	for (unsigned int i = 0; i < deviceCount; i++) {
		VkPhysicalDevice dev = devices[i];
		if (isPhysicalDeviceSuitable(dev)) {
			Logger::LogMessage("Chose physical device!");
			vkut::physicalDevice = dev;
			return;
		}
	}

	assert(false);
}

void vkut::createLogicalDevice()
{
	QueueFamilyIndices indices = findQueueFamilies(vkut::physicalDevice);


	std::vector<VkDeviceQueueCreateInfo> queueCreateInfos;

	std::set<uint32_t> uniqueQueueFamilies = { indices.graphicsFamily.getValue(), indices.presentFamily.getValue() };

	float queuePriority = 1.0f;
	for (uint32_t queueFamily : uniqueQueueFamilies)
	{
		VkDeviceQueueCreateInfo queueCreateInfo = {};

		queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
		queueCreateInfo.queueFamilyIndex = queueFamily;
		queueCreateInfo.queueCount = 1;
		queueCreateInfo.pQueuePriorities = &queuePriority;
		queueCreateInfos.push_back(queueCreateInfo);
	}


	VkPhysicalDeviceFeatures deviceFeatures = {};
	deviceFeatures.samplerAnisotropy = VK_TRUE;

	VkDeviceCreateInfo createInfo = {};
	createInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
	createInfo.queueCreateInfoCount = static_cast<uint32_t>(queueCreateInfos.size());
	createInfo.pQueueCreateInfos = queueCreateInfos.data();
	createInfo.pEnabledFeatures = &deviceFeatures;
	createInfo.enabledExtensionCount = static_cast<uint32_t>(deviceExtensionsCount);
	createInfo.ppEnabledExtensionNames = deviceExtensions;

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

void vkut::destroyLogicalDevice() {
	vkDestroyDevice(vkut::device, nullptr);
	Logger::LogMessage("Destroyed logical device!");
}

void vkut::createSurface(GLFWwindow *window)
{
	VK_CHECK(glfwCreateWindowSurface(vkut::instance, window, nullptr, &vkut::surface));
	Logger::LogMessage("Created surface!");
}

void vkut::destroySurface() 
{
	vkDestroySurfaceKHR(vkut::instance, vkut::surface, nullptr);
	Logger::LogMessage("Destroyed surface!");
}

void vkut::createInstance(const char *title) {

	assert(enableValidationLayers && checkValidationLayerSupport());


	VkApplicationInfo appInfo = {
		.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO,
		.pApplicationName = "Raytracer!",
		.applicationVersion = VK_MAKE_VERSION(1, 0, 0),
		.pEngineName = "No Engine",
		.engineVersion = VK_MAKE_VERSION(1, 0, 0),
		.apiVersion = VK_API_VERSION_1_2
	};

	VkInstanceCreateInfo createInfo = {};
	createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
	createInfo.pApplicationInfo = &appInfo;

	VkDebugUtilsMessengerCreateInfoEXT debugCreateInfo;
	if constexpr (enableValidationLayers) {
		createInfo.enabledLayerCount = static_cast<uint32_t>(validationLayersCount);
		createInfo.ppEnabledLayerNames = validationLayers;

		populateDebugMessengerCreateInfo(debugCreateInfo);
		createInfo.pNext = (VkDebugUtilsMessengerCreateInfoEXT *)&debugCreateInfo;
	}
	else {
		createInfo.enabledLayerCount = 0;
		createInfo.pNext = nullptr;
	}


	std::vector<const char *> glfwExtensions = getRequiredExtensions();
	createInfo.enabledExtensionCount = static_cast<uint32_t>(glfwExtensions.size());
	createInfo.ppEnabledExtensionNames = glfwExtensions.data();

	VK_CHECK(vkCreateInstance(&createInfo, nullptr, &vkut::instance));
	Logger::LogMessage("Created instance!");
}

void vkut::destroyInstance() {
	vkDestroyInstance(vkut::instance, nullptr);
	Logger::LogMessage("Destroyed instance!");
}

void vkut::createDebugMessenger()
{
	if constexpr (!enableValidationLayers) return;

	VkDebugUtilsMessengerCreateInfoEXT createInfo;
	populateDebugMessengerCreateInfo(createInfo);
	assert(CreateDebugUtilsMessengerEXT(vkut::instance, &createInfo, nullptr, &debugMessenger) == VK_SUCCESS);

	Logger::LogMessage("Created debug messenger!");
}

void vkut::destroyDebugMessenger() {
	if constexpr (enableValidationLayers) {
		DestroyDebugUtilsMessengerEXT(vkut::instance, debugMessenger, nullptr);
		Logger::LogMessage("Destroyed debug messenger!");
	}
}

GLFWwindow *vkut::createWindow(const char *title, int width, int height) {
	glfwInit();
	glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
	GLFWwindow *window = glfwCreateWindow(width, height, title, NULL, NULL);
	assert(window != NULL);
	Logger::LogMessage("Created window!");
	return window;
}

void vkut::destroyWindow(GLFWwindow *window) {
	glfwDestroyWindow(window);
	glfwTerminate();
	Logger::LogMessage("Destroyed window!");
}