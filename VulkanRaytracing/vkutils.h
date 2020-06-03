#ifndef VK_UTILS_H_DEFINED
#define VK_UTILS_H_DEFINED

#define VK_ENABLE_BETA_EXTENSIONS
#include "vulkan.h"
#include <vector>
#include "Optional.h"
#include <utility>

#define VK_CHECK(vkOp) 				  														\
{									  														\
	VkResult res = vkOp; 		  															\
	if(res != VK_SUCCESS)				  													\
	{ 								  														\
		Logger::logErrorFormatted("Vulkan result was not VK_SUCCESS! Code : %u\n", res);	\
		assert(false);																		\
	} 								  														\
	(void*)res;					  															\
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
		uint32_t dstBinding;
		uint32_t dstArrayElement;
		uint32_t descriptorCount;
		VkDescriptorType descriptorType;
		const VkDescriptorImageInfo *pImageInfo;
		const VkDescriptorBufferInfo *pBufferInfo;
		const VkBufferView *pTexelBufferView;
	};


	namespace common {

		[[nodiscard]]
		Image createImage(VkImageCreateInfo imageCreateInfo, VkMemoryPropertyFlags properties);
		void destroyImage(Image image); 
		void transitionImageLayout(
			VkCommandPool commandPool,
			VkImage image,
			VkImageLayout oldLayout,
			VkImageLayout newLayout,
			VkImageSubresourceRange subresourceRange,
			VkPipelineStageFlags sourceStage,
			VkPipelineStageFlags destinationStage);
		void transitionImageLayout(
			VkCommandBuffer commandBuffer,
			VkImage image,
			VkImageLayout oldLayout,
			VkImageLayout newLayout,
			VkImageSubresourceRange subresourceRange,
			VkPipelineStageFlags sourceStage,
			VkPipelineStageFlags destinationStage);

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

		[[nodiscard]]
		VkDescriptorPool createDescriptorPool(const std::vector<VkDescriptorType> &descriptorTypes, uint32_t maxDescriptorCount, uint32_t maxSets);

		//implicitly destroys all descriptor sets created from this pool
		void destroyDescriptorPool(VkDescriptorPool descriptorPool);

		//descriptor sets are implicitly destroyed during destroyDescriptorPool
		VkDescriptorSet createDescriptorSet(VkDescriptorSetLayout descriptorSetLayout, VkDescriptorPool descriptorPool, const std::vector<DescriptorSetInfo> &descriptorWrites);

		[[nodiscard]]
		Buffer createBuffer(VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags propertyFlags, VkMemoryAllocateFlagsInfo flagsInfo = {});
		void destroyBuffer(const Buffer &buffer);

		template<typename T> void copyToBuffer(const Buffer &buffer, const T &data);
		template<typename T> void copyToBuffer(const Buffer &buffer, const std::vector<T> &data);
		

		[[nodiscard]]
		VkShaderModule createShaderModule(const char *path);
		void destroyShaderModule(VkShaderModule shaderModule);

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

		void createLogicalDevice(void *pNext = nullptr);
		void destroyLogicalDevice();

		void createSwapChain(uint32_t width, uint32_t height, VkImageUsageFlags flags = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT);
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
		

		inline PFN_vkCreateAccelerationStructureKHR vkCreateAccelerationStructureKHR;
		inline PFN_vkDestroyAccelerationStructureKHR vkDestroyAccelerationStructureKHR;
		inline PFN_vkBindAccelerationStructureMemoryKHR vkBindAccelerationStructureMemoryKHR;
		inline PFN_vkGetAccelerationStructureMemoryRequirementsKHR vkGetAccelerationStructureMemoryRequirementsKHR;
		inline PFN_vkCmdBuildAccelerationStructureKHR vkCmdBuildAccelerationStructureKHR;
		inline PFN_vkCreateRayTracingPipelinesKHR vkCreateRayTracingPipelinesKHR;
		inline PFN_vkGetRayTracingShaderGroupHandlesKHR vkGetRayTracingShaderGroupHandlesKHR;
		inline PFN_vkCmdTraceRaysKHR vkCmdTraceRaysKHR;
		inline PFN_vkGetAccelerationStructureDeviceAddressKHR vkGetAccelerationStructureDeviceAddressKHR;

		struct MappedBuffer
		{
			VkBuffer buffer = {};
			VkDeviceMemory memory = {};
			uint64_t memoryAddress = {};
			void *mappedPointer = nullptr;
		};

		using ShaderBindingTable = Buffer;

		struct BottomLevelAccelerationStructure
		{
			MappedBuffer mappedBuffer;
			VkAccelerationStructureKHR accelerationStructure;
			uint64_t address;
			MappedBuffer indexBuffer, vertexBuffer;
		};

		struct TopLevelAccelerationStructure
		{
			MappedBuffer instanceBuffer;
			VkAccelerationStructureKHR accelerationStructure;
		};

		enum class ShaderGroupType
		{
			RAY_GENERATION,
			CLOSEST_HIT,
			MISS
		};

		inline VkPhysicalDeviceRayTracingPropertiesKHR physicalDeviceRaytracingProperties = {};

		void initRaytracingFunctions();
		
		void getPhysicalDeviceRaytracingProperties();

		[[nodiscard]]
		VkPipeline createPipeline(VkPipelineLayout layout, const std::vector<VkPipelineShaderStageCreateInfo> &stages, const std::vector<VkRayTracingShaderGroupCreateInfoKHR> &groups);

		VkRayTracingShaderGroupCreateInfoKHR getShaderGroupCreateInfo(ShaderGroupType groupType, uint32_t index);
		VkPipelineShaderStageCreateInfo getShaderStageCreateInfo(VkShaderModule shaderModule, VkShaderStageFlagBits stage);

		[[nodiscard]]
		BottomLevelAccelerationStructure createBLAS(VkCommandPool commandPool, const std::vector<float> &vertices, const std::vector<uint32_t> &indices);
		void destroyBottomLevelAccelerationStructure(BottomLevelAccelerationStructure blas);
		
		[[nodiscard]]
		TopLevelAccelerationStructure createTLAS(VkCommandPool commandPool, const std::vector<VkAccelerationStructureInstanceKHR> &instances);
		void destroyTopLevelAccelerationStructure(TopLevelAccelerationStructure tlas);

		[[nodiscard]]
		ShaderBindingTable createShaderBindingTable(VkCommandPool commandPool, VkPipeline pipeline, std::vector<uint32_t> handles);
		void destroyShaderBindingTable(ShaderBindingTable table);
	}
}

#endif