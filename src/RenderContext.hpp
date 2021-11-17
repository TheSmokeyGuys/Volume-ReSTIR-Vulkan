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

namespace volume_restir {

class RenderContext {
public:
  RenderContext();
  ~RenderContext();

private:
  vkb::Instance instance_;
  VkSurfaceKHR surface_;
  vkb::PhysicalDevice phys_device_;
  vkb::Device device_;
  vkb::Swapchain swapchain_;
};

}  // namespace volume_restir

#endif /* __VOLUME_RESTIR_INSTANCE_HPP__ */
