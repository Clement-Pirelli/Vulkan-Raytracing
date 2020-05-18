#ifndef VK_UTILS_H_DEFINED
#define VK_UTILS_H_DEFINED
#include "vulkan.h"

struct GLFWwindow;

class vkut {
public:
	
	
	inline static VkInstance instance = {};
	inline static VkPhysicalDevice physicalDevice = {};
	inline static VkSurfaceKHR surface = {};
	inline static VkDevice device = {};
	inline static VkQueue graphicsQueue = {};
	inline static VkQueue presentQueue = {};

	static GLFWwindow *createWindow(const char *title, int width, int height);
	static void destroyWindow(GLFWwindow *window);

	static void createInstance(const char *title);
	static void destroyInstance();

	static void createDebugMessenger();
	static void destroyDebugMessenger();

	static void createSurface(GLFWwindow *window);
	static void destroySurface();
	
	static void choosePhysicalDevice();

	static void createLogicalDevice();
	static void destroyLogicalDevice();


private:
	vkut() = delete;
	vkut(const vkut &o) = delete;
	vkut(vkut &&o) = delete;

};

#endif