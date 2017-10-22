
#include "vulkan_buffer.h"

VkDeviceMemory CreateDeviceMemory(VulkanDevice* device, uint32_t size, uint32_t types, VkMemoryPropertyFlags propertyFlags) {
    VkMemoryAllocateInfo memoryAllocateInfo = {};
    memoryAllocateInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    memoryAllocateInfo.allocationSize = size;
    memoryAllocateInfo.memoryTypeIndex = device->GetInstance()->GetMemoryTypeIndex(types, propertyFlags);
    VkDeviceMemory deviceMemory;
    vkAllocateMemory(device->GetVulkanDevice(), &memoryAllocateInfo, nullptr, &deviceMemory);
    return deviceMemory;
}

VkBuffer CreateBuffer(VulkanDevice* device, VkBufferUsageFlags allowedUsage, uint32_t size, VkDeviceMemory deviceMemory, uint32_t deviceMemoryOffset) {
    VkBufferCreateInfo bufferCreateInfo = {};
    bufferCreateInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    bufferCreateInfo.size = size;
    bufferCreateInfo.usage = allowedUsage;
    VkBuffer buffer;
    vkCreateBuffer(device->GetVulkanDevice(), &bufferCreateInfo, nullptr, &buffer);

    if (deviceMemory != VK_NULL_HANDLE) {
        vkBindBufferMemory(device->GetVulkanDevice(), buffer, deviceMemory, deviceMemoryOffset);
    }

    return buffer;
}