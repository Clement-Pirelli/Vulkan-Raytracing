#include "Raytracer.h"
#include <assert.h>
#include "Logger/Logger.h"
#include "Files.h"


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
				.finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
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

	vkut::common::destroyDescriptorPool(descriptorPool);

	vkut::setup::destroySwapchainImageViews();
	vkut::setup::destroySwapChain();

	

	//recreation	
	vkut::setup::createSwapChain(width, height, VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_STORAGE_BIT);
	vkut::setup::createSwapchainImageViews();

	VkImageSubresourceRange subresourceRange
	{
		.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
		.baseMipLevel = 0,
		.levelCount = 1,
		.baseArrayLayer = 0,
		.layerCount = 1
	};

	for (size_t i = 0; i < vkut::swapChainImages.size(); i++)
	{
		vkut::common::transitionImageLayout(commandPool,
			vkut::swapChainImages[i],
			VK_IMAGE_LAYOUT_UNDEFINED,
			VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,
			subresourceRange,
			VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
			VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT);
	}

	createDescriptorPool();
	createDescriptorSets();
	
	commandBuffers = vkut::common::createCommandBuffers(commandPool, vkut::swapChainImages.size());
	recordCommandBuffers();
}

void Raytracer::recordCommandBuffers()
{
	VkImageSubresourceRange subresourceRange
	{
		.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT, 
		.baseMipLevel = 0, 
		.levelCount = 1, 
		.baseArrayLayer = 0, 
		.layerCount = 1 
	};

	for (size_t i = 0; i < commandBuffers.size(); i++) {
		vkut::common::startRecordCommandBuffer(commandBuffers[i]);

		vkCmdBindPipeline(commandBuffers[i], VK_PIPELINE_BIND_POINT_RAY_TRACING_KHR, pipeline);
		vkut::common::transitionImageLayout(
			commandBuffers[i], 
			vkut::swapChainImages[i], 
			VK_IMAGE_LAYOUT_PRESENT_SRC_KHR, 
			VK_IMAGE_LAYOUT_GENERAL,
			subresourceRange,
			VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT,
			VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT
			);
		
		vkCmdBindDescriptorSets(commandBuffers[i], VK_PIPELINE_BIND_POINT_RAY_TRACING_KHR, pipelineLayout, 0, 1, &descriptorSets[i], 0, nullptr);
		
		size_t handleSize = vkut::raytracing::physicalDeviceRaytracingProperties.shaderGroupHandleSize;
		size_t bindingTableSize = handleSize * 3U;
		size_t rayGenOffset = 0U * handleSize;
		size_t missOffset = 1U * handleSize;
		size_t hitGroupOffset = 2U * handleSize;
		
		VkStridedBufferRegionKHR raygenBufferRegion
		{
			.buffer = shaderBindingTable.buffer,
			.offset = rayGenOffset,
			.stride = handleSize,
			.size = bindingTableSize
		};

		VkStridedBufferRegionKHR missBufferRegion
		{
			.buffer = shaderBindingTable.buffer,
			.offset = missOffset,
			.stride = handleSize,
			.size = bindingTableSize
		};

		VkStridedBufferRegionKHR hitGroupBufferRegion
		{
			.buffer = shaderBindingTable.buffer,
			.offset = hitGroupOffset,
			.stride = handleSize,
			.size = bindingTableSize
		};

		VkStridedBufferRegionKHR callableBufferRegion
		{
			.buffer = shaderBindingTable.buffer,
			.offset = 0,
			.stride = 0,
			.size = 0
		};

		vkut::raytracing::vkCmdTraceRaysKHR(
			commandBuffers[i],
			&raygenBufferRegion,
			&missBufferRegion,
			&hitGroupBufferRegion,
			&callableBufferRegion,
			vkut::swapChainExtent.width,
			vkut::swapChainExtent.height,
			1
		);


		vkut::common::transitionImageLayout(
			commandBuffers[i], 
			vkut::swapChainImages[i], 
			VK_IMAGE_LAYOUT_GENERAL, 
			VK_IMAGE_LAYOUT_PRESENT_SRC_KHR, 
			subresourceRange,
			VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
			VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT);

		vkut::common::endRecordCommandBuffer(commandBuffers[i]);
	}

}

void Raytracer::createAccelerationStructures()
{
	blas = vkut::raytracing::createBLAS(commandPool, vertices, indices);

	std::vector<VkAccelerationStructureInstanceKHR> instances =
	{
		VkAccelerationStructureInstanceKHR
		{
			.transform = {1.0f, 0.0f, 0.0, 0.0f, 0.0f, 1.0f, 0.0, 0.0f, 0.0f, 0.0f, 1.0, 0.0f},
			.instanceCustomIndex = 0,
			.mask = 0xFF,
			.instanceShaderBindingTableRecordOffset = 0x0,
			.flags = VK_GEOMETRY_INSTANCE_TRIANGLE_FACING_CULL_DISABLE_BIT_KHR,
			.accelerationStructureReference = blas.address
		}
	};

	tlas = vkut::raytracing::createTLAS(commandPool, instances);
}

