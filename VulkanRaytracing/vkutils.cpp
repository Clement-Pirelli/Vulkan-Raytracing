#include "vkutils.h"
#include "glfw3.h"
#include <assert.h>
#include "Logger/Logger.h"
#include <string>
#include <set>
#include "Files.h"

#pragma warning(disable : 4100)
#pragma warning(disable : 4702)

namespace vkut {
	namespace {

#pragma region SMALL_UTILITIES
		template<typename T>
		void setFunctionPointer(T &pointer, const char *name)
		{
			pointer = reinterpret_cast<T>(vkGetDeviceProcAddr(vkut::device, name));
		}

#define VK_SET_FUNC_PTR(func) (setFunctionPointer(func, #func))


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
#pragma endregion

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
			Logger::logWarningFormatted("[validation]: %s ", pCallbackData->pMessage);
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

			Logger::logError("Couldn't find format!");
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

			Logger::logError("Failed to find suitable memory type!");
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

		uint64_t GetBufferAddress(VkBuffer buffer)
		{

			static PFN_vkGetBufferDeviceAddressKHR vkGetBufferDeviceAddressKHR = nullptr;

			if (vkGetBufferDeviceAddressKHR == nullptr)
				VK_SET_FUNC_PTR(vkGetBufferDeviceAddressKHR);

			VkBufferDeviceAddressInfoKHR bufferAddressInfo
			{
				.sType = VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO,
				.pNext = nullptr,
				.buffer = buffer,
			};

			return vkGetBufferDeviceAddressKHR(device, &bufferAddressInfo);
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


	namespace common {

		VkShaderModule createShaderModule(const char *filePath)
		{
			std::vector<char> code = {};
			
			{
				FileReader reader = FileReader(std::string(filePath));
				size_t size = reader.length();
				code.resize(size);
				reader.read(code.data(), code.size());
			}
			
			VkShaderModuleCreateInfo createInfo
			{
				.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
				.codeSize = code.size(),
				.pCode = reinterpret_cast<const uint32_t *>(code.data())
			};

			VkShaderModule shaderModule;
			VK_CHECK(vkCreateShaderModule(vkut::device, &createInfo, nullptr, &shaderModule));
			Logger::logTrivialFormatted("Created shader module %u!", shaderModule);

			return shaderModule;
		}

		void destroyShaderModule(VkShaderModule shaderModule)
		{
			vkDestroyShaderModule(device, shaderModule, nullptr);
			Logger::logTrivialFormatted("Destroyed shader module %u!", shaderModule);
		}


		Buffer createBuffer(VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags propertyFlags, VkMemoryAllocateFlagsInfo flagsInfo)
		{
			Buffer buffer 
			{
				.size = size
			};

			VkBufferCreateInfo bufferInfo
			{
				.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
				.size = size,
				.usage = usage,
				.sharingMode = VK_SHARING_MODE_EXCLUSIVE
			};
			VK_CHECK(vkCreateBuffer(vkut::device, &bufferInfo, nullptr, &buffer.buffer));
			Logger::logTrivialFormatted("Created buffer %u! ", buffer);

			VkMemoryRequirements memRequirements;
			vkGetBufferMemoryRequirements(vkut::device, buffer.buffer, &memRequirements);

			VkMemoryAllocateInfo allocInfo
			{
				.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
				.allocationSize = memRequirements.size,
				.memoryTypeIndex = findMemoryType(memRequirements.memoryTypeBits, propertyFlags)
			};

			if (flagsInfo.sType == VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_FLAGS_INFO) 
			{
				allocInfo.pNext = &flagsInfo;
			}

			VK_CHECK(vkAllocateMemory(vkut::device, &allocInfo, nullptr, &buffer.memory));
			Logger::logTrivialFormatted("Allocated buffer memory %u! ", buffer.memory);

			VK_CHECK(vkBindBufferMemory(vkut::device, buffer.buffer, buffer.memory, 0));
			
			return buffer;
		}

		void destroyBuffer(const Buffer &buffer)
		{
			vkDestroyBuffer(vkut::device, buffer.buffer, nullptr);
			vkFreeMemory(vkut::device, buffer.memory, nullptr);
			Logger::logTrivialFormatted("Destroyed buffer %u! ", buffer.buffer);
			Logger::logTrivialFormatted("Freed buffer memory %u! ", buffer.memory);
		}

		//assumes one descriptor per swapchain image
		VkDescriptorPool createDescriptorPool(const std::vector<VkDescriptorType> &descriptorTypes)
		{
			std::vector<VkDescriptorPoolSize> poolSizes = std::vector<VkDescriptorPoolSize>(descriptorTypes.size());
			for(size_t i = 0; i < poolSizes.size(); i++)
			{
				VkDescriptorPoolSize poolSize
				{
					.type = descriptorTypes[i],
					.descriptorCount = static_cast<uint32_t>(swapChainImages.size())
				};
				poolSizes[i] = poolSize;
			}

			VkDescriptorPoolCreateInfo poolInfo
			{
				.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO,
				.maxSets = static_cast<uint32_t>(swapChainImages.size()),
				.poolSizeCount = static_cast<uint32_t>(poolSizes.size()),
				.pPoolSizes = poolSizes.data(),
			};

			VkDescriptorPool descriptorPool = {};
			VK_CHECK(vkCreateDescriptorPool(device, &poolInfo, nullptr, &descriptorPool));
			Logger::logMessageFormatted("Created descriptor pool %u! ", descriptorPool);
			return descriptorPool;
		}

		//implicitly destroys all descriptor sets from this pool
		void destroyDescriptorPool(VkDescriptorPool descriptorPool)
		{
			vkDestroyDescriptorPool(vkut::device, descriptorPool, nullptr);
			Logger::logMessageFormatted("Destroyed descriptor pool %u! ", descriptorPool);
		}
		
		VkDescriptorSet createDescriptorSet(VkDescriptorSetLayout descriptorSetLayout, VkDescriptorPool descriptorPool, const std::vector<DescriptorSetInfo> &descriptorSetInfos)
		{
			VkDescriptorSetAllocateInfo allocInfo
			{
				.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
				.descriptorPool = descriptorPool,
				.descriptorSetCount = 1,
				.pSetLayouts = &descriptorSetLayout
			};

			VkDescriptorSet descriptorSet = {};
			
			VK_CHECK(vkAllocateDescriptorSets(device, &allocInfo, &descriptorSet));
			Logger::logMessageFormatted("Allocated descriptor set %u!", descriptorSet);

			std::vector<VkWriteDescriptorSet> descriptorWrites = std::vector<VkWriteDescriptorSet>(descriptorSetInfos.size());
			for (size_t i = 0; i < descriptorWrites.size(); i++)
			{
				const DescriptorSetInfo &info = descriptorSetInfos[i];

				VkWriteDescriptorSet descriptorWrite
				{
					.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
					.pNext = info.pNext,
					.dstSet = descriptorSet,
					.dstBinding = info.dstBinding,
					.dstArrayElement = info.dstArrayElement,
					.descriptorCount = info.descriptorCount,
					.descriptorType = info.descriptorType,
					.pImageInfo = info.pImageInfo,
					.pBufferInfo = info.pBufferInfo,
					.pTexelBufferView = info.pTexelBufferView,
				};

				descriptorWrites[i] = descriptorWrite;
			}

			vkUpdateDescriptorSets(device, static_cast<uint32_t>(descriptorWrites.size()), descriptorWrites.data(), 0, nullptr);
			Logger::logMessageFormatted("Updated descriptor set %u! ", descriptorSet);
			
			return descriptorSet;
		}

		VkDescriptorSetLayout createDescriptorSetLayout(const std::vector<VkDescriptorSetLayoutBinding> &bindings)
		{
			VkDescriptorSetLayoutCreateInfo layoutInfo = {};
			layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
			layoutInfo.bindingCount = static_cast<uint32_t>(bindings.size());
			layoutInfo.pBindings = bindings.data();

			VkDescriptorSetLayout descriptorSetLayout = {};
			VK_CHECK(vkCreateDescriptorSetLayout(device, &layoutInfo, nullptr, &descriptorSetLayout));

			Logger::logMessageFormatted("Created descriptor set layout %u! ", descriptorSetLayout);

			return descriptorSetLayout;
		}

		void destroyDescriptorSetLayout(VkDescriptorSetLayout descriptorSetLayout)
		{
			vkDestroyDescriptorSetLayout(vkut::device, descriptorSetLayout, nullptr);

			Logger::logMessageFormatted("Destroyed descriptor set layout %u! ", descriptorSetLayout);
		}


		void transitionImageLayout(
			VkCommandPool commandPool, 
			VkImage image,
			VkImageLayout oldLayout,
			VkImageLayout newLayout,
			VkImageSubresourceRange subresourceRange,
			VkPipelineStageFlags sourceStage,
			VkPipelineStageFlags destinationStage)
		{
			VkCommandBuffer commandBuffer = initSingleTimeCommands(commandPool);

			transitionImageLayout(commandBuffer, image, oldLayout, newLayout, subresourceRange, sourceStage, destinationStage);

			submitSingleTimeCommands(commandPool, commandBuffer);
		}

		//taken from : https://github.com/SaschaWillems/Vulkan/blob/master/base/VulkanTools.cpp
		void transitionImageLayout(
			VkCommandBuffer commandBuffer, 
			VkImage image,
			VkImageLayout oldLayout, 
			VkImageLayout newLayout, 
			VkImageSubresourceRange subresourceRange,
			VkPipelineStageFlags sourceStage,
			VkPipelineStageFlags destinationStage)
		{
			VkImageMemoryBarrier barrier
			{
				.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
				.oldLayout = oldLayout,
				.newLayout = newLayout,

				.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
				.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,

				.image = image,
				.subresourceRange = subresourceRange
			};

			switch (oldLayout)
			{
			case VK_IMAGE_LAYOUT_UNDEFINED:
				// Image layout is undefined (or does not matter)
				// Only valid as initial layout
				// No flags required, listed only for completeness
				barrier.srcAccessMask = 0;
				break;

			case VK_IMAGE_LAYOUT_PREINITIALIZED:
				// Image is preinitialized
				// Only valid as initial layout for linear images, preserves memory contents
				// Make sure host writes have been finished
				barrier.srcAccessMask = VK_ACCESS_HOST_WRITE_BIT;
				break;

			case VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL:
				// Image is a color attachment
				// Make sure any writes to the color buffer have been finished
				barrier.srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
				break;

			case VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL:
				// Image is a depth/stencil attachment
				// Make sure any writes to the depth/stencil buffer have been finished
				barrier.srcAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
				break;

			case VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL:
				// Image is a transfer source 
				// Make sure any reads from the image have been finished
				barrier.srcAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
				break;

			case VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL:
				// Image is a transfer destination
				// Make sure any writes to the image have been finished
				barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
				break;

			case VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL:
				// Image is read by a shader
				// Make sure any shader reads from the image have been finished
				barrier.srcAccessMask = VK_ACCESS_SHADER_READ_BIT;
				break;
			default:
				// Other source layouts aren't handled (yet)
				
				break;
			}

			// Target layouts (new)
			// Destination access mask controls the dependency for the new image layout
			switch (newLayout)
			{
			case VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL:
				// Image will be used as a transfer destination
				// Make sure any writes to the image have been finished
				barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
				break;

			case VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL:
				// Image will be used as a transfer source
				// Make sure any reads from the image have been finished
				barrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
				break;

			case VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL:
				// Image will be used as a color attachment
				// Make sure any writes to the color buffer have been finished
				barrier.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
				break;

			case VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL:
				// Image layout will be used as a depth/stencil attachment
				// Make sure any writes to depth/stencil buffer have been finished
				barrier.dstAccessMask = barrier.dstAccessMask | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
				break;

			case VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL:
				// Image will be read in a shader (sampler, input attachment)
				// Make sure any writes to the image have been finished
				if (barrier.srcAccessMask == 0)
				{
					barrier.srcAccessMask = VK_ACCESS_HOST_WRITE_BIT | VK_ACCESS_TRANSFER_WRITE_BIT;
				}
				barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
				break;
			default:
				// Other source layouts aren't handled (yet)
				
				break;
			}

			vkCmdPipelineBarrier(
				commandBuffer,
				sourceStage,
				destinationStage,
				0,
				0, nullptr,
				0, nullptr,
				1, &barrier
			);
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

			Logger::logMessageFormatted("Created image %u with memory %u! ", image.image, image.memory);
			return image;
		}

		void destroyImage(Image image)
		{
			vkDestroyImage(vkut::device, image.image, nullptr);
			vkFreeMemory(vkut::device, image.memory, nullptr);
			Logger::logMessageFormatted("Destroyed image %u with memory %u! ", image.image, image.memory);
		}


		VkFormat findDepthFormat()
		{
			return findSupportedFormat(
				std::vector<VkFormat> { VK_FORMAT_D32_SFLOAT, VK_FORMAT_D32_SFLOAT_S8_UINT, VK_FORMAT_D24_UNORM_S8_UINT },
				VK_IMAGE_TILING_OPTIMAL,
				VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT
			);
		}


		VkPipelineLayout createPipelineLayout(std::vector<VkDescriptorSetLayout> descriptorSetLayouts)
		{
			VkPipelineLayout pipelineLayout = {};

			VkPipelineLayoutCreateInfo pipelineLayoutCreateInfo
			{
				.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
				.setLayoutCount = static_cast<uint32_t>(descriptorSetLayouts.size()),
				.pSetLayouts = descriptorSetLayouts.data()
			};
			vkCreatePipelineLayout(vkut::device, &pipelineLayoutCreateInfo, nullptr, &pipelineLayout);
			Logger::logMessageFormatted("Created pipeline layout %u! ", pipelineLayout);
			return pipelineLayout;
		}
		
		void destroyPipelineLayout(VkPipelineLayout pipelineLayout)
		{
			vkDestroyPipelineLayout(vkut::device, pipelineLayout, nullptr);
			Logger::logMessageFormatted("Destroyed pipeline layout %u! ", pipelineLayout);
		}

		void destroyPipeline(VkPipeline pipeline)
		{
			vkDestroyPipeline(vkut::device, pipeline, nullptr);
			Logger::logMessageFormatted("Destroyed pipeline %u! ", pipeline);
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
						.layout = colorDescriptions[i].finalLayout,
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
			Logger::logMessageFormatted("Created render pass %u! ", renderPass);

			return renderPass;
		}

		void destroyRenderPass(VkRenderPass renderPass)
		{
			vkDestroyRenderPass(vkut::device, renderPass, nullptr);
			Logger::logMessageFormatted("Destroyed render pass %u! ", renderPass);
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
			Logger::logMessageFormatted("Created image view %u! ", returnImageView);

			return returnImageView;
		}

		void destroyImageView(VkImageView view)
		{
			vkDestroyImageView(vkut::device, view, nullptr);
			Logger::logMessageFormatted("Destroyed image view %u! ", view);
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

			Logger::logMessageFormatted("Created %u command buffers! ", amount);

			return commandBuffers;
		}

		void destroyCommandBuffers(VkCommandPool commandPool, const std::vector<VkCommandBuffer> &commandBuffers)
		{
			vkFreeCommandBuffers(vkut::device, commandPool, static_cast<uint32_t>(commandBuffers.size()), commandBuffers.data());
			Logger::logMessageFormatted("Destroyed %u command buffers! ", commandBuffers.size());
		}

		void startRecordCommandBuffer(VkCommandBuffer commandBuffer)
		{
			VkCommandBufferBeginInfo beginInfo
			{
				.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
				.flags = VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT
			};
			VK_CHECK(vkBeginCommandBuffer(commandBuffer, &beginInfo));
		}

		void endRecordCommandBuffer(VkCommandBuffer commandBuffer)
		{
			VK_CHECK(vkEndCommandBuffer(commandBuffer));
		}
	}

	namespace setup {
	
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

			Logger::logMessage("Created sync objects!");
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
			Logger::logMessage("Destroyed sync objects!");
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
			Logger::logMessageFormatted("Created framebuffer %u with renderpass %u! ", framebuffer, renderPass);
			return framebuffer;
		}

		void destroyFramebuffer(VkFramebuffer framebuffer)
		{
			vkDestroyFramebuffer(vkut::device, framebuffer, nullptr);
			Logger::logMessageFormatted("Destroyed framebuffer %u! ", framebuffer);
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

			Logger::logMessageFormatted("Created graphics command pool %u! ", commandPool);

			return commandPool;
		}

		void destroyCommandPool(VkCommandPool commandPool)
		{
			vkDestroyCommandPool(vkut::device, commandPool, nullptr);
			Logger::logMessageFormatted("Destroyed command pool %u! ", commandPool);
		}

		void createSwapchainImageViews()
		{
			vkut::swapChainImageViews.resize(swapChainImages.size());
			for (size_t i = 0; i < swapChainImageViews.size(); i++)
			{
				swapChainImageViews[i] = common::createImageView(swapChainImages[i], swapChainImageFormat, VK_IMAGE_ASPECT_COLOR_BIT, 1);
			}
			Logger::logMessage("Created swapchain image views!");
		}

		void destroySwapchainImageViews()
		{
			for (size_t i = 0; i < swapChainImageViews.size(); i++)
			{
				common::destroyImageView(swapChainImageViews[i]);
			}
			Logger::logMessage("Destroyed swapchain image views!");
		}

		void createSwapChain(uint32_t width, uint32_t height, VkImageUsageFlags flags)
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
				.imageUsage = flags,
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
			Logger::logMessage("Created swapchain!");

			vkGetSwapchainImagesKHR(vkut::device, vkut::swapChain, &imageCount, nullptr);
			swapChainImages.resize(imageCount);
			vkGetSwapchainImagesKHR(vkut::device, vkut::swapChain, &imageCount, vkut::swapChainImages.data());

			vkut::swapChainImageFormat = surfaceFormat.format;
			vkut::swapChainExtent = extent;
		}

