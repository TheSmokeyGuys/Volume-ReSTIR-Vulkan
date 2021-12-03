#include "utils/buffer.hpp"

#include "utils/logging.hpp"

namespace volume_restir {
namespace buffer {

void CreateBuffer(const RenderContext* render_context, VkDeviceSize size,
                  VkBufferUsageFlags usage, VkMemoryPropertyFlags properties,
                  VkBuffer& buffer, VkDeviceMemory& bufferMemory) {
  // Create buffer
  VkBufferCreateInfo bufferInfo = {};
  bufferInfo.sType              = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
  bufferInfo.size               = size;
  bufferInfo.usage              = usage;
  bufferInfo.sharingMode        = VK_SHARING_MODE_EXCLUSIVE;

  if (vkCreateBuffer(render_context->GetNvvkContext().m_device, &bufferInfo,
                     nullptr, &buffer) != VK_SUCCESS) {
    throw std::runtime_error("Failed to create vertex buffer");
  }

  // Query buffer's memory requirements
  VkMemoryRequirements memRequirements;
  vkGetBufferMemoryRequirements(render_context->GetNvvkContext().m_device,
                                buffer, &memRequirements);

  // Allocate memory in device
  VkMemoryAllocateInfo allocInfo = {};
  allocInfo.sType                = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
  allocInfo.allocationSize       = memRequirements.size;
  allocInfo.memoryTypeIndex      = render_context->MemoryTypeIndex(
      memRequirements.memoryTypeBits, properties);

  if (vkAllocateMemory(render_context->GetNvvkContext().m_device, &allocInfo,
                       nullptr, &bufferMemory) != VK_SUCCESS) {
    throw std::runtime_error("Failed to allocate vertex buffer");
  }

  // Associate allocated memory with vertex buffer
  vkBindBufferMemory(render_context->GetNvvkContext().m_device, buffer,
                     bufferMemory, 0);
}

void CopyBuffer(const RenderContext* render_context, VkCommandPool commandPool,
                VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size) {
  VkCommandBufferAllocateInfo allocInfo = {};
  allocInfo.sType              = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
  allocInfo.level              = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
  allocInfo.commandPool        = commandPool;
  allocInfo.commandBufferCount = 1;

  VkCommandBuffer commandBuffer;
  vkAllocateCommandBuffers(render_context->GetNvvkContext().m_device,
                           &allocInfo, &commandBuffer);

  VkCommandBufferBeginInfo beginInfo = {};
  beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
  beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

  vkBeginCommandBuffer(commandBuffer, &beginInfo);

  VkBufferCopy copyRegion = {};
  copyRegion.size         = size;
  vkCmdCopyBuffer(commandBuffer, srcBuffer, dstBuffer, 1, &copyRegion);

  vkEndCommandBuffer(commandBuffer);

  VkSubmitInfo submitInfo       = {};
  submitInfo.sType              = VK_STRUCTURE_TYPE_SUBMIT_INFO;
  submitInfo.commandBufferCount = 1;
  submitInfo.pCommandBuffers    = &commandBuffer;

  auto graphics_queue = render_context->GetQueues()[QueueFlags::GRAPHICS];

  vkQueueSubmit(graphics_queue, 1, &submitInfo, VK_NULL_HANDLE);
  vkQueueWaitIdle(graphics_queue);
  vkFreeCommandBuffers(render_context->GetNvvkContext().m_device, commandPool,
                       1, &commandBuffer);
}

void CreateBufferFromData(const RenderContext* render_context,
                          VkCommandPool commandPool, void* bufferData,
                          VkDeviceSize bufferSize,
                          VkBufferUsageFlags bufferUsage, VkBuffer& buffer,
                          VkDeviceMemory& bufferMemory) {
  // Create the staging buffer
  VkBuffer stagingBuffer;
  VkDeviceMemory stagingBufferMemory;

  VkBufferUsageFlags stagingUsage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
  VkMemoryPropertyFlags stagingProperties =
      VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
      VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;
  CreateBuffer(render_context, bufferSize, stagingUsage, stagingProperties,
               stagingBuffer, stagingBufferMemory);

  // Fill the staging buffer
  void* data;
  vkMapMemory(render_context->GetNvvkContext().m_device, stagingBufferMemory, 0,
              bufferSize, 0, &data);
  memcpy(data, bufferData, static_cast<size_t>(bufferSize));
  vkUnmapMemory(render_context->GetNvvkContext().m_device, stagingBufferMemory);

  // Create the buffer
  VkBufferUsageFlags usage    = VK_BUFFER_USAGE_TRANSFER_DST_BIT | bufferUsage;
  VkMemoryPropertyFlags flags = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
  CreateBuffer(render_context, bufferSize, usage, flags, buffer, bufferMemory);

  // Copy data from staging to buffer
  CopyBuffer(render_context, commandPool, stagingBuffer, buffer, bufferSize);

  // No need for the staging buffer anymore
  vkDestroyBuffer(render_context->GetNvvkContext().m_device, stagingBuffer,
                  nullptr);
  vkFreeMemory(render_context->GetNvvkContext().m_device, stagingBufferMemory,
               nullptr);
}

}  // namespace buffer
}  // namespace volume_restir
