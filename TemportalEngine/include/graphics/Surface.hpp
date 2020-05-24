#pragma once

#include "TemportalEnginePCH.hpp"

#include <vulkan/vulkan.hpp>

NS_GRAPHICS
class VulkanInstance;
class PhysicalDevice;

class Surface
{
	friend class VulkanInstance;
	friend class PhysicalDevice;

public:
	Surface() = default;
	Surface(void* pWindowHandle);
	
	void swap(Surface &other);

	void releaseWindowHandle();

	Surface& initialize(VulkanInstance *pVulkan);
	void destroy(VulkanInstance *pVulkan);

private:
	void* mpWindowHandle;
	vk::UniqueSurfaceKHR mSurface;

};

NS_END