#pragma once

#include "config/static_config.hpp"
#include "nvh/alignment.hpp"
#include "nvh/fileoperations.hpp"
#include "nvvk/memallocator_dma_vk.hpp"
#include "nvvk/raytraceKHR_vk.hpp"
#include "nvvk/resourceallocator_vk.hpp"
#include "nvvk/shaders_vk.hpp"
#include "shaders/host_device.h"
#include "utils/restir_utils.h"

class RestirPass {
public:
  void setup(const VkDevice& device, const VkPhysicalDevice&,
             uint32_t graphicsQueueIndex,
             nvvk::ResourceAllocatorDma* allocator);

  void createDescriptorSet();
  void createRenderPass(VkExtent2D outputSize);
  void createPipeline(const VkDescriptorSetLayout& rtDescSetLayout,
                      const VkDescriptorSetLayout& descSetLayout,
                      const VkDescriptorSetLayout& uniformDescSetLayout,
                      const VkDescriptorSetLayout& lightDescSetLayout,
                      const VkDescriptorSetLayout& restirDescSetLayout);

  bool uiSetup(){};
  void run(const VkCommandBuffer& cmdBuf, const VkDescriptorSet& rtDescSet,
           const VkDescriptorSet& descSet,
           const VkDescriptorSet& uniformDescSet,
           const VkDescriptorSet& lightDescSet,
           const VkDescriptorSet& restirDescSet,
           const nvmath::vec4f& clearColor);

  void destroy();

private:
  VkDevice m_device;
  VkPhysicalDevice m_physicalDevice;
  uint32_t m_graphicsQueueIndex;
  nvvk::ResourceAllocatorDma* m_alloc;
  VkExtent2D m_size;

  VkPhysicalDeviceRayTracingPipelinePropertiesKHR m_restirProperties{
      VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_RAY_TRACING_PIPELINE_PROPERTIES_KHR};
  std::vector<VkRayTracingShaderGroupCreateInfoKHR> m_rtShaderGroups;
  VkPipelineLayout m_pipelineLayout{VK_NULL_HANDLE};
  VkPipeline m_pipeline{VK_NULL_HANDLE};

  VkRenderPass m_renderPass{VK_NULL_HANDLE};

  const nvh::GltfScene* m_scene = nullptr;

  // push constant for restir
  PushConstantRestir m_pcRestir{0.f, 0.f, 0.f, 0, 1};

  nvvk::Buffer m_SBTBuffer;
  VkStridedDeviceAddressRegionKHR m_rgenRegion{};
  VkStridedDeviceAddressRegionKHR m_missRegion{};
  VkStridedDeviceAddressRegionKHR m_hitRegion{};
  VkStridedDeviceAddressRegionKHR m_callRegion{};

  void createShaderBindingTable();
};