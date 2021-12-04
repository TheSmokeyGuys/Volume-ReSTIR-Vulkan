#include "spatialReusePass.h"

#include "nvh/fileoperations.hpp"
#include "nvvk/pipeline_vk.hpp"
#include "nvvk/renderpasses_vk.hpp"
#include "nvvk/shaders_vk.hpp"

namespace volume_restir {

extern std::vector<std::string> defaultSearchPaths;

void SpatialReusePass::run(const vk::CommandBuffer& cmdBuf,
                           const vk::DescriptorSet& sceneDescSet,
                           const vk::DescriptorSet& lightDescSet,
                           const vk::DescriptorSet& restirDescSet) {
  cmdBuf.pipelineBarrier(vk::PipelineStageFlagBits::eComputeShader,
                         vk::PipelineStageFlagBits::eComputeShader, {}, {}, {},
                         {});

  cmdBuf.bindPipeline(vk::PipelineBindPoint::eCompute, m_pipeline);
  cmdBuf.bindDescriptorSets(vk::PipelineBindPoint::eCompute, m_pipelineLayout,
                            0, {sceneDescSet, lightDescSet, restirDescSet}, {});
  cmdBuf.dispatch(m_size.width, m_size.height, 1);
}

void SpatialReusePass::setup(const vk::Device& device,
                             const vk::PhysicalDevice& physicalDevice,
                             uint32_t graphicsQueueIndex,
                             nvvk::Allocator* allocator) {
  m_device             = device;
  m_graphicsQueueIndex = graphicsQueueIndex;
  m_physicalDevice     = physicalDevice;
  m_alloc              = allocator;
}

void SpatialReusePass::createRenderPass(vk::Extent2D outputSize) {
  m_size = outputSize;
}

void SpatialReusePass::createPipeline(
    const vk::DescriptorSetLayout& sceneDescSetLayout,
    const vk::DescriptorSetLayout& lightDescSetLayout,
    const vk::DescriptorSetLayout& restirDescSetLayout) {
  std::vector<std::string> paths = defaultSearchPaths;

  // pushing time
  vk::PushConstantRange push_constants = {vk::ShaderStageFlagBits::eCompute, 0,
                                          sizeof(float)};
  vk::PipelineLayoutCreateInfo layout_info;
  std::vector<vk::DescriptorSetLayout> setlayouts{
      sceneDescSetLayout, lightDescSetLayout, restirDescSetLayout};
  layout_info.setSetLayouts(setlayouts);
  m_pipelineLayout = m_device.createPipelineLayout(layout_info);
  vk::ComputePipelineCreateInfo computePipelineCreateInfo{
      {}, {}, m_pipelineLayout};

  computePipelineCreateInfo.stage = nvvk::createShaderStageInfo(
      m_device,
      nvh::loadFile("src/shaders/spatialReuse.comp.spv", true,
                    defaultSearchPaths, true),
      VK_SHADER_STAGE_COMPUTE_BIT);
  m_pipeline = static_cast<const vk::Pipeline&>(
      m_device.createComputePipeline({}, computePipelineCreateInfo));
  m_device.destroy(computePipelineCreateInfo.stage.module);
}

void SpatialReusePass::destroy() {
  m_device.destroy(m_renderPass);
  m_device.destroy(m_pipeline);
  m_device.destroy(m_pipelineLayout);
}

}  // namespace volume_restir