std::vector<VkDescriptorType> Raytracer::createDescriptorSetLayout()
{
	VkDescriptorSetLayoutBinding accelerationStructureLayoutBinding
	{
		.binding = 0,
		.descriptorType = VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_KHR,
		.descriptorCount = 1,
		.stageFlags = VK_SHADER_STAGE_RAYGEN_BIT_KHR,
		.pImmutableSamplers = nullptr,
	};

	VkDescriptorSetLayoutBinding storageImageLayoutBinding
	{
		.binding = 1,
		.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE,
		.descriptorCount = 1,
		.stageFlags = VK_SHADER_STAGE_RAYGEN_BIT_KHR,
		.pImmutableSamplers = nullptr,
	};

	std::vector<VkDescriptorSetLayoutBinding> bindings = { accelerationStructureLayoutBinding, storageImageLayoutBinding };

	descriptorSetLayout = vkut::common::createDescriptorSetLayout(bindings);

	std::vector<VkDescriptorType> types = std::vector<VkDescriptorType>(bindings.size());
	for (size_t i = 0; i < types.size(); i++)
	{
		types[i] = bindings[i].descriptorType;
	}
	return types;
}

void Raytracer::createDescriptorPool()
{
	uint32_t maxSets = static_cast<uint32_t>(vkut::swapChainImages.size());
	uint32_t descriptorCount = maxSets;
	descriptorPool = vkut::common::createDescriptorPool(descriptorTypes, descriptorCount, maxSets);
}

void Raytracer::createDescriptorSets()
{
	VkWriteDescriptorSetAccelerationStructureKHR descriptorAccelerationStructureInfo
	{
		.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET_ACCELERATION_STRUCTURE_KHR,
		.pNext = nullptr,
		.accelerationStructureCount = 1,
		.pAccelerationStructures = &tlas.accelerationStructure
	};

	vkut::DescriptorSetInfo accelerationStructureSetInfo
	{
		.pNext = &descriptorAccelerationStructureInfo,
		.dstBinding = 0,
		.dstArrayElement = 0,
		.descriptorCount = 1,
		.descriptorType = VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_KHR,
		.pImageInfo = nullptr,
		.pBufferInfo = nullptr,
		.pTexelBufferView = nullptr
	};

	vkut::DescriptorSetInfo imageSetInfo
	{
		.pNext = nullptr,
		.dstBinding = 1,
		.dstArrayElement = 0,
		.descriptorCount = 1,
		.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE,
	};

	descriptorSets.resize(vkut::swapChainImages.size());
	for(size_t i = 0; i < descriptorSets.size(); i++)
	{
		VkDescriptorImageInfo imageInfo
		{
			.sampler = VK_NULL_HANDLE,
			.imageView = vkut::swapChainImageViews[i],
			.imageLayout = VK_IMAGE_LAYOUT_GENERAL
		};
		
		imageSetInfo.pImageInfo = &imageInfo;

		std::vector<vkut::DescriptorSetInfo> descriptorSetInfos
		{
			accelerationStructureSetInfo,
			imageSetInfo
		};

		descriptorSets[i] = vkut::common::createDescriptorSet(descriptorSetLayout, descriptorPool, descriptorSetInfos);

	}
}

