#ifndef __VOLUME_RESTIR_INSTANCE_HPP__
#define __VOLUME_RESTIR_INSTANCE_HPP__

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
