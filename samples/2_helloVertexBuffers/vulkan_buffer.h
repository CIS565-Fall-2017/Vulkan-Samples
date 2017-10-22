#pragma once

#include <vulkan/vulkan.h>
#include "vulkan_device.h"

VkDeviceMemory CreateDeviceMemory(VulkanDevice* device, uint32_t size, uint32_t types, VkMemoryPropertyFlags propertyFlags);
VkBuffer CreateBuffer(VulkanDevice* device, VkBufferUsageFlags allowedUsage, uint32_t size, VkDeviceMemory deviceMemory = VK_NULL_HANDLE, uint32_t deviceMemoryOffset = 0);