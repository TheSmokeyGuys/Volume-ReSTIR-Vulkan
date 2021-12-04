#include "RenderContext.hpp"

#include <stdexcept>

#include "SingtonManager.hpp"
#include "spdlog/spdlog.h"

namespace volume_restir {

namespace {
QueueFamilyIndices checkDeviceQueueSupport(
    VkPhysicalDevice device, QueueFlagBits requiredQueues,
    VkSurfaceKHR surface = VK_NULL_HANDLE) {
  uint32_t queueFamilyCount = 0;
  vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, nullptr);

  std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
  vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount,
                                           queueFamilies.data());

  VkQueueFlags requiredVulkanQueues = 0;
  if (requiredQueues[QueueFlags::GRAPHICS]) {
    requiredVulkanQueues |= VK_QUEUE_GRAPHICS_BIT;
  }
  if (requiredQueues[QueueFlags::COMPUTE]) {
    requiredVulkanQueues |= VK_QUEUE_COMPUTE_BIT;
  }
  if (requiredQueues[QueueFlags::TRANSFER]) {
    requiredVulkanQueues |= VK_QUEUE_TRANSFER_BIT;
  }

  QueueFamilyIndices indices = {};
  indices.fill(-1);
  VkQueueFlags supportedQueues = 0;
  bool needsPresent            = requiredQueues[QueueFlags::PRESENT];
  bool presentSupported        = false;

  int i = 0;
  for (const auto& queueFamily : queueFamilies) {
    if (queueFamily.queueCount > 0) {
      supportedQueues |= queueFamily.queueFlags;
    }

    if (queueFamily.queueCount > 0 &&
        queueFamily.queueFlags & VK_QUEUE_GRAPHICS_BIT) {
      indices[QueueFlags::GRAPHICS] = i;
    }

    if (queueFamily.queueCount > 0 &&
        queueFamily.queueFlags & VK_QUEUE_COMPUTE_BIT) {
      indices[QueueFlags::COMPUTE] = i;
    }

    if (queueFamily.queueCount > 0 &&
        queueFamily.queueFlags & VK_QUEUE_TRANSFER_BIT) {
      indices[QueueFlags::TRANSFER] = i;
    }

    if (needsPresent) {
      VkBool32 presentSupport = false;
      vkGetPhysicalDeviceSurfaceSupportKHR(device, i, surface, &presentSupport);
      if (queueFamily.queueCount > 0 && presentSupport) {
        presentSupported             = true;
        indices[QueueFlags::PRESENT] = i;
      }
    }

    if ((requiredVulkanQueues & supportedQueues) == requiredVulkanQueues &&
        (!needsPresent || presentSupported)) {
      break;
    }

    i++;
  }

  return indices;
}
}  // namespace

