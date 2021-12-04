#pragma once
#include <nvmath/nvmath.h>
#include <nvmath/nvmath_glsltypes.h>

#include <queue>
#include <vulkan/vulkan.hpp>

#ifndef NVVK_ALLOC_DEDICATED
#define NVVK_ALLOC_DEDICATED
#include "nvvk/allocator_vk.hpp"
#endif


#include "nvh/gltfscene.hpp"
#include "nvvk/appbase_vkpp.hpp"
#include "nvvk/commands_vk.hpp"
#include "nvvk/descriptorsets_vk.hpp"
#include "nvvk/raytraceKHR_vk.hpp"
#include "shaders/headers/binding.glsl"
#include "utils/nvvkUtils.h"

namespace volume_restir {
class GLTFSceneBuffers {
public:
  nvvk::Buffer& getVertices() { return m_vertices; }

  nvvk::Buffer& getIndices() { return m_indices; }
  [[nodiscard]] const nvvk::Buffer& getMatrices() const { return m_matrices; }
  [[nodiscard]] const nvvk::Buffer& getPtLights() const {
    return m_ptLightsBuffer;
  }
  [[nodiscard]] const nvvk::Buffer& getTriLights() const {
    return m_triangleLightsBuffer;
  }
  [[nodiscard]] const nvvk::Buffer& getMaterials() const { return m_materials; }
  [[nodiscard]] const nvvk::Buffer& getAliasTable() const {
    return m_aliasTableBuffer;
  }
  [[nodiscard]] const std::vector<nvvk::Texture>& getTextures() const {
    return m_textures;
  }
  [[nodiscard]] const nvvk::Texture& getDefaultNormal() const {
    return m_defaultNormal;
  }
  [[nodiscard]] const nvvk::Texture& getDefaultWhite() const {
    return m_defaultWhite;
  }
  [[nodiscard]] const vk::DeviceSize& getPtLightsBufferSize() const {
    return m_ptLightsBufferSize;
  }
  [[nodiscard]] const vk::DeviceSize& getTriLightsBufferSize() const {
    return m_triangleLightsBufferSize;
  }
  [[nodiscard]] const vk::DeviceSize& getAliasTableBufferSize() const {
    return m_aliasTableBufferSize;
  }
  [[nodiscard]] const uint32_t& getPtLightsCount() const {
    return m_pointLightCount;
  }
  [[nodiscard]] const uint32_t& getTriLightsCount() const {
    return m_triangleLightCount;
  }
  [[nodiscard]] const uint32_t& getAliasTableCount() const {
    return m_aliasTableCount;
  }

  [[nodiscard]] const nvvk::Texture& getEnvironmentalTexture() const {
    return m_environmentalTexture;
  }
  [[nodiscard]] const nvvk::Texture& getEnvironmentalAliasMap() const {
    return m_environmentAliasMap;
  }

  vk::DescriptorSetLayout& getDescLayout() { return m_sceneDescSetLayout; }
  vk::DescriptorSet& getDescSet() { return m_sceneDescSet; }

  [[nodiscard]] void create(const nvh::GltfScene& gltfScene,
                            tinygltf::Model& tmodel, nvvk::Allocator* alloc,
                            const vk::Device& device,
                            const vk::PhysicalDevice& physicalDevice,
                            uint32_t graphicsQueueIndex);

  void createDescriptorSet(vk::DescriptorPool& staticDescPool);

  void destroy();

private:
  nvvk::DebugUtil m_debug;
  nvvk::AllocatorDedicated* m_alloc;
  vk::Device m_device;
  vk::PhysicalDevice m_physicalDevice;
  uint32_t m_graphicsQueueIndex;

  nvvk::Buffer m_vertices;
  nvvk::Buffer m_normals;
  nvvk::Buffer m_texcoords;
  nvvk::Buffer m_indices;
  nvvk::Buffer m_materials;
  nvvk::Buffer m_matrices;
  std::vector<nvvk::Texture> m_textures;
  nvvk::Buffer m_tangents;
  nvvk::Buffer m_colors;

  std::vector<shader::pointLight> m_pointLights;
  std::vector<shader::triangleLight> m_triangleLights;
  nvvk::Buffer m_ptLightsBuffer;
  nvvk::Buffer m_triangleLightsBuffer;
  nvvk::Buffer m_aliasTableBuffer;
  vk::DeviceSize m_ptLightsBufferSize;
  vk::DeviceSize m_triangleLightsBufferSize;
  vk::DeviceSize m_aliasTableBufferSize;
  uint32_t m_pointLightCount;
  uint32_t m_triangleLightCount;
  uint32_t m_aliasTableCount;

  nvvk::Texture m_defaultNormal;
  nvvk::Texture m_defaultWhite;

  nvvk::Texture m_environmentalTexture;
  nvvk::Texture m_environmentAliasMap;

  nvvk::RaytracingBuilderKHR m_rtBuilder;
  vk::PhysicalDeviceRayTracingPipelinePropertiesKHR m_rtProperties;
  nvvk::Buffer m_primlooks;

  vk::DescriptorSetLayout m_sceneDescSetLayout;
  vk::DescriptorSet m_sceneDescSet;
  vk::DescriptorPool m_sceneDescPool;

  [[nodiscard]] void _createRtBuffer(const nvh::GltfScene& gltfScene);

  [[nodiscard]] inline nvvk::RaytracingBuilderKHR::BlasInput
  _primitiveToGeometry(const vk::Device& device,
                       const nvh::GltfPrimMesh& prim) {
    // Building part
    vk::DeviceAddress vertexAddress =
        device.getBufferAddress({m_vertices.buffer});
    vk::DeviceAddress indexAddress =
        device.getBufferAddress({m_indices.buffer});

    vk::AccelerationStructureGeometryTrianglesDataKHR triangles;
    triangles.setVertexFormat(vk::Format::eR32G32B32Sfloat);
    triangles.setVertexData(vertexAddress);
    triangles.setVertexStride(sizeof(nvmath::vec3f));
    triangles.setIndexType(vk::IndexType::eUint32);
    triangles.setIndexData(indexAddress);
    triangles.setTransformData({});
    triangles.setMaxVertex(prim.vertexCount);

    // Setting up the build info of the acceleration
    vk::AccelerationStructureGeometryKHR asGeom;
    asGeom.setGeometryType(vk::GeometryTypeKHR::eTriangles);
    asGeom.setFlags(vk::GeometryFlagBitsKHR::eOpaque);
    asGeom.geometry.setTriangles(triangles);

    vk::AccelerationStructureBuildRangeInfoKHR offset;
    offset.setFirstVertex(prim.vertexOffset);
    offset.setPrimitiveCount(prim.indexCount / 3);
    offset.setPrimitiveOffset(prim.firstIndex * sizeof(uint32_t));
    offset.setTransformOffset(0);

    nvvk::RaytracingBuilderKHR::BlasInput input;
    input.asGeometry.emplace_back(asGeom);
    input.asBuildOffsetInfo.emplace_back(offset);
    return input;
  }

  vk::SamplerCreateInfo _gltfSamplerToVulkan(tinygltf::Sampler& tsampler);
  void _loadEnvironment();
};

}  // namespace volume_restir
