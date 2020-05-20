#include "glfw3.h"
#define VK_ENABLE_BETA_EXTENSIONS
#include "vulkan.h"
#include "vkutils.h"

constexpr const char *title = "Raytracing!";
constexpr int width = 1366;
constexpr int height = 768;
const std::vector<const char *> requiredExtensions = {
	VK_KHR_SWAPCHAIN_EXTENSION_NAME,
	VK_KHR_RAY_TRACING_EXTENSION_NAME,
	VK_KHR_PIPELINE_LIBRARY_EXTENSION_NAME,
	VK_EXT_DESCRIPTOR_INDEXING_EXTENSION_NAME,
	VK_KHR_BUFFER_DEVICE_ADDRESS_EXTENSION_NAME,
	VK_KHR_DEFERRED_HOST_OPERATIONS_EXTENSION_NAME,
	VK_KHR_GET_MEMORY_REQUIREMENTS_2_EXTENSION_NAME
};

int main() 
{
	GLFWwindow *window = vkut::createWindow(title, width, height);
	vkut::createInstance(title);
	vkut::createDebugMessenger();
	vkut::createSurface(window);
	vkut::choosePhysicalDevice(requiredExtensions);
	vkut::createLogicalDevice();
	vkut::createSwapChain(width, height);
	vkut::createSwapchainImageViews();

	std::vector<VkAttachmentDescription> descriptions = 
	{
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
	VkRenderPass renderPass = vkut::createRenderPass(descriptions);

	bool quitting = false;

	while (!quitting) 
	{
		glfwPollEvents();
		quitting = glfwWindowShouldClose(window);
	}

	vkut::destroyRenderPass(renderPass);
	vkut::destroySwapchainImageViews();
	vkut::destroySwapChain();
	vkut::destroyLogicalDevice();
	vkut::destroySurface();
	vkut::destroyDebugMessenger();
	vkut::destroyInstance();
	vkut::destroyWindow(window);
}