		void destroySwapChain()
		{
			vkDestroySwapchainKHR(device, swapChain, nullptr);
			Logger::logMessage("Destroyed swapchain!");
		}

		void createLogicalDevice(void *pNext)
		{
			QueueFamilyIndices indices = findQueueFamilies(vkut::physicalDevice);


			std::vector<VkDeviceQueueCreateInfo> queueCreateInfos;

			std::set<uint32_t> uniqueQueueFamilies = { indices.graphicsFamily.getValue(), indices.presentFamily.getValue() };

			float queuePriority = 1.0f;
			for (uint32_t queueFamily : uniqueQueueFamilies)
			{
				VkDeviceQueueCreateInfo queueCreateInfo{
					.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
					.queueFamilyIndex = queueFamily,
					.queueCount = 1,
					.pQueuePriorities = &queuePriority
				};

				queueCreateInfos.push_back(queueCreateInfo);
			}


			VkPhysicalDeviceFeatures deviceFeatures
			{
				.samplerAnisotropy = VK_TRUE
			};

			VkPhysicalDeviceBufferDeviceAddressFeatures addressFeatures
			{
				.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_BUFFER_DEVICE_ADDRESS_FEATURES,
				.pNext = pNext,
				.bufferDeviceAddress = VK_TRUE,
			};

			VkPhysicalDeviceFeatures2 deviceFeatures2
			{
				.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2,
				.pNext = &addressFeatures,
				.features = deviceFeatures,
			};

			//While creating the device, also don't set VkDeviceCreateInfo::pEnabledFeatures. 
			//Fill in VkPhysicalDeviceFeatures2 structure instead and pass it as VkDeviceCreateInfo::pNext. 
			
			//Enable this device feature - attach additional structure VkPhysicalDeviceBufferDeviceAddressFeatures* 
			//to VkPhysicalDeviceFeatures2::pNext and set its member bufferDeviceAddress to VK_TRUE.
			
			VkDeviceCreateInfo createInfo
			{
				.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,
				.pNext = (void*)&deviceFeatures2,
				.queueCreateInfoCount = static_cast<uint32_t>(queueCreateInfos.size()),
				.pQueueCreateInfos = queueCreateInfos.data(),
				.enabledExtensionCount = static_cast<uint32_t>(deviceExtensions.size()),
				.ppEnabledExtensionNames = deviceExtensions.data(),
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
			Logger::logMessage("Created logical device!");

			vkGetDeviceQueue(vkut::device, indices.graphicsFamily.getValue(), 0, &vkut::graphicsQueue);
			vkGetDeviceQueue(vkut::device, indices.presentFamily.getValue(), 0, &vkut::presentQueue);
		}

		void destroyLogicalDevice()
		{
			vkDestroyDevice(vkut::device, nullptr);
			Logger::logMessage("Destroyed logical device!");
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
					Logger::logMessage("Chose physical device!");
					vkut::physicalDevice = dev;
					return;
				}
			}

