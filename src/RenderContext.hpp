#ifndef __VOLUME_RESTIR_INSTANCE_HPP__
#define __VOLUME_RESTIR_INSTANCE_HPP__

/**
 * @file RenderContext.hpp
 * @author Zhihao Ruan (ruanzh@seas.upenn.edu)
 *
 * @brief RenderContext should be a singleton that holds all the shared global
 * rendering contexts for this application. Please checkout SingletonManager.hpp
 * for how to use singletons.
 *
 * @date 2021-11-16
 */

#include <vulkan/vulkan.h>

#include "VkBootstrap.h"

namespace volume_restir {

class RenderContext {
private:
  vkb::Instance instance_;
  VkSurfaceKHR surface_;
};

}  // namespace volume_restir

#endif /* __VOLUME_RESTIR_INSTANCE_HPP__ */
