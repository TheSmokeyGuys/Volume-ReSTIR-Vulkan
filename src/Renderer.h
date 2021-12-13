/*
 * Copyright (c) 2014-2021, NVIDIA CORPORATION.  All rights reserved.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 * SPDX-FileCopyrightText: Copyright (c) 2014-2021 NVIDIA CORPORATION
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "GBuffer.hpp"
#include "SingletonManager.hpp"
#include "config/static_config.hpp"
#include "nvvk/appbase_vk.hpp"
#include "nvvk/debug_util_vk.hpp"
#include "nvvk/descriptorsets_vk.hpp"
#include "nvvk/memallocator_dma_vk.hpp"
#include "nvvk/resourceallocator_vk.hpp"
#include "passes/restirPass.h"
#include "passes/spatialReusePass.h"
#include "shaders/host_device.h"
// #VKRay
#include "nvvk/raytraceKHR_vk.hpp"

//--------------------------------------------------------------------------------------------------
// Simple rasterizer of OBJ objects
// - Each OBJ loaded are stored in an `ObjModel` and referenced by a
// `ObjInstance`
// - It is possible to have many `ObjInstance` referencing the same `ObjModel`
// - Rendering is done in an offscreen framebuffer
// - The image of the framebuffer is displayed in post-process in a full-screen
// quad
//
class Renderer : public nvvk::AppBaseVk {
public:
  // The OBJ model
  struct ObjModel {
    uint32_t nbIndices{0};
    uint32_t nbVertices{0};
    nvvk::Buffer vertexBuffer;  // Device buffer of all 'Vertex'
    nvvk::Buffer indexBuffer;  // Device buffer of the indices forming triangles
    nvvk::Buffer
        matColorBuffer;  // Device buffer of array of 'Wavefront material'
    nvvk::Buffer
        matIndexBuffer;  // Device buffer of array of 'Wavefront material'
  };

  struct ObjInstance {
    nvmath::mat4f transform;  // Matrix of the instance
    uint32_t objIndex{0};     // Model index reference
  };

  void setup(const VkInstance& instance, const VkDevice& device,
             const VkPhysicalDevice& physicalDevice,
             uint32_t queueFamily) override;
  void createDescriptorSetLayout();
  void createGraphicsPipeline();
  void loadModel(const std::string& filename,
                 nvmath::mat4f transform = nvmath::mat4f(1));
  void updateDescriptorSet();
  void createUniformBuffer();
  void createObjDescriptionBuffer();
  void createTextureImages(const VkCommandBuffer& cmdBuf,
                           const std::vector<std::string>& textures);
  void updateUniformBuffer(const VkCommandBuffer& cmdBuf);
  void onResize(int /*w*/, int /*h*/) override;
  void destroyResources();
  void rasterize(const VkCommandBuffer& cmdBuff);

  // #Post - Draw the rendered image on a quad using a tonemapper
  void createOffscreenRender();
  void createPostPipeline();
  void createPostDescriptor();
  void updatePostDescriptorSet();
  void drawPost(VkCommandBuffer cmdBuf);

  // #VKRay
  void initRayTracing();
  auto objectToVkGeometryKHR(const ObjModel& model);
  void createBottomLevelAS();
  void createTopLevelAS();
  void createRtDescriptorSet();
  void updateRtDescriptorSet();
  void createRtPipeline();
  void createRtShaderBindingTable();
  void raytrace(const VkCommandBuffer& cmdBuf, const nvmath::vec4f& clearColor);

  // objects
  void loadGLTFModel(const std::string& filename);
  void createGLTFBuffer();
  void createVDBBuffer();
  auto sphereToVkGeometryKHR();

  // restir lights
  void createRestirLights();
  void createLightDescriptorSet();

  // restir uniform buffers
  void createRestirUniformBuffer();
  void updateRestirUniformBuffer(const VkCommandBuffer& cmdBuf);
  void createRestirUniformDescriptorSet();
  void updateRestirUniformDescriptorSet();

  // restir reservoirs
  void createRestirBuffer();
  void createRestirDescriptorSet();
  void updateRestirDescriptorSet();

  void createRestirPipeline();
  void createSpatialReusePipeline();

  // Post descriptor set for ReSTIR
  void createRestirPostDescriptor();
  void updateRestirPostDescriptorSet();
  void createRestirPostPipeline();
  void restirDrawPost(VkCommandBuffer cmdBuf);

  // GBuffers
  void createGBuffers();
  void updateGBufferFrameIdx();
  uint32_t getCurrentFrameIdx() { return m_currentGBufferFrameIdx; }

  // getters and setters
  std::vector<Sphere>& getSpheres() { return m_spheres; }
  PushConstantRaster& getPushConstant() { return m_pcRaster; }
  VkRenderPass& getOffscreenRenderPass() { return m_offscreenRenderPass; }
  VkFramebuffer& getOffscreenFrameBuffer() { return m_offscreenFramebuffer; }
  RestirPass& getRestirPass() { return m_restirPass; }
  SpatialReusePass& getSpatialReusePass() { return m_spatialReusePass; }
  VkDescriptorSet& getDescSet() { return m_descSet; }
  VkDescriptorSet& getRtDescSet() { return m_rtDescSet; }
  VkDescriptorSet& getRestirUniformDescSet() { return m_restirUniformDescSet; }
  VkDescriptorSet& getLightDescSet() { return m_lightDescSet; }
  std::vector<VkDescriptorSet>& getRestirDescSets() { return m_restirDescSets; }
  VkDescriptorSet& getRestirDescSet() {
    return m_restirDescSets[getCurrentFrameIdx()];
  }

