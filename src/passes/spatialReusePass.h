#pragma once

#include "nvh/alignment.hpp"
#include "nvh/fileoperations.hpp"
#include "nvvk/memallocator_dma_vk.hpp"
#include "nvvk/raytraceKHR_vk.hpp"
#include "nvvk/resourceallocator_vk.hpp"
#include "nvvk/shaders_vk.hpp"
#include "utils/restir_utils.h"
//#include "GBuffer.hpp"

class SpatialReusePass {
public:
  void setup(const VkDevice& device, const VkPhysicalDevice&,
             uint32_t graphicsQueueIndex,
             nvvk::ResourceAllocatorDma* allocator);

  void createDescriptorSet(){};
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
           const VkDescriptorSet& restirDescSet);

  void destroy();

private:
  VkDevice m_device;
  VkPhysicalDevice m_physicalDevice;
  uint32_t m_graphicsQueueIndex;
  nvvk::ResourceAllocatorDma* m_alloc;
  VkExtent2D m_size;

  VkPipelineLayout m_pipelineLayout{VK_NULL_HANDLE};
  VkPipeline m_pipeline{VK_NULL_HANDLE};

  VkRenderPass m_renderPass{VK_NULL_HANDLE};

  const nvh::GltfScene* m_scene = nullptr;
};