			assert(false);
		}

		void createSurface(GLFWwindow *window)
		{
			VK_CHECK(glfwCreateWindowSurface(vkut::instance, window, nullptr, &vkut::surface));
			Logger::logMessage("Created surface!");
		}

		void destroySurface()
		{
			vkDestroySurfaceKHR(vkut::instance, vkut::surface, nullptr);
			Logger::logMessage("Destroyed surface!");
		}

		void createDebugMessenger()
		{
			if constexpr (!enableValidationLayers)
				return;

			VkDebugUtilsMessengerCreateInfoEXT createInfo;
			populateDebugMessengerCreateInfo(createInfo);
			assert(CreateDebugUtilsMessengerEXT(vkut::instance, &createInfo, nullptr, &debugMessenger) == VK_SUCCESS);

			Logger::logMessage("Created debug messenger!");
		}

		void destroyDebugMessenger() {
			if constexpr (enableValidationLayers)
			{
				DestroyDebugUtilsMessengerEXT(vkut::instance, debugMessenger, nullptr);
				Logger::logMessage("Destroyed debug messenger!");
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

			VkInstanceCreateInfo createInfo
			{
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
			Logger::logMessage("Created instance!");
		}

		void destroyInstance()
		{
			vkDestroyInstance(vkut::instance, nullptr);
			Logger::logMessage("Destroyed instance!");
		}

		GLFWwindow *createWindow(const char *title, int width, int height)
		{
			glfwInit();
			glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
			GLFWwindow *window = glfwCreateWindow(width, height, title, NULL, NULL);
			assert(window != NULL);
			Logger::logMessage("Created window!");
			return window;
		}

		void destroyWindow(GLFWwindow *window)
		{
			glfwDestroyWindow(window);
			glfwTerminate();
			Logger::logMessage("Destroyed window!");
		}
	}

	namespace raytracing {

		void initRaytracingFunctions()
		{
			
			VK_SET_FUNC_PTR(vkCreateAccelerationStructureKHR);
			VK_SET_FUNC_PTR(vkDestroyAccelerationStructureKHR);
			VK_SET_FUNC_PTR(vkBindAccelerationStructureMemoryKHR);
			VK_SET_FUNC_PTR(vkGetAccelerationStructureMemoryRequirementsKHR);
			VK_SET_FUNC_PTR(vkCmdBuildAccelerationStructureKHR);
			VK_SET_FUNC_PTR(vkCreateRayTracingPipelinesKHR);
			VK_SET_FUNC_PTR(vkGetRayTracingShaderGroupHandlesKHR);
			VK_SET_FUNC_PTR(vkCmdTraceRaysKHR);
			VK_SET_FUNC_PTR(vkGetAccelerationStructureDeviceAddressKHR);

		}

		void getPhysicalDeviceRaytracingProperties() 
		{
			physicalDeviceRaytracingProperties.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_RAY_TRACING_PROPERTIES_NV;
			VkPhysicalDeviceProperties2 deviceProps2{};
			deviceProps2.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROPERTIES_2;
			deviceProps2.pNext = &physicalDeviceRaytracingProperties;
			vkGetPhysicalDeviceProperties2(vkut::physicalDevice, &deviceProps2);
		}

		VkMemoryRequirements getAccelerationStructureMemoryRequirements(VkAccelerationStructureKHR acceleration, VkAccelerationStructureMemoryRequirementsTypeKHR type)
		{
			VkMemoryRequirements2 memoryRequirements2;
			memoryRequirements2.sType = VK_STRUCTURE_TYPE_MEMORY_REQUIREMENTS_2;
			memoryRequirements2.pNext = nullptr;

			VkAccelerationStructureMemoryRequirementsInfoKHR accelerationMemoryRequirements
			{
				accelerationMemoryRequirements.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_MEMORY_REQUIREMENTS_INFO_KHR,
				accelerationMemoryRequirements.pNext = nullptr,
				accelerationMemoryRequirements.type = type,
				accelerationMemoryRequirements.buildType = VK_ACCELERATION_STRUCTURE_BUILD_TYPE_DEVICE_KHR,
				accelerationMemoryRequirements.accelerationStructure = acceleration,
			};

			vkGetAccelerationStructureMemoryRequirementsKHR(device, &accelerationMemoryRequirements, &memoryRequirements2);

			return memoryRequirements2.memoryRequirements;
		}

		MappedBuffer createAccelerationScratchBuffer(VkAccelerationStructureKHR acceleration, VkAccelerationStructureMemoryRequirementsTypeKHR type)
		{
			VkMemoryRequirements asRequirements = getAccelerationStructureMemoryRequirements(acceleration, type);

			VkMemoryAllocateFlagsInfo memAllocFlagsInfo
			{
				.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_FLAGS_INFO,
				.pNext = nullptr,
				.flags = VK_MEMORY_ALLOCATE_DEVICE_ADDRESS_BIT_KHR,
				.deviceMask = 0
			};

			Buffer BLASbuffer = vkut::common::createBuffer(
				asRequirements.size, 
				VK_BUFFER_USAGE_RAY_TRACING_BIT_KHR | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT, 
				VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
				memAllocFlagsInfo);

			MappedBuffer mappedBuffer
			{
				.buffer = BLASbuffer.buffer,
				.memory = BLASbuffer.memory,
			};

			mappedBuffer.memoryAddress = GetBufferAddress(mappedBuffer.buffer);
			
			return mappedBuffer;
		}

		template<typename T>
		MappedBuffer createMappedBuffer(const std::vector<T> &data) {
			
			uint32_t byteLength = static_cast<uint32_t>( data.size() * sizeof(T));

			VkMemoryAllocateFlagsInfo memAllocFlagsInfo
			{
				.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_FLAGS_INFO,
				.pNext = nullptr,
				.flags = VK_MEMORY_ALLOCATE_DEVICE_ADDRESS_BIT_KHR,
				.deviceMask = 0
			};

			Buffer buffer = vkut::common::createBuffer(
				byteLength,
				VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT, 
				VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
				memAllocFlagsInfo);


			MappedBuffer mappedBuffer
			{
				.buffer = buffer.buffer,
				.memory = buffer.memory
			};

			mappedBuffer.memoryAddress = GetBufferAddress(mappedBuffer.buffer);

			void *dstData;
			VK_CHECK(vkMapMemory(vkut::device, mappedBuffer.memory, 0, byteLength, 0, &dstData));

			memcpy(dstData, (void *)data.data(), byteLength);
			
			vkUnmapMemory(device, mappedBuffer.memory);
			mappedBuffer.mappedPointer = dstData;

			return mappedBuffer;
		}
		
		void destroyMappedBuffer(MappedBuffer mappedBuffer)
		{
			vkut::common::destroyBuffer({ mappedBuffer.buffer, mappedBuffer.memory });
		}

		void BindAccelerationMemory(VkAccelerationStructureKHR acceleration, VkDeviceMemory memory)
		{
			VkBindAccelerationStructureMemoryInfoKHR accelerationMemoryBindInfo
			{
				.sType = VK_STRUCTURE_TYPE_BIND_ACCELERATION_STRUCTURE_MEMORY_INFO_KHR,
				.pNext = nullptr,
				.accelerationStructure = acceleration,
				.memory = memory,
				.memoryOffset = 0,
				.deviceIndexCount = 0,
				.pDeviceIndices = nullptr,
			};

			VK_CHECK(vkBindAccelerationStructureMemoryKHR(device, 1, &accelerationMemoryBindInfo));
		}

		VkAccelerationStructureKHR createAccelerationStructure(VkAccelerationStructureCreateGeometryTypeInfoKHR geometryTypeInfo, VkAccelerationStructureTypeKHR type)
		{
			VkAccelerationStructureCreateInfoKHR accelerationInfo
			{
				.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_CREATE_INFO_KHR,
				.pNext = nullptr,
				.compactedSize = 0,
				.type = type,
				.flags = VK_BUILD_ACCELERATION_STRUCTURE_PREFER_FAST_TRACE_BIT_KHR,
				.maxGeometryCount = 1,
				.pGeometryInfos = &geometryTypeInfo,
				.deviceAddress = VK_NULL_HANDLE,
			};

			VkAccelerationStructureKHR accelerationStructure = {};
			
			VK_CHECK(vkCreateAccelerationStructureKHR(device, &accelerationInfo, nullptr, &accelerationStructure));
			Logger::logMessageFormatted("Created acceleration structure %u! ", accelerationStructure);
			
			return accelerationStructure;
		}

		BottomLevelAccelerationStructure createBLAS(VkCommandPool commandPool, const std::vector<float> &vertices, const std::vector<uint32_t> &indices)
		{
			VkAccelerationStructureKHR accelerationStructure = {};
			MappedBuffer accelerationMappedBuffer = {};
			uint64_t address = {};
			
			VkAccelerationStructureCreateGeometryTypeInfoKHR accelerationCreateGeometryInfo
			{
				.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_CREATE_GEOMETRY_TYPE_INFO_KHR,
				.pNext = nullptr,
				.geometryType = VK_GEOMETRY_TYPE_TRIANGLES_KHR,
				.maxPrimitiveCount = static_cast<uint32_t>(indices.size() / 3),
				.indexType = VK_INDEX_TYPE_UINT32,
				.maxVertexCount = static_cast<uint32_t>(vertices.size() / 3),
				.vertexFormat = VK_FORMAT_R32G32B32_SFLOAT,
				.allowsTransforms = VK_FALSE,
			};

			accelerationStructure = createAccelerationStructure(
				accelerationCreateGeometryInfo,
				VK_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL_KHR);

			MappedBuffer objectMemory = createAccelerationScratchBuffer(
				accelerationStructure, 
				VK_ACCELERATION_STRUCTURE_MEMORY_REQUIREMENTS_TYPE_OBJECT_KHR);

			BindAccelerationMemory(accelerationStructure, objectMemory.memory);

			MappedBuffer buildScratchMemory = createAccelerationScratchBuffer(
				accelerationStructure, 
				VK_ACCELERATION_STRUCTURE_MEMORY_REQUIREMENTS_TYPE_BUILD_SCRATCH_KHR);

			// Get bottom level acceleration structure handle for use in top level instances
			VkAccelerationStructureDeviceAddressInfoKHR devAddrInfo
			{
				.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_DEVICE_ADDRESS_INFO_KHR,
				.pNext = nullptr,
				.accelerationStructure = accelerationStructure,
			};
			address = vkGetAccelerationStructureDeviceAddressKHR(device, &devAddrInfo);
			
			MappedBuffer vertexBuffer = createMappedBuffer(vertices);

			MappedBuffer indexBuffer = createMappedBuffer(indices);

			VkAccelerationStructureGeometryKHR accelerationGeometry
			{
				.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_KHR,
				.pNext = nullptr,
				.geometryType = VK_GEOMETRY_TYPE_TRIANGLES_KHR,
				.geometry
				{
					.triangles
					{
						.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_TRIANGLES_DATA_KHR,
						.pNext = nullptr,
						.vertexFormat = VK_FORMAT_R32G32B32_SFLOAT,
						.vertexData{ .deviceAddress = vertexBuffer.memoryAddress },
						.vertexStride = 3 * sizeof(float),
						.indexType = VK_INDEX_TYPE_UINT32,
						.indexData{ .deviceAddress = indexBuffer.memoryAddress },
						.transformData{ .deviceAddress = 0 },
					}
				},
				.flags = VK_GEOMETRY_OPAQUE_BIT_KHR,
			};

			std::vector<VkAccelerationStructureGeometryKHR> accelerationGeometries( { accelerationGeometry });
			const VkAccelerationStructureGeometryKHR *ppGeometries = accelerationGeometries.data();

			VkAccelerationStructureBuildGeometryInfoKHR accelerationBuildGeometryInfo
			{
				.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_GEOMETRY_INFO_KHR,
				.pNext = nullptr,
				.type = VK_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL_KHR,
				.flags = VK_BUILD_ACCELERATION_STRUCTURE_PREFER_FAST_TRACE_BIT_KHR,
				.update = VK_FALSE,
				.srcAccelerationStructure = VK_NULL_HANDLE,
				.dstAccelerationStructure = accelerationStructure,
				.geometryArrayOfPointers = VK_FALSE,
				.geometryCount = 1,
				.ppGeometries = &ppGeometries,
				.scratchData{.deviceAddress = buildScratchMemory.memoryAddress },
			};

			VkAccelerationStructureBuildOffsetInfoKHR accelerationBuildOffsetInfo
			{
				.primitiveCount = 1,
				.primitiveOffset = 0,
				.firstVertex = 0,
				.transformOffset = 0
			};

			std::vector<VkAccelerationStructureBuildOffsetInfoKHR *> accelerationBuildOffsets = { &accelerationBuildOffsetInfo };

			VkCommandBuffer commandBuffer = vkut::initSingleTimeCommands(commandPool);

			vkCmdBuildAccelerationStructureKHR(commandBuffer, 1, &accelerationBuildGeometryInfo, accelerationBuildOffsets.data());

			vkut::submitSingleTimeCommands(commandPool, commandBuffer);

			assert(address != 0);
			BottomLevelAccelerationStructure result
			{
				.mappedBuffer = accelerationMappedBuffer,
				.accelerationStructure = accelerationStructure,
				.address = address,
				.indexBuffer = indexBuffer,
				.vertexBuffer = vertexBuffer
			};

			destroyMappedBuffer(objectMemory);
			destroyMappedBuffer(buildScratchMemory);

			Logger::logMessageFormatted("Created bottom level acceleration structure %u! ", accelerationStructure);

			return result;
		}

		void destroyBottomLevelAccelerationStructure(BottomLevelAccelerationStructure blas)
		{
			destroyMappedBuffer(blas.mappedBuffer);
			destroyMappedBuffer(blas.indexBuffer);
			destroyMappedBuffer(blas.vertexBuffer);

			vkDestroyAccelerationStructureKHR(vkut::device, blas.accelerationStructure, nullptr);
			Logger::logMessageFormatted("Destroyed bottom level acceleration structure %u! ", blas.accelerationStructure);
		}

		TopLevelAccelerationStructure createTLAS(VkCommandPool commandPool, const std::vector<VkAccelerationStructureInstanceKHR> &instances)
		{
			VkAccelerationStructureKHR accelerationStructure = {};

			VkAccelerationStructureCreateGeometryTypeInfoKHR accelerationCreateGeometryInfo
			{
				.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_CREATE_GEOMETRY_TYPE_INFO_KHR,
				.pNext = nullptr,
				.geometryType = VK_GEOMETRY_TYPE_INSTANCES_KHR,
				.maxPrimitiveCount = static_cast<uint32_t>(instances.size()),
				.indexType = VK_INDEX_TYPE_NONE_KHR,
				.maxVertexCount = 0,
				.vertexFormat = VK_FORMAT_UNDEFINED,
				.allowsTransforms = VK_FALSE
			};

			accelerationStructure = createAccelerationStructure(accelerationCreateGeometryInfo, VK_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL_KHR);

			MappedBuffer objectMemory = createAccelerationScratchBuffer(
				accelerationStructure, VK_ACCELERATION_STRUCTURE_MEMORY_REQUIREMENTS_TYPE_OBJECT_KHR);

			BindAccelerationMemory(accelerationStructure, objectMemory.memory);

			MappedBuffer buildScratchMemory = createAccelerationScratchBuffer(
				accelerationStructure, VK_ACCELERATION_STRUCTURE_MEMORY_REQUIREMENTS_TYPE_BUILD_SCRATCH_KHR);

			MappedBuffer instanceBuffer = createMappedBuffer(instances);

			VkAccelerationStructureGeometryKHR accelerationGeometry
			{
				.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_KHR,
				.pNext = nullptr,
				.geometryType = VK_GEOMETRY_TYPE_INSTANCES_KHR,
				.geometry
				{
					.instances
					{
						.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_INSTANCES_DATA_KHR,
						.pNext = nullptr,
						.arrayOfPointers = VK_FALSE,
						.data{ .deviceAddress = instanceBuffer.memoryAddress }
					}
				},
				.flags = VK_GEOMETRY_OPAQUE_BIT_KHR,
			};

			std::vector<VkAccelerationStructureGeometryKHR> accelerationGeometries({ accelerationGeometry });
			const VkAccelerationStructureGeometryKHR *ppGeometries = accelerationGeometries.data();

			VkAccelerationStructureBuildGeometryInfoKHR accelerationBuildGeometryInfo
			{
				.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_GEOMETRY_INFO_KHR,
				.pNext = nullptr,
				.type = VK_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL_KHR,
				.flags = VK_BUILD_ACCELERATION_STRUCTURE_PREFER_FAST_TRACE_BIT_KHR,
				.update = VK_FALSE,
				.srcAccelerationStructure = VK_NULL_HANDLE,
				.dstAccelerationStructure = accelerationStructure,
				.geometryArrayOfPointers = VK_FALSE,
				.geometryCount = 1,
				.ppGeometries = &ppGeometries,
				.scratchData { .deviceAddress = buildScratchMemory.memoryAddress }
			};

			VkAccelerationStructureBuildOffsetInfoKHR accelerationBuildOffsetInfo
			{
				.primitiveCount = 1,
				.primitiveOffset = 0x0,
				.firstVertex = 0,
				.transformOffset = 0x0
			};

			std::vector<VkAccelerationStructureBuildOffsetInfoKHR *> accelerationBuildOffsets = { &accelerationBuildOffsetInfo };

			VkCommandBuffer commandBuffer = initSingleTimeCommands(commandPool);

			vkCmdBuildAccelerationStructureKHR(commandBuffer, 1, &accelerationBuildGeometryInfo,
				accelerationBuildOffsets.data());

			submitSingleTimeCommands(commandPool, commandBuffer);

			destroyMappedBuffer(buildScratchMemory);
			destroyMappedBuffer(objectMemory);

			Logger::logMessageFormatted("Created top level acceleration structure %u! ", accelerationStructure);

			return TopLevelAccelerationStructure
			{
				.instanceBuffer = instanceBuffer,
				.accelerationStructure = accelerationStructure
			};
		}

		void destroyTopLevelAccelerationStructure(TopLevelAccelerationStructure tlas)
		{
			destroyMappedBuffer(tlas.instanceBuffer);
			vkDestroyAccelerationStructureKHR(vkut::device, tlas.accelerationStructure, nullptr);

			Logger::logMessageFormatted("Destroyed top level acceleration structure %u! ", tlas.accelerationStructure);
		}

		VkPipeline createPipeline(VkPipelineLayout layout, const std::vector<VkPipelineShaderStageCreateInfo> &stages, const std::vector<VkRayTracingShaderGroupCreateInfoKHR> &groups)
		{
			VkPipeline pipeline = {};
			VkRayTracingPipelineCreateInfoKHR info
			{
				.sType = VK_STRUCTURE_TYPE_RAY_TRACING_PIPELINE_CREATE_INFO_KHR,
				.pNext = nullptr,
				.flags = 0,
				.stageCount = static_cast<uint32_t>(stages.size()),
				.pStages = stages.data(),
				.groupCount = static_cast<uint32_t>(groups.size()),
				.pGroups = groups.data(),
				.maxRecursionDepth = physicalDeviceRaytracingProperties.maxRecursionDepth,
				.libraries = VK_STRUCTURE_TYPE_PIPELINE_LIBRARY_CREATE_INFO_KHR,
				.pLibraryInterface = 0,
				.layout = layout,
				.basePipelineHandle = VK_NULL_HANDLE, // Optional
				.basePipelineIndex = -1, // Optional
			};

			VK_CHECK(vkCreateRayTracingPipelinesKHR(vkut::device, VK_NULL_HANDLE, 1, &info, nullptr, &pipeline));
			Logger::logMessageFormatted("Created raytracing pipeline %u! ", pipeline);

			return pipeline;
		}

		VkRayTracingShaderGroupCreateInfoKHR getShaderGroupCreateInfo(ShaderGroupType groupType, uint32_t index)
		{
			VkRayTracingShaderGroupCreateInfoKHR info = {};

			switch (groupType)
			{
			case vkut::raytracing::ShaderGroupType::RAY_GENERATION:

				info = VkRayTracingShaderGroupCreateInfoKHR
				{
					.type = VK_RAY_TRACING_SHADER_GROUP_TYPE_GENERAL_KHR,
					.generalShader = index,
					.closestHitShader = VK_SHADER_UNUSED_KHR,
				};

				break;
			case vkut::raytracing::ShaderGroupType::CLOSEST_HIT:

				info = VkRayTracingShaderGroupCreateInfoKHR
				{
					.type = VK_RAY_TRACING_SHADER_GROUP_TYPE_TRIANGLES_HIT_GROUP_KHR,
					.generalShader = VK_SHADER_UNUSED_KHR,
					.closestHitShader = index,
				};

				break;
			case vkut::raytracing::ShaderGroupType::MISS:

				info = VkRayTracingShaderGroupCreateInfoKHR
				{
					.type = VK_RAY_TRACING_SHADER_GROUP_TYPE_GENERAL_KHR,
					.generalShader = index,
					.closestHitShader = VK_SHADER_UNUSED_KHR,
				};

				break;
			default:
				assert(false);
				break;
			}

			info.sType = VK_STRUCTURE_TYPE_RAY_TRACING_SHADER_GROUP_CREATE_INFO_KHR;
			info.intersectionShader = VK_SHADER_UNUSED_KHR;
			info.anyHitShader = VK_SHADER_UNUSED_KHR;

			return info;
		}

		VkPipelineShaderStageCreateInfo getShaderStageCreateInfo(VkShaderModule shaderModule, VkShaderStageFlagBits stage)
		{
			VkPipelineShaderStageCreateInfo info =
			{
				.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
				.pNext = nullptr,
				.flags = 0,
				.stage = stage,
				.module = shaderModule,
				.pName = "main",
				.pSpecializationInfo = nullptr,
			};

			return info;
		}

		ShaderBindingTable createShaderBindingTable(VkCommandPool commandPool, VkPipeline pipeline, std::vector<uint32_t> handles)
		{
			uint32_t groupCount = static_cast<uint32_t>(handles.size());
			uint32_t groupHandleSize = physicalDeviceRaytracingProperties.shaderGroupHandleSize;

			uint32_t bindingTableSize = groupCount * groupHandleSize;
			std::vector<char> shaderHandleStorage(bindingTableSize);
			vkGetRayTracingShaderGroupHandlesKHR(device, pipeline, 0, groupCount, bindingTableSize, shaderHandleStorage.data());

			Buffer stagingBuffer = vkut::common::createBuffer(bindingTableSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

			void *data;
			vkMapMemory(device, stagingBuffer.memory, 0, bindingTableSize, 0, &data);
			memcpy(data, shaderHandleStorage.data(), shaderHandleStorage.size());
			vkUnmapMemory(device, stagingBuffer.memory);

			ShaderBindingTable table = vkut::common::createBuffer(bindingTableSize, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_RAY_TRACING_BIT_KHR, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
			
			VkCommandBuffer commandBuffer = vkut::initSingleTimeCommands(commandPool);
			VkBufferCopy copyRegion
			{
				.size = bindingTableSize
			};

			vkCmdCopyBuffer(commandBuffer, stagingBuffer.buffer, table.buffer, 1, &copyRegion);
			
			vkut::submitSingleTimeCommands(commandPool, commandBuffer);

			vkut::common::destroyBuffer(stagingBuffer);

			return table;
		}
		
		void destroyShaderBindingTable(ShaderBindingTable table)
		{
			vkut::common::destroyBuffer(table);
		}

	}

}