RenderContext::RenderContext() {
  nvvk::ContextCreateInfo contextInfo(true);

  contextInfo.setVersion(1, 2);
  contextInfo.addInstanceLayer("VK_LAYER_LUNARG_monitor", true);
  contextInfo.addInstanceExtension(VK_KHR_SURFACE_EXTENSION_NAME);
  contextInfo.addInstanceExtension(VK_EXT_DEBUG_UTILS_EXTENSION_NAME, true);
#ifdef WIN32
  contextInfo.addInstanceExtension(VK_KHR_WIN32_SURFACE_EXTENSION_NAME);
#else
  contextInfo.addInstanceExtension(VK_KHR_XLIB_SURFACE_EXTENSION_NAME);
  contextInfo.addInstanceExtension(VK_KHR_XCB_SURFACE_EXTENSION_NAME);
#endif
  contextInfo.addInstanceExtension(
      VK_KHR_GET_PHYSICAL_DEVICE_PROPERTIES_2_EXTENSION_NAME);
  contextInfo.addDeviceExtension(VK_KHR_SWAPCHAIN_EXTENSION_NAME);
  contextInfo.addDeviceExtension(VK_KHR_DEDICATED_ALLOCATION_EXTENSION_NAME);
  contextInfo.addDeviceExtension(
      VK_KHR_GET_MEMORY_REQUIREMENTS_2_EXTENSION_NAME);
  // #VKRay: Activate the ray tracing extension
  contextInfo.addDeviceExtension(VK_KHR_MAINTENANCE3_EXTENSION_NAME);
  contextInfo.addDeviceExtension(VK_KHR_PIPELINE_LIBRARY_EXTENSION_NAME);
  contextInfo.addDeviceExtension(
      VK_KHR_DEFERRED_HOST_OPERATIONS_EXTENSION_NAME);
  contextInfo.addDeviceExtension(VK_KHR_BUFFER_DEVICE_ADDRESS_EXTENSION_NAME);
  contextInfo.addDeviceExtension(VK_KHR_SHADER_CLOCK_EXTENSION_NAME);
  vk::PhysicalDeviceAccelerationStructureFeaturesKHR accelFeature;
  contextInfo.addDeviceExtension(VK_KHR_ACCELERATION_STRUCTURE_EXTENSION_NAME,
                                 false, &accelFeature);

  // Check this
  vk::PhysicalDeviceRayTracingPipelineFeaturesKHR rtPipelineFeature;
  contextInfo.addDeviceExtension(VK_KHR_RAY_TRACING_PIPELINE_EXTENSION_NAME,
                                 false, &rtPipelineFeature);

  // TODO: Why ignoring these debug messages?
  // Creating Vulkan base application
  nvvk_context_.ignoreDebugMessage(0x99fb7dfd);  // dstAccelerationStructure
  nvvk_context_.ignoreDebugMessage(0x45e8716f);  // dstAccelerationStructure
  nvvk_context_.initInstance(contextInfo);
  // Find all compatible devices
  auto compatibleDevices = nvvk_context_.getCompatibleDevices(contextInfo);
  assert(!compatibleDevices.empty());
  // Use a compatible device
  nvvk_context_.initDevice(compatibleDevices[0], contextInfo);

  // create surface KHR
  VkSurfaceKHR surface;
  if (glfwCreateWindowSurface(nvvk_context_.m_instance,
                              SingletonManager::GetWindow().WindowPtr(),
                              nullptr, &surface) != VK_SUCCESS) {
    spdlog::error("Failed to create a Window surface");
    throw std::runtime_error("Failed to create a Window surface");
  }
  surface_ = surface;
  nvvk_context_.setGCTQueueWithPresent(surface_);

  device_      = nvvk_context_.m_device;
  instance_    = nvvk_context_.m_instance;
  phys_device_ = nvvk_context_.m_physicalDevice;

      vk::DynamicLoader dl;
  PFN_vkGetInstanceProcAddr vkGetInstanceProcAddr =
      dl.getProcAddress<PFN_vkGetInstanceProcAddr>("vkGetInstanceProcAddr");
  VULKAN_HPP_DEFAULT_DISPATCHER.init(vkGetInstanceProcAddr);
  VULKAN_HPP_DEFAULT_DISPATCHER.init(instance_);
  VULKAN_HPP_DEFAULT_DISPATCHER.init(device_);

  // create queues
  InitQueues();
}

RenderContext::~RenderContext() {
  nvvk_context_.deinit();
  spdlog::info("All RenderContext cleaned up successfully");
}

void RenderContext::InitQueues() {
  QueueFlagBits requiredQueues =
      vkq_index::kComputeIdx | vkq_index::kGraphicsIdx |
      vkq_index::kPresentIdx | vkq_index::kTransferIdx;

  queue_family_indices_ = checkDeviceQueueSupport(
      nvvk_context_.m_physicalDevice, requiredQueues, surface_);

  for (unsigned int i = 0; i < requiredQueues.size(); ++i) {
    if (requiredQueues[i]) {
      queues_[i] = device_.getQueue(queue_family_indices_[i], 0);
    }
  }
  spdlog::debug("Initialized all queues");
}

uint32_t RenderContext::MemoryTypeIndex(
    uint32_t type_bits, VkMemoryPropertyFlags properties) const {
  VkPhysicalDeviceMemoryProperties device_memory_properties =
      nvvk_context_.m_physicalInfo.memoryProperties;

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
