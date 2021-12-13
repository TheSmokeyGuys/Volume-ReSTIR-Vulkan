#include "restirPass.h"

#include "nvh/fileoperations.hpp"
#include "nvvk/pipeline_vk.hpp"
#include "nvvk/renderpasses_vk.hpp"
#include "nvvk/shaders_vk.hpp"

extern std::vector<std::string> defaultSearchPaths;

void RestirPass::run(const VkCommandBuffer& cmdBuf,
                     const VkDescriptorSet& rtDescSet,
                     const VkDescriptorSet& descSet,
                     const VkDescriptorSet& uniformDescSet,
                     const VkDescriptorSet& lightDescSet,
                     const VkDescriptorSet& restirDescSet,
                     const nvmath::vec4f& clearColor) {
  vkCmdPipelineBarrier(cmdBuf, VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
                       VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
                       VK_DEPENDENCY_DEVICE_GROUP_BIT, 0, nullptr, 0, nullptr,
                       0, nullptr);

  // Initializing push constant values
  m_pcRestir.clearColor = clearColor;

  vkCmdBindPipeline(cmdBuf, VK_PIPELINE_BIND_POINT_RAY_TRACING_KHR, m_pipeline);

  std::vector<VkDescriptorSet> descriptorSets{
      rtDescSet, descSet, uniformDescSet, lightDescSet, restirDescSet};
  vkCmdBindDescriptorSets(cmdBuf, VK_PIPELINE_BIND_POINT_RAY_TRACING_KHR,
                          m_pipelineLayout, 0,
                          static_cast<uint32_t>(descriptorSets.size()),
                          descriptorSets.data(), 0, nullptr);

  // Size of a program identifier
  uint32_t groupSize =
      nvh::align_up(m_restirProperties.shaderGroupHandleSize,
                    m_restirProperties.shaderGroupBaseAlignment);
  uint32_t groupStride = groupSize;
  VkBufferDeviceAddressInfo info{VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO,
                                 nullptr, m_SBTBuffer.buffer};
  VkDeviceAddress sbtAddress = vkGetBufferDeviceAddress(m_device, &info);

  // using Stride = vk::StridedDeviceAddressRegionKHR;
  // std::array<Stride, 4> strideAddresses{
  //	Stride{sbtAddress + 0u * groupSize, groupStride, groupSize * 1},  //
  // raygen 	Stride{sbtAddress + 1u * groupSize, groupStride, groupSize * 2},
  // // miss 	Stride{sbtAddress + 3u * groupSize, groupStride, groupSize * 1},
  // // hit 	Stride{0u, 0u, 0u} };      // callable

  vkCmdPushConstants(cmdBuf, m_pipelineLayout,
                     VK_SHADER_STAGE_RAYGEN_BIT_KHR |
                         VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR |
                         VK_SHADER_STAGE_MISS_BIT_KHR,
                     0, sizeof(PushConstantRestir), &m_pcRestir);
  vkCmdTraceRaysKHR(cmdBuf, &m_rgenRegion, &m_missRegion, &m_hitRegion,
                    &m_callRegion, m_size.width, m_size.height, 1);
}

void RestirPass::setup(const VkDevice& device,
                       const VkPhysicalDevice& physicalDevice,
                       uint32_t graphicsQueueIndex,
                       nvvk::ResourceAllocatorDma* allocator) {
  m_device             = device;
  m_graphicsQueueIndex = graphicsQueueIndex;
  m_physicalDevice     = physicalDevice;
  m_alloc              = allocator;

  // Requesting ray tracing properties
  VkPhysicalDeviceProperties2 prop2{
      VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROPERTIES_2};
  prop2.pNext = &m_restirProperties;
  vkGetPhysicalDeviceProperties2(m_physicalDevice, &prop2);
}

void RestirPass::createDescriptorSet() {}

void RestirPass::createRenderPass(VkExtent2D outputSize) {
  m_size = outputSize;
}

