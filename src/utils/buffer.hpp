#ifndef __VOLUME_RESTIR_UTILS_BUFFER_UTILS_HPP__
#define __VOLUME_RESTIR_UTILS_BUFFER_UTILS_HPP__

#include <vulkan/vulkan.hpp>

#include "RenderContext.hpp"

namespace volume_restir {
namespace buffer {

void CreateBuffer(const RenderContext* render_context, VkDeviceSize size,
                  VkBufferUsageFlags usage, VkMemoryPropertyFlags properties,
                  VkBuffer& buffer, VkDeviceMemory& bufferMemory);
void CopyBuffer(const RenderContext* render_context, VkCommandPool commandPool,
                VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size);
void CreateBufferFromData(const RenderContext* render_context,
                          VkCommandPool commandPool, void* bufferData,
                          VkDeviceSize bufferSize,
                          VkBufferUsageFlags bufferUsage, VkBuffer& buffer,
                          VkDeviceMemory& bufferMemory);

}  // namespace buffer
}  // namespace volume_restir

#endif /* __VOLUME_RESTIR_UTILS_BUFFER_UTILS_HPP__ */
