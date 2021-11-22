#include "RenderContext.hpp"

#include <stdexcept>

#include "SingtonManager.hpp"
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
  VkSurfaceKHR surface_;
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

  // create swapchain
  SwapChain temp(this, surface_);
  swapchain_ = temp;
}

RenderContext::~RenderContext() {
  vkb::destroy_swapchain(swapchain_.swapchain_);
  spdlog::debug("Destroyed swapchain");
  vkb::destroy_device(device_);
  spdlog::debug("Destroyed logical device");
  vkDestroySurfaceKHR(instance_.instance, swapchain_.vkSurface,
                      instance_.allocation_callbacks);
  spdlog::debug("Destroyed Vulkan surface");
  vkb::destroy_instance(instance_);
  spdlog::debug("Destroyed instance");
  spdlog::info("All RenderContext cleaned up successfully");
}
}  // namespace volume_restir
