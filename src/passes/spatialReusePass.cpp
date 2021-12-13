#include "spatialReusePass.h"

#include "nvh/fileoperations.hpp"
#include "nvvk/pipeline_vk.hpp"
#include "nvvk/renderpasses_vk.hpp"
#include "nvvk/shaders_vk.hpp"

extern std::vector<std::string> defaultSearchPaths;

void SpatialReusePass::run(const VkCommandBuffer& cmdBuf,
                           const VkDescriptorSet& rtDescSet,
                           const VkDescriptorSet& descSet,
                           const VkDescriptorSet& uniformDescSet,
                           const VkDescriptorSet& lightDescSet,
                           const VkDescriptorSet& restirDescSet) {
  vkCmdPipelineBarrier(cmdBuf, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
                       VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
                       VK_DEPENDENCY_DEVICE_GROUP_BIT, 0, nullptr, 0, nullptr,
                       0, nullptr);
  vkCmdBindPipeline(cmdBuf, VK_PIPELINE_BIND_POINT_COMPUTE, m_pipeline);

  std::vector<VkDescriptorSet> descriptorSets{
      rtDescSet, descSet, uniformDescSet, lightDescSet, restirDescSet};
  vkCmdBindDescriptorSets(cmdBuf, VK_PIPELINE_BIND_POINT_COMPUTE,
                          m_pipelineLayout, 0,
                          static_cast<uint32_t>(descriptorSets.size()),
                          descriptorSets.data(), 0, nullptr);
  vkCmdDispatch(cmdBuf, m_size.width, m_size.height, 1);
}

void SpatialReusePass::setup(const VkDevice& device,
                             const VkPhysicalDevice& physicalDevice,
                             uint32_t graphicsQueueIndex,
                             nvvk::ResourceAllocatorDma* allocator) {
  m_device             = device;
  m_graphicsQueueIndex = graphicsQueueIndex;
  m_physicalDevice     = physicalDevice;
  m_alloc              = allocator;
}

void SpatialReusePass::createRenderPass(VkExtent2D outputSize) {
  m_size = outputSize;
}

void SpatialReusePass::createPipeline(
    const VkDescriptorSetLayout& rtDescSetLayout,
    const VkDescriptorSetLayout& descSetLayout,
    const VkDescriptorSetLayout& uniformDescSetLayout,
    const VkDescriptorSetLayout& lightDescSetLayout,
    const VkDescriptorSetLayout& restirDescSetLayout) {
  std::vector<std::string> paths = defaultSearchPaths;

  // pushing time
  VkPushConstantRange push_constants = {VK_SHADER_STAGE_COMPUTE_BIT, 0,
                                        sizeof(float)};

  VkPipelineLayoutCreateInfo layout_info{
      VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO};
  std::vector<VkDescriptorSetLayout> setlayouts{
      rtDescSetLayout, descSetLayout, uniformDescSetLayout, lightDescSetLayout,
      restirDescSetLayout};
  layout_info.setLayoutCount         = static_cast<uint32_t>(setlayouts.size());
  layout_info.pSetLayouts            = setlayouts.data();
  layout_info.pushConstantRangeCount = 1;
  layout_info.pPushConstantRanges    = &push_constants;
  vkCreatePipelineLayout(m_device, &layout_info, nullptr, &m_pipelineLayout);

  VkComputePipelineCreateInfo computePipelineCreateInfo{
      VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO};
  computePipelineCreateInfo.layout = m_pipelineLayout;
  computePipelineCreateInfo.stage =
      nvvk::createShaderStageInfo(m_device,
                                  nvh::loadFile("spv/spatialReuse.comp.spv",
                                                true, defaultSearchPaths, true),
                                  VK_SHADER_STAGE_COMPUTE_BIT);
  vkCreateComputePipelines(m_device, VK_NULL_HANDLE, 1,
                           &computePipelineCreateInfo, nullptr, &m_pipeline);

  vkDestroyShaderModule(m_device, computePipelineCreateInfo.stage.module,
                        nullptr);
}

void SpatialReusePass::destroy() {
  if (m_renderPass != VK_NULL_HANDLE)
    vkDestroyRenderPass(m_device, m_renderPass, nullptr);
  if (m_pipeline != VK_NULL_HANDLE)
    vkDestroyPipeline(m_device, m_pipeline, nullptr);
  if (m_pipelineLayout != VK_NULL_HANDLE)
    vkDestroyPipelineLayout(m_device, m_pipelineLayout, nullptr);
}