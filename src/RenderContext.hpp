#ifndef __VOLUME_RESTIR_INSTANCE_HPP__
#define __VOLUME_RESTIR_INSTANCE_HPP__

/**
 * @file RenderContext.hpp
 * @author Zhihao Ruan (ruanzh@seas.upenn.edu)
 *
 * @brief RenderContext should be used to initialize a Renderer.
 *
 * @date 2021-11-16
 */

#include <vulkan/vulkan.hpp>

#include "nvvk/context_vk.hpp"
#include "utils/vkqueue_utils.hpp"

namespace volume_restir {

using Queues = std::array<VkQueue, sizeof(QueueFlags)>;

class RenderContext {
public:
  RenderContext();
  ~RenderContext();

  const Queues& GetQueues() const { return queues_; }

  const vk::Device& GetDevice() const { return device_; }
  vk::Device& GetDevice() { return device_; }
  const vk::Instance& GetInstance() const { return instance_; }
  const vk::PhysicalDevice& GetPhysicalDevice() const { return phys_device_; }

  vk::SurfaceKHR Surface() const { return surface_; }

  const QueueFamilyIndices& GetQueueFamilyIndices() const {
    return queue_family_indices_;
  }

  const int32_t GetQueueFamilyIndex(QueueFlags flag) const {
    return queue_family_indices_[flag];
  }

  uint32_t MemoryTypeIndex(uint32_t type_bits,
                           VkMemoryPropertyFlags properties) const;

  vk::Device GetDevice() const { return device_; }

private:
  nvvk::Context nvvk_context_;

  vk::Instance instance_;
  vk::Device device_;
  vk::PhysicalDevice phys_device_;

  // render queues
  Queues queues_;
  QueueFamilyIndices queue_family_indices_;

  vk::SurfaceKHR surface_;

  void InitQueues();
};

}  // namespace volume_restir

#endif /* __VOLUME_RESTIR_INSTANCE_HPP__ */