void RestirPass::createPipeline(
    const VkDescriptorSetLayout& rtDescSetLayout,
    const VkDescriptorSetLayout& descSetLayout,
    const VkDescriptorSetLayout& uniformDescSetLayout,
    const VkDescriptorSetLayout& lightDescSetLayout,
    const VkDescriptorSetLayout& restirDescSetLayout) {
  enum StageIndices {
    eRaygen,
    eMiss,
    eMiss2,
    eClosestHit,
    eClosestHit2,
    eIntersection,
    eShaderGroupCount
  };

  std::vector<std::string> paths = defaultSearchPaths;

  // All stages
  std::array<VkPipelineShaderStageCreateInfo, eShaderGroupCount> stages{};
  VkPipelineShaderStageCreateInfo stage{
      VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO};
  stage.pName = "main";  // All the same entry point
  // Raygen
  stage.module = nvvk::createShaderModule(
      m_device, nvh::loadFile("spv/restir.rgen.spv", true, paths, true));
  stage.stage     = VK_SHADER_STAGE_RAYGEN_BIT_KHR;
  stages[eRaygen] = stage;
  // Miss
  stage.module = nvvk::createShaderModule(
      m_device, nvh::loadFile("spv/restir.rmiss.spv", true, paths, true));
  stage.stage   = VK_SHADER_STAGE_MISS_BIT_KHR;
  stages[eMiss] = stage;
  // The second miss shader is invoked when a shadow ray misses the geometry. It
  // simply indicates that no occlusion has been found
  stage.module = nvvk::createShaderModule(
      m_device, nvh::loadFile("spv/restirShadow.rmiss.spv", true, paths, true));
  stage.stage    = VK_SHADER_STAGE_MISS_BIT_KHR;
  stages[eMiss2] = stage;
  // Hit Group - Closest Hit
  stage.module = nvvk::createShaderModule(
      m_device, nvh::loadFile("spv/restir.rchit.spv", true, paths, true));
  stage.stage         = VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR;
  stages[eClosestHit] = stage;
  // Closest hit
  stage.module = nvvk::createShaderModule(
      m_device, nvh::loadFile("spv/restir2.rchit.spv", true, paths, true));
  stage.stage          = VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR;
  stages[eClosestHit2] = stage;
  // Intersection
  stage.module = nvvk::createShaderModule(
      m_device, nvh::loadFile("spv/raytrace.rint.spv", true, paths, true));
  stage.stage           = VK_SHADER_STAGE_INTERSECTION_BIT_KHR;
  stages[eIntersection] = stage;

  // Shader groups
  VkRayTracingShaderGroupCreateInfoKHR group{
      VK_STRUCTURE_TYPE_RAY_TRACING_SHADER_GROUP_CREATE_INFO_KHR};
  group.anyHitShader       = VK_SHADER_UNUSED_KHR;
  group.closestHitShader   = VK_SHADER_UNUSED_KHR;
  group.generalShader      = VK_SHADER_UNUSED_KHR;
  group.intersectionShader = VK_SHADER_UNUSED_KHR;

  // Raygen group record
  group.type          = VK_RAY_TRACING_SHADER_GROUP_TYPE_GENERAL_KHR;
  group.generalShader = eRaygen;
  m_rtShaderGroups.push_back(group);

  // Miss group record
  group.type          = VK_RAY_TRACING_SHADER_GROUP_TYPE_GENERAL_KHR;
  group.generalShader = eMiss;
  m_rtShaderGroups.push_back(group);

  // Miss group record
  group.type          = VK_RAY_TRACING_SHADER_GROUP_TYPE_GENERAL_KHR;
  group.generalShader = eMiss2;
  m_rtShaderGroups.push_back(group);

  // Hit group record
  group.type = VK_RAY_TRACING_SHADER_GROUP_TYPE_TRIANGLES_HIT_GROUP_KHR;
  group.generalShader    = VK_SHADER_UNUSED_KHR;
  group.closestHitShader = eClosestHit;
  m_rtShaderGroups.push_back(group);

  // Hit group record
  group.type = VK_RAY_TRACING_SHADER_GROUP_TYPE_PROCEDURAL_HIT_GROUP_KHR;
  group.closestHitShader   = eClosestHit2;
  group.intersectionShader = eIntersection;
  m_rtShaderGroups.push_back(group);

  // Push constant: we want to be able to update constants used by the shaders
  VkPushConstantRange pushConstant{VK_SHADER_STAGE_RAYGEN_BIT_KHR |
                                       VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR |
                                       VK_SHADER_STAGE_MISS_BIT_KHR,
                                   0, sizeof(PushConstantRestir)};

  VkPipelineLayoutCreateInfo pipelineLayoutCreateInfo{
      VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO};

  std::vector<VkDescriptorSetLayout> rtDescSetLayouts{
      rtDescSetLayout, descSetLayout, uniformDescSetLayout, lightDescSetLayout,
      restirDescSetLayout};
  pipelineLayoutCreateInfo.setLayoutCount =
      static_cast<uint32_t>(rtDescSetLayouts.size());
  pipelineLayoutCreateInfo.pSetLayouts            = rtDescSetLayouts.data();
  pipelineLayoutCreateInfo.pushConstantRangeCount = 1;
  pipelineLayoutCreateInfo.pPushConstantRanges    = &pushConstant;
  vkCreatePipelineLayout(m_device, &pipelineLayoutCreateInfo, nullptr,
                         &m_pipelineLayout);

  // Assemble the shader stages and recursion depth info into the ray tracing
  // pipeline
  VkRayTracingPipelineCreateInfoKHR rayPipelineInfo{
      VK_STRUCTURE_TYPE_RAY_TRACING_PIPELINE_CREATE_INFO_KHR};
  rayPipelineInfo.stageCount =
      static_cast<uint32_t>(stages.size());  // Stages are shaders
  rayPipelineInfo.pStages = stages.data();

  rayPipelineInfo.groupCount = static_cast<uint32_t>(m_rtShaderGroups.size());
  rayPipelineInfo.pGroups    = m_rtShaderGroups.data();

  rayPipelineInfo.maxPipelineRayRecursionDepth = 2;  // Ray depth
  rayPipelineInfo.layout                       = m_pipelineLayout;
  vkCreateRayTracingPipelinesKHR(m_device, {}, {}, 1, &rayPipelineInfo, nullptr,
                                 &m_pipeline);

  for (auto& s : stages) vkDestroyShaderModule(m_device, s.module, nullptr);

  createShaderBindingTable();
}