void Raytracer::createPipeline()
{

	std::vector<VkPipelineShaderStageCreateInfo> shaderStages
	{
		vkut::raytracing::getShaderStageCreateInfo(vkut::common::createShaderModule("../Assets/shaders/raytrace.rgen.spv"), VK_SHADER_STAGE_RAYGEN_BIT_KHR),
		vkut::raytracing::getShaderStageCreateInfo(vkut::common::createShaderModule("../Assets/shaders/raytrace.rmiss.spv"), VK_SHADER_STAGE_MISS_BIT_KHR),
		vkut::raytracing::getShaderStageCreateInfo(vkut::common::createShaderModule("../Assets/shaders/raytrace.rchit.spv"), VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR)
	};

	std::vector<VkRayTracingShaderGroupCreateInfoKHR> shaderGroups = 
	{
		vkut::raytracing::getShaderGroupCreateInfo(vkut::raytracing::ShaderGroupType::RAY_GENERATION, raygenShaderIndex),
		vkut::raytracing::getShaderGroupCreateInfo(vkut::raytracing::ShaderGroupType::MISS, missShaderIndex),
		vkut::raytracing::getShaderGroupCreateInfo(vkut::raytracing::ShaderGroupType::CLOSEST_HIT, closestHitShaderIndex),
	};

	pipeline = vkut::raytracing::createPipeline(pipelineLayout, shaderStages, shaderGroups);

	for (size_t i = 0; i < 3U; i++)
	{
		vkut::common::destroyShaderModule(shaderStages[i].module);
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

	VkPipelineStageFlags waitStages[] = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };
	VkSubmitInfo submitInfo
	{
		.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
		.waitSemaphoreCount = 1,
		.pWaitSemaphores = &currentSemaphore,
		.pWaitDstStageMask = waitStages,
		.commandBufferCount = 1,
		.pCommandBuffers = &commandBuffers[imageIndex],
		.signalSemaphoreCount = 1,
		.pSignalSemaphores = &currentSemaphore,
	};

	VkFence *toReset = &vkut::inFlightFences[currentFrame];
	VK_CHECK(vkResetFences(vkut::device, 1, toReset));

	VK_CHECK(vkQueueSubmit(vkut::graphicsQueue, 1, &submitInfo, *toReset));

	VkPresentInfoKHR presentInfo
	{
		.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR,
		.waitSemaphoreCount = 1,
		.pWaitSemaphores = &currentSemaphore,
		.swapchainCount = 1,
		.pSwapchains = &vkut::swapChain,
		.pImageIndices = &imageIndex,
		.pResults = nullptr // Optional
	};

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

void Raytracer::init()
{
	window = vkut::setup::createWindow(title, width, height);
	glfwSetWindowUserPointer(window, this);
	glfwSetFramebufferSizeCallback(window, Raytracer::framebufferResizeCallback);

	vkut::setup::createInstance(title);
	vkut::setup::createDebugMessenger();
	vkut::setup::createSurface(window);

	vkut::setup::choosePhysicalDevice(requiredExtensions);
	vkut::raytracing::getPhysicalDeviceRaytracingProperties();

	VkPhysicalDeviceRayTracingFeaturesKHR rayTracingFeatures
	{
		.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_RAY_TRACING_FEATURES_KHR,
		.rayTracing = VK_TRUE,
	};
	vkut::setup::createLogicalDevice(&rayTracingFeatures);
	vkut::raytracing::initRaytracingFunctions();

	vkut::setup::createSwapChain(width, height, VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_STORAGE_BIT);
	vkut::setup::createSwapchainImageViews();

	commandPool = vkut::setup::createGraphicsCommandPool();
	VkImageSubresourceRange subresourceRange
	{
		.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
		.baseMipLevel = 0,
		.levelCount = 1,
		.baseArrayLayer = 0,
		.layerCount = 1
	};

	for (size_t i = 0; i < vkut::swapChainImages.size(); i++) 
	{
		vkut::common::transitionImageLayout(commandPool, 
			vkut::swapChainImages[i],
			VK_IMAGE_LAYOUT_UNDEFINED,
			VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,
			subresourceRange,
			VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
			VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT);
	}

	vkut::setup::createSyncObjects(maxFramesInFlight);
	
	createAccelerationStructures();

	descriptorTypes = createDescriptorSetLayout();

	pipelineLayout = vkut::common::createPipelineLayout({ descriptorSetLayout });

	descriptorPool = vkut::common::createDescriptorPool(descriptorTypes, static_cast<uint32_t>(vkut::swapChainImages.size()), static_cast<uint32_t>(vkut::swapChainImages.size()));
	
	createDescriptorSets();

	createPipeline();

	shaderBindingTable = vkut::raytracing::createShaderBindingTable(commandPool, pipeline, {  raygenShaderIndex, missShaderIndex, closestHitShaderIndex });

	commandBuffers = vkut::common::createCommandBuffers(commandPool, vkut::swapChainImages.size());
	recordCommandBuffers();
}

void Raytracer::run()
{
	init();

	do
	{
		glfwPollEvents();
		drawFrame();
	} while (!glfwWindowShouldClose(window));

	cleanup();
}

void Raytracer::cleanup()
{
	vkDeviceWaitIdle(vkut::device);

	vkut::common::destroyCommandBuffers(commandPool, commandBuffers);

	vkut::raytracing::destroyShaderBindingTable(shaderBindingTable);

	vkut::common::destroyPipeline(pipeline);

	vkut::common::destroyDescriptorPool(descriptorPool);
	
	vkut::common::destroyPipelineLayout(pipelineLayout);

	vkut::common::destroyDescriptorSetLayout(descriptorSetLayout);


	vkut::raytracing::destroyTopLevelAccelerationStructure(tlas);
	vkut::raytracing::destroyBottomLevelAccelerationStructure(blas);

	vkut::setup::destroySyncObjects();

	vkut::setup::destroyCommandPool(commandPool);
	vkut::setup::destroySwapchainImageViews();
	vkut::setup::destroySwapChain();
	vkut::setup::destroyLogicalDevice();
	vkut::setup::destroySurface();
	vkut::setup::destroyDebugMessenger();
	vkut::setup::destroyInstance();
	vkut::setup::destroyWindow(window);
}