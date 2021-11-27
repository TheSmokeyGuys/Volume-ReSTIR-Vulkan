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

#include <vulkan/vulkan.h>

#include "VkBootstrap.h"
#include "SingtonManager.hpp"
#include "VertexManager.hpp"

namespace volume_restir {

class RenderContext {
public:
  RenderContext();
  ~RenderContext();

  vkb::Instance& Instance() { return instance_; }
  vkb::Device& Device() { return device_; }
  VkSurfaceKHR Surface() const { return surface_; }
  const vkb::Instance& Instance() const { return instance_; }
  const vkb::Device& Device() const { return device_; }

private:
  vkb::Instance instance_;
  vkb::PhysicalDevice phys_device_;
  vkb::Device device_;
  VkSurfaceKHR surface_;
};

}  // namespace volume_restir

#endif /* __VOLUME_RESTIR_INSTANCE_HPP__ */
