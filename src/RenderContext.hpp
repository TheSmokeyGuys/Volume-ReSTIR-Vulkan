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

  const nvvk::Context& GetNvvkContext() const { return nvvk_context_; }
  const QueueFamilyIndices& GetQueueFamilyIndices() const {
    return queue_family_indices_;
  }

  const int32_t GetQueueFamilyIndex(QueueFlags flag) const {
    return queue_family_indices_[flag];
  }
  const Queues& GetQueues() const { return queues_; }

  vk::SurfaceKHR Surface() const { return surface_; }

  uint32_t MemoryTypeIndex(uint32_t type_bits,
                           VkMemoryPropertyFlags properties) const;

private:
  nvvk::Context nvvk_context_;

  // render queues
  Queues queues_;
  QueueFamilyIndices queue_family_indices_;

  vk::SurfaceKHR surface_;

  void InitQueues();
};

}  // namespace volume_restir

#endif /* __VOLUME_RESTIR_INSTANCE_HPP__ */