void RestirPass::createShaderBindingTable() {
  uint32_t missCount{2};
  uint32_t hitCount{2};
  auto handleCount    = 1 + missCount + hitCount;
  uint32_t handleSize = m_restirProperties.shaderGroupHandleSize;

  // The SBT (buffer) need to have starting groups to be aligned and handles in
  // the group to be aligned.
  uint32_t handleSizeAligned =
      nvh::align_up(handleSize, m_restirProperties.shaderGroupHandleAlignment);

  m_rgenRegion.stride = nvh::align_up(
      handleSizeAligned, m_restirProperties.shaderGroupBaseAlignment);
  m_rgenRegion.size =
      m_rgenRegion.stride;  // The size member of pRayGenShaderBindingTable must
                            // be equal to its stride member
  m_missRegion.stride = handleSizeAligned;
  m_missRegion.size =
      nvh::align_up(missCount * handleSizeAligned,
                    m_restirProperties.shaderGroupBaseAlignment);
  m_hitRegion.stride = handleSizeAligned;
  m_hitRegion.size   = nvh::align_up(hitCount * handleSizeAligned,
                                   m_restirProperties.shaderGroupBaseAlignment);

  // Get the shader group handles
  uint32_t dataSize = handleCount * handleSize;
  std::vector<uint8_t> handles(dataSize);
  auto result = vkGetRayTracingShaderGroupHandlesKHR(
      m_device, m_pipeline, 0, handleCount, dataSize, handles.data());
  assert(result == VK_SUCCESS);

  // Allocate a buffer for storing the SBT.
  VkDeviceSize sbtSize = m_rgenRegion.size + m_missRegion.size +
                         m_hitRegion.size + m_callRegion.size;
  m_SBTBuffer =
      m_alloc->createBuffer(sbtSize,
                            VK_BUFFER_USAGE_TRANSFER_SRC_BIT |
                                VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT |
                                VK_BUFFER_USAGE_SHADER_BINDING_TABLE_BIT_KHR,
                            VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
                                VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
  // m_debug.setObjectName(m_rtSBTBuffer.buffer, std::string("SBT"));  // Give
  // it a debug name for NSight.

  // Find the SBT addresses of each group
  VkBufferDeviceAddressInfo info{VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO,
                                 nullptr, m_SBTBuffer.buffer};
  VkDeviceAddress sbtAddress = vkGetBufferDeviceAddress(m_device, &info);
  m_rgenRegion.deviceAddress = sbtAddress;
  m_missRegion.deviceAddress = sbtAddress + m_rgenRegion.size;
  m_hitRegion.deviceAddress =
      sbtAddress + m_rgenRegion.size + m_missRegion.size;

  // Helper to retrieve the handle data
  auto getHandle = [&](int i) { return handles.data() + i * handleSize; };

  // Map the SBT buffer and write in the handles.
  auto* pSBTBuffer = reinterpret_cast<uint8_t*>(m_alloc->map(m_SBTBuffer));
  uint8_t* pData{nullptr};
  uint32_t handleIdx{0};
  // Raygen
  pData = pSBTBuffer;
  memcpy(pData, getHandle(handleIdx++), handleSize);
  // Miss
  pData = pSBTBuffer + m_rgenRegion.size;
  for (uint32_t c = 0; c < missCount; c++) {
    memcpy(pData, getHandle(handleIdx++), handleSize);
    pData += m_missRegion.stride;
  }
  // Hit
  pData = pSBTBuffer + m_rgenRegion.size + m_missRegion.size;
  for (uint32_t c = 0; c < hitCount; c++) {
    memcpy(pData, getHandle(handleIdx++), handleSize);
    pData += m_hitRegion.stride;
  }

  m_alloc->unmap(m_SBTBuffer);
  m_alloc->finalizeAndReleaseStaging();
}

void RestirPass::destroy() {
  if (m_renderPass != VK_NULL_HANDLE)
    vkDestroyRenderPass(m_device, m_renderPass, nullptr);
  if (m_pipeline != VK_NULL_HANDLE)
    vkDestroyPipeline(m_device, m_pipeline, nullptr);
  if (m_pipelineLayout != VK_NULL_HANDLE)
    vkDestroyPipelineLayout(m_device, m_pipelineLayout, nullptr);
  if (m_SBTBuffer.buffer != VK_NULL_HANDLE) m_alloc->destroy(m_SBTBuffer);
}
