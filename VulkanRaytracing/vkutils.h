#ifndef VK_UTILS_H_DEFINED
#define VK_UTILS_H_DEFINED
#define VK_ENABLE_BETA_EXTENSIONS
#include "vulkan.h"
#include <vector>
#include "Optional.h"
#include <utility>

#define VK_CHECK(vkOp) 				  														\
{									  														\
	VkResult result = vkOp; 		  														\
	if(result!=VK_SUCCESS)				  													\
	{ 								  														\
		Logger::LogErrorFormatted("Vulkan result was not VK_SUCCESS! Code : %u\n", result);	\
	} 								  														\
	assert(result == VK_SUCCESS); 	  														\
	(void*)result;					  														\
}

struct GLFWwindow;

namespace vkut {
	
	inline VkInstance instance = {};
	inline VkPhysicalDevice physicalDevice = {};
	inline VkSurfaceKHR surface = {};
	inline VkDevice device = {};
	inline VkQueue graphicsQueue = {};
	inline VkQueue presentQueue = {};

	inline VkSwapchainKHR swapChain;
	inline std::vector<VkImage> swapChainImages;
	inline std::vector<VkImageView> swapChainImageViews;
	inline VkFormat swapChainImageFormat = {};
	inline VkExtent2D swapChainExtent = {};

	//common
	VkImageView createImageView(VkImage image, VkFormat format, VkImageAspectFlags aspectFlags, uint32_t mipLevels);
	void destroyImageView(VkImageView view);

	VkRenderPass createRenderPass(const std::vector<VkAttachmentDescription> &colorDescriptions, Optional<VkAttachmentDescription> depthDescription = Optional<VkAttachmentDescription>());
	void destroyRenderPass(VkRenderPass renderPass);


	//setup
	GLFWwindow *createWindow(const char *title, int width, int height);
	void destroyWindow(GLFWwindow *window);

	void createInstance(const char *title);
	void destroyInstance();

	void createDebugMessenger();
	void destroyDebugMessenger();

	void createSurface(GLFWwindow *window);
	void destroySurface();
	
	void choosePhysicalDevice(std::vector<const char *> requiredDeviceExtensions);

	void createLogicalDevice();
	void destroyLogicalDevice();

	void createSwapChain(uint32_t width, uint32_t height);
	void destroySwapChain();

	void createSwapchainImageViews();
	void destroySwapchainImageViews();

}

#endif