private:
  // Information pushed at each draw call
  PushConstantRaster m_pcRaster{
      {1},                // Identity matrix
      {10.f, 55.f, 8.f},  // light position
      0,                  // instance Id
      1000.f,             // light intensity
      0                   // light type
  };

  // Array of objects and instances in the scene
  std::vector<ObjModel> m_objModel;      // Model on host
  std::vector<ObjDesc> m_objDesc;        // Model description for device access
  std::vector<ObjInstance> m_instances;  // Scene model instances

  // Graphic pipeline
  VkPipelineLayout m_pipelineLayout;
  VkPipeline m_graphicsPipeline;

  nvvk::Buffer m_bGlobals;  // Device-Host of the camera matrices
  nvvk::Buffer m_bObjDesc;  // Device buffer of the OBJ descriptions

  std::vector<nvvk::Texture> m_textures;  // vector of all textures of the scene

  // Buffer of GLTF objects and instances in the scene
  nvvk::Buffer m_gltfVertices;
  nvvk::Buffer m_gltfIndices;
  nvvk::Buffer m_gltfNormals;
  nvvk::Buffer m_gltfTexcoords;
  nvvk::Buffer m_gltfTangents;
  nvvk::Buffer m_gltfColors;
  nvvk::Buffer m_gltfMaterials;
  nvvk::Buffer m_gltfMatrices;
  nvvk::Buffer m_gltfPrimLookup;
  std::vector<nvvk::Texture> m_gltfTextures;

  nvvk::ResourceAllocatorDma
      m_alloc;  // Allocator for buffer, images, acceleration structures
  nvvk::DebugUtil m_debug;  // Utility to name objects

  VkPipeline m_postPipeline{VK_NULL_HANDLE};
  VkPipelineLayout m_postPipelineLayout{VK_NULL_HANDLE};
  VkRenderPass m_offscreenRenderPass{VK_NULL_HANDLE};
  VkFramebuffer m_offscreenFramebuffer{VK_NULL_HANDLE};
  nvvk::Texture m_offscreenColor;  // FIXME: output img buffer from RtPipeline
  nvvk::Texture m_offscreenDepth;
  VkFormat m_offscreenColorFormat{VK_FORMAT_R32G32B32A32_SFLOAT};
  VkFormat m_offscreenDepthFormat{VK_FORMAT_X8_D24_UNORM_PACK32};

  VkPhysicalDeviceRayTracingPipelinePropertiesKHR m_rtProperties{
      VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_RAY_TRACING_PIPELINE_PROPERTIES_KHR};
  nvvk::RaytracingBuilderKHR m_rtBuilder;

  std::vector<VkRayTracingShaderGroupCreateInfoKHR> m_rtShaderGroups;
  VkPipelineLayout m_rtPipelineLayout;
  VkPipeline m_rtPipeline;

  // GBuffers
  std::array<GBuffer, static_config::kNumGBuffers> m_gBuffers;
  uint32_t m_currentGBufferFrameIdx{0};

  // pass
  RestirPass m_restirPass;
  SpatialReusePass m_spatialReusePass;

  // restir uniforms
  RestirUniforms m_restirUniforms;
  nvvk::Buffer m_restirUniformBuffer;

  // restir lights
  std::vector<PointLight> m_pointLights;
  std::vector<TriangleLight> m_triangleLights;
  uint32_t m_aliasTableCount;
  nvvk::Buffer m_ptLightsBuffer;
  nvvk::Buffer m_triangleLightsBuffer;
  nvvk::Buffer m_aliasTableBuffer;

  // restir reservoirs
  std::vector<nvvk::Texture> m_reservoirInfoBuffers;
  std::vector<nvvk::Texture> m_reservoirWeightBuffers;
  nvvk::Texture m_reservoirTmpInfoBuffer;
  nvvk::Texture m_reservoirTmpWeightBuffer;
  // FIXME: output img buffer from `Restir`Pipeline
  //  may have to combine it with m_offscreenColor
  nvvk::Texture m_storageImage;

  // restir post pipelines
  VkPipeline m_restirPostPipeline{VK_NULL_HANDLE};
  VkPipelineLayout m_restirPostPipelineLayout{VK_NULL_HANDLE};

  nvvk::DescriptorSetBindings m_descSetLayoutBind;
  VkDescriptorPool m_descPool{VK_NULL_HANDLE};
  VkDescriptorSetLayout m_descSetLayout{VK_NULL_HANDLE};
  VkDescriptorSet m_descSet{VK_NULL_HANDLE};

  nvvk::DescriptorSetBindings m_postDescSetLayoutBind;
  VkDescriptorPool m_postDescPool{VK_NULL_HANDLE};
  VkDescriptorSetLayout m_postDescSetLayout{VK_NULL_HANDLE};
  VkDescriptorSet m_postDescSet{VK_NULL_HANDLE};

  nvvk::DescriptorSetBindings m_rtDescSetLayoutBind;
  VkDescriptorPool m_rtDescPool{VK_NULL_HANDLE};
  VkDescriptorSetLayout m_rtDescSetLayout{VK_NULL_HANDLE};
  VkDescriptorSet m_rtDescSet{VK_NULL_HANDLE};

  nvvk::DescriptorSetBindings m_lightDescSetLayoutBind;
  VkDescriptorPool m_lightDescPool{VK_NULL_HANDLE};
  VkDescriptorSetLayout m_lightDescSetLayout{VK_NULL_HANDLE};
  VkDescriptorSet m_lightDescSet{VK_NULL_HANDLE};

  nvvk::DescriptorSetBindings m_restirDescSetLayoutBind;
  VkDescriptorPool m_restirDescPool{VK_NULL_HANDLE};
  VkDescriptorSetLayout m_restirDescSetLayout{VK_NULL_HANDLE};
  std::vector<VkDescriptorSet> m_restirDescSets;

  nvvk::DescriptorSetBindings m_restirPostDescSetLayoutBind;
  VkDescriptorPool m_restirPostDescPool{VK_NULL_HANDLE};
  VkDescriptorSetLayout m_restirPostDescSetLayout{VK_NULL_HANDLE};
  VkDescriptorSet m_restirPostDescSet;

  nvvk::DescriptorSetBindings m_restirUniformDescSetLayoutBind;
  VkDescriptorPool m_restirUniformDescPool{VK_NULL_HANDLE};
  VkDescriptorSetLayout m_restirUniformDescSetLayout{VK_NULL_HANDLE};
  VkDescriptorSet m_restirUniformDescSet{VK_NULL_HANDLE};

  // shader binding tables
  nvvk::Buffer m_rtSBTBuffer;
  VkStridedDeviceAddressRegionKHR m_rgenRegion{};
  VkStridedDeviceAddressRegionKHR m_missRegion{};
  VkStridedDeviceAddressRegionKHR m_hitRegion{};
  VkStridedDeviceAddressRegionKHR m_callRegion{};

  // Push constant for ray tracer
  PushConstantRay m_pcRay{};

  std::vector<Sphere> m_spheres;         // All spheres
  nvvk::Buffer m_spheresBuffer;          // Buffer holding the spheres
  nvvk::Buffer m_spheresAabbBuffer;      // Buffer of all Aabb
  nvvk::Buffer m_spheresMatColorBuffer;  // Multiple materials
  nvvk::Buffer
      m_spheresMatIndexBuffer;  // Define which sphere uses which material

  nvvk::RaytracingBuilderKHR::BlasInput gltfToGeometryKHR(
      const VkDevice& device, const nvh::GltfPrimMesh& prim);
};
