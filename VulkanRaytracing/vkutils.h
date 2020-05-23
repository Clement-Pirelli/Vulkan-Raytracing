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
		assert(false);																		\
	} 								  														\
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

	inline VkSwapchainKHR swapChain = {};
	inline std::vector<VkImage> swapChainImages = {};
	inline std::vector<VkImageView> swapChainImageViews = {};
	inline VkFormat swapChainImageFormat = {};
	inline VkExtent2D swapChainExtent = {};
	inline std::vector<VkSemaphore> imageAvailableSemaphores = {};
	inline std::vector<VkFence> inFlightFences = {};

	//structs
	struct Image
	{
		VkImage image;
		VkDeviceMemory memory;
	};

	struct Buffer
	{
		VkBuffer buffer;
		VkDeviceMemory memory;
		VkDeviceSize size;
	};

	struct DescriptorSetInfo
	{
		const void *pNext;
		uint32_t                         dstBinding;
		uint32_t                         dstArrayElement;
		uint32_t                         descriptorCount;
		VkDescriptorType                 descriptorType;
		const VkDescriptorImageInfo *pImageInfo;
		const VkDescriptorBufferInfo *pBufferInfo;
		const VkBufferView *pTexelBufferView;
	};


	namespace common {

		[[nodiscard]]
		Image createImage(VkImageCreateInfo imageCreateInfo, VkMemoryPropertyFlags properties);
		void destroyImage(Image image); 
		void transitionImageLayout(VkCommandPool commandPool, VkImage image, VkFormat format, VkImageLayout oldLayout, VkImageLayout newLayout, uint32_t mipLevels);

		[[nodiscard]]
		VkImageView createImageView(VkImage image, VkFormat format, VkImageAspectFlags aspectFlags, uint32_t mipLevels);
		void destroyImageView(VkImageView view);

		[[nodiscard]]
		VkRenderPass createRenderPass(const std::vector<VkAttachmentDescription> &colorDescriptions, Optional<VkAttachmentDescription> depthDescription = Optional<VkAttachmentDescription>());
		void destroyRenderPass(VkRenderPass renderPass);

		[[nodiscard]]
		VkPipelineLayout createPipelineLayout(std::vector<VkDescriptorSetLayout> descriptorSetLayouts);
		void destroyPipelineLayout(VkPipelineLayout pipelineLayout);

		void destroyPipeline(VkPipeline pipeline);

		VkFormat findDepthFormat();
		
		[[nodiscard]]
		std::vector<VkCommandBuffer> createCommandBuffers(VkCommandPool commandPool, size_t amount, VkCommandBufferLevel level = VK_COMMAND_BUFFER_LEVEL_PRIMARY);
		void destroyCommandBuffers(VkCommandPool commandPool, const std::vector<VkCommandBuffer> &commandBuffers);

		void startRecordCommandBuffer(VkCommandBuffer commandBuffer);
		void endRecordCommandBuffer(VkCommandBuffer commandBuffer);
	
		[[nodiscard]]
		VkDescriptorSetLayout createDescriptorSetLayout(const std::vector<VkDescriptorSetLayoutBinding> &bindings);
		void destroyDescriptorSetLayout(VkDescriptorSetLayout descriptorSetLayout);

		VkDescriptorPool createDescriptorPool(const std::vector<VkDescriptorType> &descriptorTypes);

		void destroyDescriptorPool(VkDescriptorPool descriptorPool);

		std::vector<VkDescriptorSet> createDescriptorSets(VkDescriptorSetLayout descriptorSetLayout, VkDescriptorPool descriptorPool, const std::vector<DescriptorSetInfo> &descriptorWrites);


		[[nodiscard]]
		Buffer createBuffer(VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags propertyFlags);
		void destroyBuffer(const Buffer &buffer);
	}
	
	namespace setup {
	
		[[nodiscard]]
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

		[[nodiscard]]
		VkCommandPool createGraphicsCommandPool();
		void destroyCommandPool(VkCommandPool commandPool);

		[[nodiscard]]
		VkFramebuffer createRenderPassFramebuffer(VkRenderPass renderPass, uint32_t width, uint32_t height, const std::vector<VkImageView> &colorViews, Optional<VkImageView> depthAttachment);
		void destroyFramebuffer(VkFramebuffer framebuffer);

		void createSyncObjects(size_t maxFramesInFlight);
		void destroySyncObjects();
	}

	namespace raytracing {
		
		inline VkPhysicalDeviceRayTracingPropertiesKHR physicalDeviceRaytracingProperties = {};

		void initRaytracingFunctions();
		[[nodiscard]]
		void getPhysicalDeviceRaytracingProperties();

		[[nodiscard]]
		VkPipeline createPipeline(VkPipelineLayout layout);
	}
}

#endif