#include "RenderContext.hpp"

#include <stdexcept>

#include "spdlog/spdlog.h"

namespace volume_restir {

RenderContext::RenderContext() {
  // create VkInstance
  vkb::InstanceBuilder instance_builder;
  auto instance_build_success = instance_builder.use_default_debug_messenger()
                                    .request_validation_layers()
                                    .build();
  if (!instance_build_success) {
    spdlog::error("Failed to create Vulkan instance: {}",
                  instance_build_success.error().message());
    throw std::runtime_error("Failed to create Vulkan instance");
  }
  instance_ = instance_build_success.value();
  spdlog::debug("Successfully created Vulkan instance");

  // create surface
  if (glfwCreateWindowSurface(instance_.instance,
                              SingletonManager::GetWindow().WindowPtr(),
                              nullptr, &surface_) != VK_SUCCESS) {
    spdlog::error("Failed to create window surface!");
    throw std::runtime_error("Failed to create window surface");
  }
  spdlog::debug("Successfully created Vulkan window surface");

  // create physical device
  vkb::PhysicalDeviceSelector phys_device_selector{instance_};
  auto phys_device_success =
      phys_device_selector.set_surface(surface_).select();
  if (!phys_device_success) {
    spdlog::error("Failed to select physical device: {}",
                  phys_device_success.error().message());
    throw std::runtime_error("Failed to select physical device");
  }
  phys_device_ = phys_device_success.value();
  spdlog::debug("Successfully selected Vulkan physical device");

  // create logical device
  vkb::DeviceBuilder device_builder{phys_device_};
  auto device_success = device_builder.build();
  if (!device_success) {
    spdlog::error("Failed to create logical device: {}",
                  device_success.error().message());
    throw std::runtime_error("Failed to create logical device");
  }
  device_ = device_success.value();
  spdlog::debug("Successfully created Vulkan logical device");
}

RenderContext::~RenderContext() {
  vkb::destroy_device(device_);
  spdlog::debug("Destroyed logical device");
  vkb::destroy_instance(instance_);
  spdlog::debug("Destroyed instance");
  spdlog::info("All RenderContext cleaned up successfully");
}

uint32_t RenderContext::MemoryTypeIndex(
    uint32_t type_bits, VkMemoryPropertyFlags properties) const {
  VkPhysicalDeviceMemoryProperties device_memory_properties =
      Device().physical_device.memory_properties;

  // Iterate over all memory types available for the device used in this example
  for (uint32_t i = 0; i < device_memory_properties.memoryTypeCount; i++) {
    if ((type_bits & 1) == 1) {
      if ((device_memory_properties.memoryTypes[i].propertyFlags &
           properties) == properties) {
        return i;
      }
    }
    type_bits >>= 1;
  }

  spdlog::error("Could not find a suitable memory type!");
  throw std::runtime_error("Could not find a suitable memory type!");
}

}  // namespace volume_restir
