#pragma once
#include "glfw3.h"
#define VK_ENABLE_BETA_EXTENSIONS
#include "vulkan.h"
#include "vkutils.h"


class Raytracer
{
public:

	void run();

private:

	static constexpr const char *title = "Raytracing!";	
	const std::vector<const char *> requiredExtensions = {
		VK_KHR_SWAPCHAIN_EXTENSION_NAME,
		VK_KHR_RAY_TRACING_EXTENSION_NAME,
		VK_KHR_PIPELINE_LIBRARY_EXTENSION_NAME,
		VK_EXT_DESCRIPTOR_INDEXING_EXTENSION_NAME,
		VK_KHR_BUFFER_DEVICE_ADDRESS_EXTENSION_NAME,
		VK_KHR_DEFERRED_HOST_OPERATIONS_EXTENSION_NAME,
		VK_KHR_GET_MEMORY_REQUIREMENTS_2_EXTENSION_NAME
	};


	const std::vector<float> vertices = {
		.5f, .2f, .0f,
		.2f, .8f, .0f,
		.8f, .8f, .0f
	};
	const std::vector<uint32_t> indices = { 0, 1, 2 };
	static constexpr size_t maxFramesInFlight = 2;
	size_t currentFrame = 0;

	GLFWwindow *window = nullptr;
	int width = 1366;
	int height = 768;
	bool windowResized = false;

	VkCommandPool commandPool = {};
	std::vector<VkCommandBuffer> commandBuffers = {};
	vkut::raytracing::BottomLevelAccelerationStructure blas = {};
	vkut::raytracing::TopLevelAccelerationStructure tlas = {};
	VkDescriptorSetLayout descriptorSetLayout = {};
	VkDescriptorPool descriptorPool = {};
	std::vector<VkDescriptorSet> descriptorSets = {};
	std::vector<VkDescriptorType> descriptorTypes = {};
	VkPipelineLayout pipelineLayout = {};
	VkPipeline pipeline = {};
	vkut::raytracing::ShaderBindingTable shaderBindingTable = {};

	const uint32_t raygenShaderIndex = 0U;
	const uint32_t missShaderIndex = 1U;
	const uint32_t closestHitShaderIndex = 2U;

	void init();
	void cleanup();

	void recreateSwapchainDependents();

	void recordCommandBuffers();

	void createAccelerationStructures();
	std::vector<VkDescriptorType> createDescriptorSetLayout();
	void createDescriptorPool();
	void createDescriptorSets();
	void createPipeline();

	void drawFrame();

	static void framebufferResizeCallback(GLFWwindow *window, int width, int height);
};

