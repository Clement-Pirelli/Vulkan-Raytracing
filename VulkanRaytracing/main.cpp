#include "glfw3.h"
#include "vulkan.h"
#include "vkutils.h"



int main() {
	constexpr const char *title = "Raytracing!";
	constexpr int width = 1366;
	constexpr int height = 768;
	auto window = vkut::createWindow(title, width, height);
	vkut::createInstance(title);
	vkut::createDebugMessenger();
	vkut::createSurface(window);
	vkut::choosePhysicalDevice();
	vkut::createLogicalDevice();

	bool quitting = false;

	while (!quitting) {
		glfwPollEvents();
		quitting = glfwWindowShouldClose(window);
	}

	vkut::destroyLogicalDevice();
	vkut::destroySurface();
	vkut::destroyDebugMessenger();
	vkut::destroyInstance();
	vkut::destroyWindow(window);
}