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

#include <sstream>

#define STB_IMAGE_IMPLEMENTATION
#include <random>

#include "Renderer.h"
#include "SingletonManager.hpp"
#include "config/static_config.hpp"
#include "nvh/alignment.hpp"
#include "nvh/cameramanipulator.hpp"
#include "nvh/fileoperations.hpp"
#include "nvvk/buffers_vk.hpp"
#include "nvvk/commands_vk.hpp"
#include "nvvk/descriptorsets_vk.hpp"
#include "nvvk/images_vk.hpp"
#include "nvvk/pipeline_vk.hpp"
#include "nvvk/renderpasses_vk.hpp"
#include "nvvk/shaders_vk.hpp"
#include "obj_loader.h"
#include "stb_image.h"
#include "utils/logging.hpp"
#include "utils/shader_functions.hpp"

extern std::vector<std::string> defaultSearchPaths;

namespace {
VkSamplerCreateInfo gltfSamplerToVulkan(tinygltf::Sampler& tSampler) {
  VkSamplerCreateInfo samplerCreateInfo{VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO};

  std::map<int, VkFilter> filters;
  filters[9728] = VK_FILTER_NEAREST;  // NEAREST
  filters[9729] = VK_FILTER_LINEAR;   // LINEAR
  filters[9984] = VK_FILTER_NEAREST;  // NEAREST_MIPMAP_NEAREST
  filters[9985] = VK_FILTER_LINEAR;   // LINEAR_MIPMAP_NEAREST
  filters[9986] = VK_FILTER_NEAREST;  // NEAREST_MIPMAP_LINEAR
  filters[9987] = VK_FILTER_LINEAR;   // LINEAR_MIPMAP_LINEAR

  std::map<int, VkSamplerMipmapMode> mipmap;
  mipmap[9728] = VK_SAMPLER_MIPMAP_MODE_NEAREST;  // NEAREST
  mipmap[9729] = VK_SAMPLER_MIPMAP_MODE_NEAREST;  // LINEAR
  mipmap[9984] = VK_SAMPLER_MIPMAP_MODE_NEAREST;  // NEAREST_MIPMAP_NEAREST
  mipmap[9985] = VK_SAMPLER_MIPMAP_MODE_NEAREST;  // LINEAR_MIPMAP_NEAREST
  mipmap[9986] = VK_SAMPLER_MIPMAP_MODE_LINEAR;   // NEAREST_MIPMAP_LINEAR
  mipmap[9987] = VK_SAMPLER_MIPMAP_MODE_LINEAR;   // LINEAR_MIPMAP_LINEAR

  std::map<int, VkSamplerAddressMode> addressMode;
  addressMode[33071] = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
  addressMode[33648] = VK_SAMPLER_ADDRESS_MODE_MIRRORED_REPEAT;
  addressMode[10497] = VK_SAMPLER_ADDRESS_MODE_REPEAT;

  samplerCreateInfo.magFilter  = filters[tSampler.magFilter];
  samplerCreateInfo.minFilter  = filters[tSampler.minFilter];
  samplerCreateInfo.mipmapMode = mipmap[tSampler.minFilter];

  samplerCreateInfo.addressModeU = addressMode[tSampler.wrapS];
  samplerCreateInfo.addressModeV = addressMode[tSampler.wrapT];
  // samplerCreateInfo.addressModeW = addressMode[tSampler.wrapR];

  // Always allow LOD
  samplerCreateInfo.maxLod = FLT_MAX;
  return samplerCreateInfo;
}
}  // namespace

//--------------------------------------------------------------------------------------------------
// Keep the handle on the device
// Initialize the tool to do all our allocations: buffers, images
//
void Renderer::setup(const VkInstance& instance, const VkDevice& device,
                     const VkPhysicalDevice& physicalDevice,
                     uint32_t queueFamily) {
  AppBaseVk::setup(instance, device, physicalDevice, queueFamily);
  m_alloc.init(instance, device, physicalDevice);
  m_debug.setup(m_device);
  m_offscreenDepthFormat = nvvk::findDepthFormat(physicalDevice);
}

//--------------------------------------------------------------------------------------------------
// Create GBuffers for ReSTIR
//
void Renderer::createGBuffers() {
  for (size_t i = 0; i < static_config::kNumGBuffers; ++i) {
    m_gBuffers[i].create(&m_alloc, m_device, m_graphicsQueueIndex, m_size,
                         m_renderPass);
  }
  spdlog::info("Created GBuffers");
}

void Renderer::updateGBufferFrameIdx() {
  m_currentGBufferFrameIdx =
      (m_currentGBufferFrameIdx + 1) % static_config::kNumGBuffers;
}

//--------------------------------------------------------------------------------------------------
// Called at each frame to update the camera matrix
//
void Renderer::updateUniformBuffer(const VkCommandBuffer& cmdBuf) {
  // Prepare new UBO contents on host.
  const float aspectRatio = m_size.width / static_cast<float>(m_size.height);
  GlobalUniforms hostUBO  = {};
  const auto& view        = CameraManip.getMatrix();
  const auto& proj =
      nvmath::perspectiveVK(CameraManip.getFov(), aspectRatio, 0.1f, 1000.0f);
  // proj[1][1] *= -1;  // Inverting Y for Vulkan (not needed with
  // perspectiveVK).

  hostUBO.viewProj    = proj * view;
  hostUBO.viewInverse = nvmath::invert(view);
  hostUBO.projInverse = nvmath::invert(proj);

  // UBO on the device, and what stages access it.
  VkBuffer deviceUBO  = m_bGlobals.buffer;
  auto uboUsageStages = VK_PIPELINE_STAGE_VERTEX_SHADER_BIT |
                        VK_PIPELINE_STAGE_RAY_TRACING_SHADER_BIT_KHR;

  // Ensure that the modified UBO is not visible to previous frames.
  VkBufferMemoryBarrier beforeBarrier{VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER};
  beforeBarrier.srcAccessMask = VK_ACCESS_SHADER_READ_BIT;
  beforeBarrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
  beforeBarrier.buffer        = deviceUBO;
  beforeBarrier.offset        = 0;
  beforeBarrier.size          = sizeof(hostUBO);
  vkCmdPipelineBarrier(cmdBuf, uboUsageStages, VK_PIPELINE_STAGE_TRANSFER_BIT,
                       VK_DEPENDENCY_DEVICE_GROUP_BIT, 0, nullptr, 1,
                       &beforeBarrier, 0, nullptr);

  // Schedule the host-to-device upload. (hostUBO is copied into the cmd
  // buffer so it is okay to deallocate when the function returns).
  vkCmdUpdateBuffer(cmdBuf, m_bGlobals.buffer, 0, sizeof(GlobalUniforms),
                    &hostUBO);

  // Making sure the updated UBO will be visible.
  VkBufferMemoryBarrier afterBarrier{VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER};
  afterBarrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
  afterBarrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
  afterBarrier.buffer        = deviceUBO;
  afterBarrier.offset        = 0;
  afterBarrier.size          = sizeof(hostUBO);
  vkCmdPipelineBarrier(cmdBuf, VK_PIPELINE_STAGE_TRANSFER_BIT, uboUsageStages,
                       VK_DEPENDENCY_DEVICE_GROUP_BIT, 0, nullptr, 1,
                       &afterBarrier, 0, nullptr);
}

//--------------------------------------------------------------------------------------------------
// Describing the layout pushed when rendering
//
void Renderer::createDescriptorSetLayout() {
  auto nbTxt = static_cast<uint32_t>(m_textures.size());

  // Camera matrices
  m_descSetLayoutBind.addBinding(
      SceneBindings::eGlobals, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1,
      VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_RAYGEN_BIT_KHR);
#ifndef USE_GLTF
  // Obj descriptions
  m_descSetLayoutBind.addBinding(
      SceneBindings::eObjDescs, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1,
      VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT |
          VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR);
  // Textures
  m_descSetLayoutBind.addBinding(
      SceneBindings::eTextures, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
      nbTxt,
      VK_SHADER_STAGE_FRAGMENT_BIT | VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR);
#endif
  // Implicit geometries
  m_descSetLayoutBind.addBinding(SceneBindings::eImplicit,
                                 VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1,
                                 VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR |
                                     VK_SHADER_STAGE_INTERSECTION_BIT_KHR);

  m_descSetLayoutBind.addBinding(SceneBindings::eSphereMaterial,
                                 VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1,
                                 VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR |
                                     VK_SHADER_STAGE_INTERSECTION_BIT_KHR);

#ifdef USE_GLTF
  m_descSetLayoutBind.addBinding(SceneBindings::eGLTFVertices,
                                 VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1,
                                 VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR);
  m_descSetLayoutBind.addBinding(SceneBindings::eGLTFNormals,
                                 VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1,
                                 VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR);
  m_descSetLayoutBind.addBinding(SceneBindings::eGLTFTexcoords,
                                 VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1,
                                 VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR);
  m_descSetLayoutBind.addBinding(SceneBindings::eGLTFIndices,
                                 VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1,
                                 VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR);
  m_descSetLayoutBind.addBinding(SceneBindings::eGLTFMaterials,
                                 VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1,
                                 VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR);
  m_descSetLayoutBind.addBinding(SceneBindings::eGLTFMatrices,
                                 VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1,
                                 VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR);
  m_descSetLayoutBind.addBinding(SceneBindings::eGLTFTangents,
                                 VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1,
                                 VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR);
  m_descSetLayoutBind.addBinding(SceneBindings::eGLTFTextures,
                                 VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
                                 static_cast<uint32_t>(m_gltfTextures.size()),
                                 VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR);
  m_descSetLayoutBind.addBinding(SceneBindings::eGLTFColors,
                                 VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1,
                                 VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR);
  m_descSetLayoutBind.addBinding(SceneBindings::eGLTFPrimLookup,
                                 VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1,
                                 VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR);
#endif

  m_descSetLayout = m_descSetLayoutBind.createLayout(m_device);
  m_descPool      = m_descSetLayoutBind.createPool(m_device, 1);
  m_descSet =
      nvvk::allocateDescriptorSet(m_device, m_descPool, m_descSetLayout);
  spdlog::info("Created global descriptor set layout");
}

//--------------------------------------------------------------------------------------------------
// Setting up the buffers in the descriptor set
//
void Renderer::updateDescriptorSet() {
  std::vector<VkWriteDescriptorSet> writes;

  // Camera matrices and scene description
  VkDescriptorBufferInfo dbiUnif{m_bGlobals.buffer, 0, VK_WHOLE_SIZE};
  writes.emplace_back(m_descSetLayoutBind.makeWrite(
      m_descSet, SceneBindings::eGlobals, &dbiUnif));

#ifndef USE_GLTF
  VkDescriptorBufferInfo dbiSceneDesc{m_bObjDesc.buffer, 0, VK_WHOLE_SIZE};
  writes.emplace_back(m_descSetLayoutBind.makeWrite(
      m_descSet, SceneBindings::eObjDescs, &dbiSceneDesc));

  // All OBJ texture samplers
  std::vector<VkDescriptorImageInfo> diit;
  for (auto& texture : m_textures) {
    diit.emplace_back(texture.descriptor);
  }
  writes.emplace_back(m_descSetLayoutBind.makeWriteArray(
      m_descSet, SceneBindings::eTextures, diit.data()));
#endif

  // spheres
  VkDescriptorBufferInfo dbiSpheres{m_spheresBuffer.buffer, 0, VK_WHOLE_SIZE};
  writes.emplace_back(
      m_descSetLayoutBind.makeWrite(m_descSet, eImplicit, &dbiSpheres));

  VkDescriptorBufferInfo spherematInfo{m_sphereMaterialsBuffer.buffer, 0,
                                       VK_WHOLE_SIZE};

  writes.emplace_back(m_descSetLayoutBind.makeWrite(
      m_descSet, SceneBindings::eSphereMaterial, &spherematInfo));

#ifdef USE_GLTF
  // GLTF stuff
  {
    VkDescriptorBufferInfo primInfo{m_gltfPrimLookup.buffer, 0, VK_WHOLE_SIZE};
    VkDescriptorBufferInfo verInfo{m_gltfVertices.buffer, 0, VK_WHOLE_SIZE};
    VkDescriptorBufferInfo norInfo{m_gltfNormals.buffer, 0, VK_WHOLE_SIZE};
    VkDescriptorBufferInfo texInfo{m_gltfTexcoords.buffer, 0, VK_WHOLE_SIZE};
    VkDescriptorBufferInfo idxInfo{m_gltfIndices.buffer, 0, VK_WHOLE_SIZE};
    VkDescriptorBufferInfo mateInfo{m_gltfMaterials.buffer, 0, VK_WHOLE_SIZE};
    VkDescriptorBufferInfo mtxInfo{m_gltfMatrices.buffer, 0, VK_WHOLE_SIZE};
    VkDescriptorBufferInfo tanInfo{m_gltfTangents.buffer, 0, VK_WHOLE_SIZE};
    VkDescriptorBufferInfo colInfo{m_gltfColors.buffer, 0, VK_WHOLE_SIZE};

    writes.emplace_back(m_descSetLayoutBind.makeWrite(
        m_descSet, SceneBindings::eGLTFPrimLookup, &primInfo));
    writes.emplace_back(m_descSetLayoutBind.makeWrite(
        m_descSet, SceneBindings::eGLTFVertices, &verInfo));
    writes.emplace_back(m_descSetLayoutBind.makeWrite(
        m_descSet, SceneBindings::eGLTFNormals, &norInfo));
    writes.emplace_back(m_descSetLayoutBind.makeWrite(
        m_descSet, SceneBindings::eGLTFTexcoords, &texInfo));
    writes.emplace_back(m_descSetLayoutBind.makeWrite(
        m_descSet, SceneBindings::eGLTFIndices, &idxInfo));
    writes.emplace_back(m_descSetLayoutBind.makeWrite(
        m_descSet, SceneBindings::eGLTFMaterials, &mateInfo));
    writes.emplace_back(m_descSetLayoutBind.makeWrite(
        m_descSet, SceneBindings::eGLTFMatrices, &mtxInfo));
    writes.emplace_back(m_descSetLayoutBind.makeWrite(
        m_descSet, SceneBindings::eGLTFTangents, &tanInfo));
    writes.emplace_back(m_descSetLayoutBind.makeWrite(
        m_descSet, SceneBindings::eGLTFColors, &colInfo));
  }

  // All GLTF texture samplers
  std::vector<VkDescriptorImageInfo> diit;
  for (auto& texture : m_gltfTextures) {
    diit.emplace_back(texture.descriptor);
  }
  writes.emplace_back(m_descSetLayoutBind.makeWriteArray(
      m_descSet, SceneBindings::eGLTFTextures, diit.data()));
#endif

  // Writing the information
  vkUpdateDescriptorSets(m_device, static_cast<uint32_t>(writes.size()),
                         writes.data(), 0, nullptr);
  spdlog::info("Updated global descriptor set");
}

//--------------------------------------------------------------------------------------------------
// Creating the pipeline layout
//
void Renderer::createGraphicsPipeline() {
  VkPushConstantRange pushConstantRanges = {
      VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, 0,
      sizeof(PushConstantRaster)};

  // Creating the Pipeline Layout
  VkPipelineLayoutCreateInfo createInfo{
      VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO};
  createInfo.setLayoutCount         = 1;
  createInfo.pSetLayouts            = &m_descSetLayout;
  createInfo.pushConstantRangeCount = 1;
  createInfo.pPushConstantRanges    = &pushConstantRanges;
  vkCreatePipelineLayout(m_device, &createInfo, nullptr, &m_pipelineLayout);

  // Creating the Pipeline
  std::vector<std::string> paths = defaultSearchPaths;
  nvvk::GraphicsPipelineGeneratorCombined gpb(m_device, m_pipelineLayout,
                                              m_offscreenRenderPass);
  gpb.depthStencilState.depthTestEnable = true;
  gpb.addShader(nvh::loadFile("spv/vert_shader.vert.spv", true, paths, true),
                VK_SHADER_STAGE_VERTEX_BIT);
  gpb.addShader(nvh::loadFile("spv/frag_shader.frag.spv", true, paths, true),
                VK_SHADER_STAGE_FRAGMENT_BIT);
  gpb.addBindingDescription({0, sizeof(VertexObj)});
  gpb.addAttributeDescriptions({
      {0, 0, VK_FORMAT_R32G32B32_SFLOAT,
       static_cast<uint32_t>(offsetof(VertexObj, pos))},
      {1, 0, VK_FORMAT_R32G32B32_SFLOAT,
       static_cast<uint32_t>(offsetof(VertexObj, nrm))},
      {2, 0, VK_FORMAT_R32G32B32_SFLOAT,
       static_cast<uint32_t>(offsetof(VertexObj, color))},
      {3, 0, VK_FORMAT_R32G32_SFLOAT,
       static_cast<uint32_t>(offsetof(VertexObj, texCoord))},
  });

  m_graphicsPipeline = gpb.createPipeline();
  m_debug.setObjectName(m_graphicsPipeline, "Graphics");
  spdlog::info("Created Graphics Pipeline");
}

void Renderer::loadGLTFModel(const std::string& filename) {
  spdlog::info("Loading GLTF Model...");
  SingletonManager::GetGLTFLoader().loadScene(filename);
  nvh::GltfScene gltfScene = SingletonManager::GetGLTFLoader().getGLTFScene();

  // ---------- Collect point lights ----------
  m_pointLights.reserve(gltfScene.m_lights.size());
  for (const nvh::GltfLight& light : gltfScene.m_lights) {
    PointLight& addedLight = m_pointLights.emplace_back(
        PointLight{light.worldMatrix.col(3),
                   nvmath::vec4f(light.light.color[0], light.light.color[1],
                                 light.light.color[2], 0.0f)});
    addedLight.emission_luminance.w = shader::luminance(
        addedLight.emission_luminance.x, addedLight.emission_luminance.y,
        addedLight.emission_luminance.z);
  }
  spdlog::info("Collected GLTF point lights of size: {}", m_pointLights.size());

  // ---------- Collect Triangle Lights ----------
  for (const nvh::GltfNode& node : gltfScene.m_nodes) {
    const nvh::GltfPrimMesh& mesh = gltfScene.m_primMeshes[node.primMesh];
    const nvh::GltfMaterial& material =
        gltfScene.m_materials[mesh.materialIndex];
    if (material.emissiveFactor.sq_norm() > 1e-6) {
      const uint32_t* indices = gltfScene.m_indices.data() + mesh.firstIndex;
      const nvmath::vec3f* pos =
          gltfScene.m_positions.data() + mesh.vertexOffset;
      for (uint32_t i = 0; i < mesh.indexCount; i += 3, indices += 3) {
        // triangle
        nvmath::vec4f p1 =
            node.worldMatrix * nvmath::vec4f(pos[indices[0]], 1.0f);
        nvmath::vec4f p2 =
            node.worldMatrix * nvmath::vec4f(pos[indices[1]], 1.0f);
        nvmath::vec4f p3 =
            node.worldMatrix * nvmath::vec4f(pos[indices[2]], 1.0f);
        nvmath::vec3f p1_vec3(p1.x, p1.y, p1.z), p2_vec3(p2.x, p2.y, p2.z),
            p3_vec3(p3.x, p3.y, p3.z);

        nvmath::vec3f normal =
            nvmath::cross(p2_vec3 - p1_vec3, p3_vec3 - p1_vec3);
        float area = normal.norm();
        normal /= area;
        area *= 0.5f;

        float emissionLuminance = shader::luminance(material.emissiveFactor.x,
                                                    material.emissiveFactor.y,
                                                    material.emissiveFactor.z);
        m_triangleLights.push_back(TriangleLight{
            p1, p2, p3,
            nvmath::vec4f(material.emissiveFactor, emissionLuminance),
            nvmath::vec4f(normal, area)});
      }
    }
  }
  spdlog::info("Collected GLTF triangle lights of size: {}",
               m_triangleLights.size());
  spdlog::info("GLTF Model has been loaded");
}

void Renderer::createGLTFBuffer() {
  if (!SingletonManager::GetGLTFLoader().isSceneLoaded()) {
    spdlog::error("GLTF model is not loaded");
    throw std::runtime_error("creating empty GLTF buffer");
  }

  nvvk::CommandPool cmdBufGet(m_device, m_graphicsQueueIndex);
  VkCommandBuffer cmdBuf = cmdBufGet.createCommandBuffer();

  nvh::GltfScene gltfScene = SingletonManager::GetGLTFLoader().getGLTFScene();

  // GLTF Scenes
  m_gltfVertices = m_alloc.createBuffer(
      cmdBuf, gltfScene.m_positions,
      VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT |
          VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_BUILD_INPUT_READ_ONLY_BIT_KHR |
          VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT);
  m_gltfIndices = m_alloc.createBuffer(
      cmdBuf, gltfScene.m_indices,
      VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT |
          VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_BUILD_INPUT_READ_ONLY_BIT_KHR |
          VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT);
  m_gltfNormals   = m_alloc.createBuffer(cmdBuf, gltfScene.m_normals,
                                       VK_BUFFER_USAGE_STORAGE_BUFFER_BIT);
  m_gltfTexcoords = m_alloc.createBuffer(cmdBuf, gltfScene.m_texcoords0,
                                         VK_BUFFER_USAGE_STORAGE_BUFFER_BIT);
  m_gltfTangents  = m_alloc.createBuffer(cmdBuf, gltfScene.m_tangents,
                                        VK_BUFFER_USAGE_STORAGE_BUFFER_BIT);
  m_gltfColors    = m_alloc.createBuffer(cmdBuf, gltfScene.m_colors0,
                                      VK_BUFFER_USAGE_STORAGE_BUFFER_BIT);

  // GLTF Primitive Lookups
  std::vector<RestirPrimitiveLookup> primLookup;
  for (auto& primMesh : gltfScene.m_primMeshes)
    primLookup.push_back(
        {primMesh.firstIndex, primMesh.vertexOffset, primMesh.materialIndex});
  m_gltfPrimLookup = m_alloc.createBuffer(cmdBuf, primLookup,
                                          VK_BUFFER_USAGE_STORAGE_BUFFER_BIT);

  // GLTF Materials
  std::vector<GltfMaterials> shadeMaterials;
  for (auto& m : gltfScene.m_materials) {
    GltfMaterials smat;
    smat.pbrBaseColorFactor          = m.baseColorFactor;
    smat.pbrBaseColorTexture         = m.baseColorTexture;
    smat.pbrMetallicFactor           = m.metallicFactor;
    smat.pbrRoughnessFactor          = m.roughnessFactor;
    smat.pbrMetallicRoughnessTexture = m.metallicRoughnessTexture;
    smat.khrDiffuseFactor            = m.specularGlossiness.diffuseFactor;
    smat.khrSpecularFactor           = m.specularGlossiness.specularFactor;
    smat.khrDiffuseTexture           = m.specularGlossiness.diffuseTexture;
    smat.khrGlossinessFactor         = m.specularGlossiness.glossinessFactor;
    smat.khrSpecularGlossinessTexture =
        m.specularGlossiness.specularGlossinessTexture;
    smat.uvTransform        = m.textureTransform.uvTransform;
    smat.shadingModel       = m.shadingModel;
    smat.emissiveTexture    = m.emissiveTexture;
    smat.emissiveFactor     = m.emissiveFactor;
    smat.alphaMode          = m.alphaMode;
    smat.alphaCutoff        = m.alphaCutoff;
    smat.doubleSided        = m.doubleSided;
    smat.normalTexture      = m.normalTexture;
    smat.normalTextureScale = m.normalTextureScale;
    shadeMaterials.emplace_back(smat);
  }
  m_gltfMaterials = m_alloc.createBuffer(cmdBuf, shadeMaterials,
                                         VK_BUFFER_USAGE_STORAGE_BUFFER_BIT);

  // GLTF Model Matrices
  std::vector<GLTFModelMatrices> nodeMatrices;
  for (auto& node : gltfScene.m_nodes) {
    GLTFModelMatrices mat;
    mat.transform                  = node.worldMatrix;
    mat.transformInverseTransposed = invert(node.worldMatrix);
    nodeMatrices.emplace_back(mat);
  }
  m_gltfMatrices = m_alloc.createBuffer(cmdBuf, nodeMatrices,
                                        VK_BUFFER_USAGE_STORAGE_BUFFER_BIT);

  // ------------ Create texture buffer ------------
  VkFormat format = VK_FORMAT_R8G8B8A8_UNORM;
  VkSamplerCreateInfo samplerCreateInfo{VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO};
  samplerCreateInfo.minFilter  = VK_FILTER_LINEAR;
  samplerCreateInfo.magFilter  = VK_FILTER_LINEAR;
  samplerCreateInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
  samplerCreateInfo.maxLod     = FLT_MAX;

  auto addDefaultTexture = [this, cmdBuf](VkFormat format) {
    std::array<uint8_t, 4> white = {255, 255, 255, 255};
    VkSamplerCreateInfo samplerCreateInfo{
        VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO};
    m_gltfTextures.emplace_back(this->m_alloc.createTexture(
        cmdBuf, 4, white.data(),
        nvvk::makeImage2DCreateInfo(VkExtent2D{1, 1}, format),
        samplerCreateInfo));
    m_debug.setObjectName(m_gltfTextures.back().image, "restirGLTFdummy");
  };
  tinygltf::Model tmodel = SingletonManager::GetGLTFLoader().getTModel();
  if (tmodel.images.empty()) {
    // No images, add a default one.
    addDefaultTexture(format);
  } else {
    m_gltfTextures.resize(tmodel.textures.size());
    // load textures
    for (int i = 0; i < tmodel.textures.size(); ++i) {
      int sourceImage = tmodel.textures[i].source;
      if (sourceImage >= tmodel.images.size() || sourceImage < 0) {
        // Incorrect source image
        addDefaultTexture(format);
        continue;
      }
      auto& gltfImage = tmodel.images[sourceImage];
      if (gltfImage.width == -1 || gltfImage.height == -1 ||
          gltfImage.image.empty()) {
        // Image not present or incorrectly loaded (image.empty)
        addDefaultTexture(format);
        continue;
      }
      void* buffer            = &gltfImage.image[0];
      VkDeviceSize bufferSize = gltfImage.image.size();
      auto imgSize = VkExtent2D{static_cast<uint32_t>(gltfImage.width),
                                static_cast<uint32_t>(gltfImage.height)};

      // std::cout << "Loading Texture: " << gltfImage.uri << std::endl;
      if (tmodel.textures[i].sampler > -1) {
        // Retrieve the texture sampler
        auto gltfSampler  = tmodel.samplers[tmodel.textures[i].sampler];
        samplerCreateInfo = gltfSamplerToVulkan(gltfSampler);
      }
      VkImageCreateInfo imageCreateInfo = nvvk::makeImage2DCreateInfo(
          imgSize, format, VK_IMAGE_USAGE_SAMPLED_BIT, true);

      nvvk::Image image =
          m_alloc.createImage(cmdBuf, bufferSize, buffer, imageCreateInfo);
      nvvk::cmdGenerateMipmaps(cmdBuf, image.image, format, imgSize,
                               imageCreateInfo.mipLevels);
      VkImageViewCreateInfo ivInfo =
          nvvk::makeImageViewCreateInfo(image.image, imageCreateInfo);
      m_gltfTextures[i] =
          m_alloc.createTexture(image, ivInfo, samplerCreateInfo);
      m_debug.setObjectName(
          m_gltfTextures[i].image,
          std::string("restirGLTFTxt" + std::to_string(i)).c_str());
    }
  }
  cmdBufGet.submitAndWait(cmdBuf);
  m_alloc.finalizeAndReleaseStaging();
}

//--------------------------------------------------------------------------------------------------
// Loading the OBJ file and setting up all buffers
//
void Renderer::loadModel(const std::string& filename, nvmath::mat4f transform) {
  LOGI("Loading File:  %s \n", filename.c_str());
  ObjLoader loader;
  loader.loadModel(filename);

  // Converting from Srgb to linear
  for (auto& m : loader.m_materials) {
    m.ambient  = nvmath::pow(m.ambient, 2.2f);
    m.diffuse  = nvmath::pow(m.diffuse, 2.2f);
    m.specular = nvmath::pow(m.specular, 2.2f);
  }

  ObjModel model;
  model.nbIndices  = static_cast<uint32_t>(loader.m_indices.size());
  model.nbVertices = static_cast<uint32_t>(loader.m_vertices.size());

  float scaleFactor         = 10;
  nvmath::mat3f scaleMatrix = nvmath::mat3f(1.0);
  scaleMatrix *= scaleFactor;

  for (auto& vertex : loader.m_vertices) {
    vertex.pos = scaleMatrix * vertex.pos;
  }

  // Create the buffers on Device and copy vertices, indices and materials
  nvvk::CommandPool cmdBufGet(m_device, m_graphicsQueueIndex);
  VkCommandBuffer cmdBuf  = cmdBufGet.createCommandBuffer();
  VkBufferUsageFlags flag = VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT;
  VkBufferUsageFlags
      rayTracingFlags =  // used also for building acceleration structures
      flag |
      VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_BUILD_INPUT_READ_ONLY_BIT_KHR |
      VK_BUFFER_USAGE_STORAGE_BUFFER_BIT;
  model.vertexBuffer =
      m_alloc.createBuffer(cmdBuf, loader.m_vertices,
                           VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | rayTracingFlags);
  model.indexBuffer =
      m_alloc.createBuffer(cmdBuf, loader.m_indices,
                           VK_BUFFER_USAGE_INDEX_BUFFER_BIT | rayTracingFlags);
  model.matColorBuffer = m_alloc.createBuffer(
      cmdBuf, loader.m_materials, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | flag);
  model.matIndexBuffer = m_alloc.createBuffer(
      cmdBuf, loader.m_matIndx, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | flag);
  // Creates all textures found and find the offset for this model
  auto txtOffset = static_cast<uint32_t>(m_textures.size());
  createTextureImages(cmdBuf, loader.m_textures);
  cmdBufGet.submitAndWait(cmdBuf);
  m_alloc.finalizeAndReleaseStaging();

  std::string objNb = std::to_string(m_objModel.size());
  m_debug.setObjectName(model.vertexBuffer.buffer,
                        (std::string("vertex_" + objNb)));
  m_debug.setObjectName(model.indexBuffer.buffer,
                        (std::string("index_" + objNb)));
  m_debug.setObjectName(model.matColorBuffer.buffer,
                        (std::string("mat_" + objNb)));
  m_debug.setObjectName(model.matIndexBuffer.buffer,
                        (std::string("matIdx_" + objNb)));

  // Keeping transformation matrix of the instance
  ObjInstance instance;
  instance.transform = transform;
  instance.objIndex  = static_cast<uint32_t>(m_objModel.size());
  m_instances.push_back(instance);

  // Creating information for device access
  ObjDesc desc;
  desc.txtOffset = txtOffset;
  desc.vertexAddress =
      nvvk::getBufferDeviceAddress(m_device, model.vertexBuffer.buffer);
  desc.indexAddress =
      nvvk::getBufferDeviceAddress(m_device, model.indexBuffer.buffer);
  desc.materialAddress =
      nvvk::getBufferDeviceAddress(m_device, model.matColorBuffer.buffer);
  desc.materialIndexAddress =
      nvvk::getBufferDeviceAddress(m_device, model.matIndexBuffer.buffer);

  // Keeping the obj host model and device description
  m_objModel.emplace_back(model);
  m_objDesc.emplace_back(desc);
}

//--------------------------------------------------------------------------------------------------
// Creating the uniform buffer holding the camera matrices
// - Buffer is host visible
//
void Renderer::createUniformBuffer() {
  m_bGlobals = m_alloc.createBuffer(
      sizeof(GlobalUniforms),
      VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
      VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
  m_debug.setObjectName(m_bGlobals.buffer, "Globals");
  spdlog::info("Allocated Global Uniform buffer");
}

//--------------------------------------------------------------------------------------------------
// Creating the uniform buffer for ReSTIR reservoirs
//
void Renderer::createRestirBuffer() {
  nvvk::CommandPool cmdBufGet(m_device, m_graphicsQueueIndex);
  VkCommandBuffer cmdBuf = cmdBufGet.createCommandBuffer();

  // resize reservoir buffers
  m_reservoirInfoBuffers.resize(static_config::kNumGBuffers);
  m_reservoirWeightBuffers.resize(static_config::kNumGBuffers);

  // create image sampler
  auto colorCreateInfo = nvvk::makeImage2DCreateInfo(
      m_size, VK_FORMAT_R32G32B32A32_SFLOAT,
      VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT |
          VK_IMAGE_USAGE_STORAGE_BIT);
  VkSamplerCreateInfo samplerCreateInfo{VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO};
  samplerCreateInfo.minFilter  = VK_FILTER_NEAREST;
  samplerCreateInfo.magFilter  = VK_FILTER_NEAREST;
  samplerCreateInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_NEAREST;

  // set reservoir buffers
  for (size_t i = 0; i < static_config::kNumGBuffers; ++i) {
    // m_reservoirInfoBuffers
    {
      nvvk::Image image = m_alloc.createImage(colorCreateInfo);
      VkImageViewCreateInfo ivInfo =
          nvvk::makeImageViewCreateInfo(image.image, colorCreateInfo);
      m_reservoirInfoBuffers[i] =
          m_alloc.createTexture(image, ivInfo, samplerCreateInfo);
      m_reservoirInfoBuffers[i].descriptor.imageLayout =
          VK_IMAGE_LAYOUT_GENERAL;
      nvvk::cmdBarrierImageLayout(cmdBuf, m_reservoirInfoBuffers[i].image,
                                  VK_IMAGE_LAYOUT_UNDEFINED,
                                  VK_IMAGE_LAYOUT_GENERAL);
    }
    // m_reservoirWeightBuffers
    {
      nvvk::Image image = m_alloc.createImage(colorCreateInfo);
      VkImageViewCreateInfo ivInfo =
          nvvk::makeImageViewCreateInfo(image.image, colorCreateInfo);
      m_reservoirWeightBuffers[i] =
          m_alloc.createTexture(image, ivInfo, samplerCreateInfo);
      m_reservoirWeightBuffers[i].descriptor.imageLayout =
          VK_IMAGE_LAYOUT_GENERAL;
      nvvk::cmdBarrierImageLayout(cmdBuf, m_reservoirWeightBuffers[i].image,
                                  VK_IMAGE_LAYOUT_UNDEFINED,
                                  VK_IMAGE_LAYOUT_GENERAL);
    }
  }

  // m_reservoirTmpInfoBuffer
  {
    nvvk::Image image = m_alloc.createImage(colorCreateInfo);
    VkImageViewCreateInfo ivInfo =
        nvvk::makeImageViewCreateInfo(image.image, colorCreateInfo);
    m_reservoirTmpInfoBuffer =
        m_alloc.createTexture(image, ivInfo, samplerCreateInfo);
    m_reservoirTmpInfoBuffer.descriptor.imageLayout = VK_IMAGE_LAYOUT_GENERAL;
    nvvk::cmdBarrierImageLayout(cmdBuf, m_reservoirTmpInfoBuffer.image,
                                VK_IMAGE_LAYOUT_UNDEFINED,
                                VK_IMAGE_LAYOUT_GENERAL);
  }

  // m_reservoirTmpWeightBuffer
  {
    nvvk::Image image = m_alloc.createImage(colorCreateInfo);
    VkImageViewCreateInfo ivInfo =
        nvvk::makeImageViewCreateInfo(image.image, colorCreateInfo);
    m_reservoirTmpWeightBuffer =
        m_alloc.createTexture(image, ivInfo, samplerCreateInfo);
    m_reservoirTmpWeightBuffer.descriptor.imageLayout = VK_IMAGE_LAYOUT_GENERAL;
    nvvk::cmdBarrierImageLayout(cmdBuf, m_reservoirTmpWeightBuffer.image,
                                VK_IMAGE_LAYOUT_UNDEFINED,
                                VK_IMAGE_LAYOUT_GENERAL);
  }

  // m_storageImage
  {
    nvvk::Image image = m_alloc.createImage(colorCreateInfo);
    VkImageViewCreateInfo ivInfo =
        nvvk::makeImageViewCreateInfo(image.image, colorCreateInfo);
    m_storageImage = m_alloc.createTexture(image, ivInfo, samplerCreateInfo);
    m_storageImage.descriptor.imageLayout = VK_IMAGE_LAYOUT_GENERAL;
    nvvk::cmdBarrierImageLayout(cmdBuf, m_storageImage.image,
                                VK_IMAGE_LAYOUT_UNDEFINED,
                                VK_IMAGE_LAYOUT_GENERAL);
  }

  cmdBufGet.submitAndWait(cmdBuf);
  m_alloc.finalizeAndReleaseStaging();
}

//--------------------------------------------------------------------------------------------------
// Create a storage buffer containing the description of the scene elements
// - Which geometry is used by which instance
// - Transformation
// - Offset for texture
//
void Renderer::createObjDescriptionBuffer() {
  nvvk::CommandPool cmdGen(m_device, m_graphicsQueueIndex);

  auto cmdBuf = cmdGen.createCommandBuffer();
  m_bObjDesc  = m_alloc.createBuffer(cmdBuf, m_objDesc,
                                    VK_BUFFER_USAGE_STORAGE_BUFFER_BIT);
  cmdGen.submitAndWait(cmdBuf);
  m_alloc.finalizeAndReleaseStaging();
  m_debug.setObjectName(m_bObjDesc.buffer, "ObjDescs");
}

//--------------------------------------------------------------------------------------------------
// Creating all textures and samplers
//
void Renderer::createTextureImages(const VkCommandBuffer& cmdBuf,
                                   const std::vector<std::string>& textures) {
  VkSamplerCreateInfo samplerCreateInfo{VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO};
  samplerCreateInfo.minFilter  = VK_FILTER_LINEAR;
  samplerCreateInfo.magFilter  = VK_FILTER_LINEAR;
  samplerCreateInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
  samplerCreateInfo.maxLod     = FLT_MAX;

  VkFormat format = VK_FORMAT_R8G8B8A8_SRGB;

  // If no textures are present, create a dummy one to accommodate the pipeline
  // layout
  if (textures.empty() && m_textures.empty()) {
    nvvk::Texture texture;

    std::array<uint8_t, 4> color{255u, 255u, 255u, 255u};
    VkDeviceSize bufferSize = sizeof(color);
    auto imgSize            = VkExtent2D{1, 1};
    auto imageCreateInfo    = nvvk::makeImage2DCreateInfo(imgSize, format);

    // Creating the dummy texture
    nvvk::Image image =
        m_alloc.createImage(cmdBuf, bufferSize, color.data(), imageCreateInfo);
    VkImageViewCreateInfo ivInfo =
        nvvk::makeImageViewCreateInfo(image.image, imageCreateInfo);
    texture = m_alloc.createTexture(image, ivInfo, samplerCreateInfo);

    // The image format must be in VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
    nvvk::cmdBarrierImageLayout(cmdBuf, texture.image,
                                VK_IMAGE_LAYOUT_UNDEFINED,
                                VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
    m_textures.push_back(texture);
  } else {
    // Uploading all images
    for (const auto& texture : textures) {
      std::stringstream o;
      int texWidth, texHeight, texChannels;
      o << "media/textures/" << texture;
      std::string txtFile = nvh::findFile(o.str(), defaultSearchPaths, true);

      stbi_uc* stbi_pixels = stbi_load(txtFile.c_str(), &texWidth, &texHeight,
                                       &texChannels, STBI_rgb_alpha);

      std::array<stbi_uc, 4> color{255u, 0u, 255u, 255u};

      stbi_uc* pixels = stbi_pixels;
      // Handle failure
      if (!stbi_pixels) {
        texWidth = texHeight = 1;
        texChannels          = 4;
        pixels               = reinterpret_cast<stbi_uc*>(color.data());
      }

      VkDeviceSize bufferSize =
          static_cast<uint64_t>(texWidth) * texHeight * sizeof(uint8_t) * 4;
      auto imgSize = VkExtent2D{(uint32_t)texWidth, (uint32_t)texHeight};
      auto imageCreateInfo = nvvk::makeImage2DCreateInfo(
          imgSize, format, VK_IMAGE_USAGE_SAMPLED_BIT, true);

      {
        nvvk::Image image =
            m_alloc.createImage(cmdBuf, bufferSize, pixels, imageCreateInfo);
        nvvk::cmdGenerateMipmaps(cmdBuf, image.image, format, imgSize,
                                 imageCreateInfo.mipLevels);
        VkImageViewCreateInfo ivInfo =
            nvvk::makeImageViewCreateInfo(image.image, imageCreateInfo);
        nvvk::Texture texture =
            m_alloc.createTexture(image, ivInfo, samplerCreateInfo);

        m_textures.push_back(texture);
      }

      stbi_image_free(stbi_pixels);
    }
  }
}

//--------------------------------------------------------------------------------------------------
// Destroying all allocations
//
void Renderer::destroyResources() {
#ifdef USE_RT_PIPELINE
  vkDestroyPipeline(m_device, m_graphicsPipeline, nullptr);
  vkDestroyPipelineLayout(m_device, m_pipelineLayout, nullptr);
#endif
  vkDestroyDescriptorPool(m_device, m_descPool, nullptr);
  vkDestroyDescriptorSetLayout(m_device, m_descSetLayout, nullptr);

  m_alloc.destroy(m_bGlobals);

#ifdef USE_GLTF
  m_alloc.destroy(m_gltfVertices);
  m_alloc.destroy(m_gltfColors);
  m_alloc.destroy(m_gltfIndices);
  m_alloc.destroy(m_gltfMaterials);
  m_alloc.destroy(m_gltfMatrices);
  m_alloc.destroy(m_gltfNormals);
  m_alloc.destroy(m_gltfPrimLookup);
  m_alloc.destroy(m_gltfTangents);
  m_alloc.destroy(m_gltfTexcoords);
  for (auto& texture : m_gltfTextures) {
    m_alloc.destroy(texture);
  }
#else
  m_alloc.destroy(m_bObjDesc);

  for (auto& m : m_objModel) {
    m_alloc.destroy(m.vertexBuffer);
    m_alloc.destroy(m.indexBuffer);
    m_alloc.destroy(m.matColorBuffer);
    m_alloc.destroy(m.matIndexBuffer);
  }

  for (auto& t : m_textures) {
    m_alloc.destroy(t);
  }
#endif

  //#Post
  m_alloc.destroy(m_offscreenColor);
  m_alloc.destroy(m_offscreenDepth);
  vkDestroyPipeline(m_device, m_postPipeline, nullptr);
  vkDestroyPipelineLayout(m_device, m_postPipelineLayout, nullptr);
  vkDestroyDescriptorPool(m_device, m_postDescPool, nullptr);
  vkDestroyDescriptorSetLayout(m_device, m_postDescSetLayout, nullptr);
  vkDestroyRenderPass(m_device, m_offscreenRenderPass, nullptr);
  vkDestroyFramebuffer(m_device, m_offscreenFramebuffer, nullptr);

#ifdef USE_ANIMATION
  // Compute
  m_alloc.destroy(m_spheresVelocityBuffer);
  vkDestroyPipeline(m_device, m_compPipeline, nullptr);
  vkDestroyPipelineLayout(m_device, m_compPipelineLayout, nullptr);
  vkDestroyDescriptorPool(m_device, m_compDescPool, nullptr);
  vkDestroyDescriptorSetLayout(m_device, m_compDescSetLayout, nullptr);
#endif

  // #VKRay
  m_rtBuilder.destroy();
#ifdef USE_RT_PIPELINE
  vkDestroyPipeline(m_device, m_rtPipeline, nullptr);
  vkDestroyPipelineLayout(m_device, m_rtPipelineLayout, nullptr);
#endif
  vkDestroyDescriptorPool(m_device, m_rtDescPool, nullptr);
  vkDestroyDescriptorSetLayout(m_device, m_rtDescSetLayout, nullptr);
  m_alloc.destroy(m_rtSBTBuffer);

  // Spheres
  m_alloc.destroy(m_spheresBuffer);
  m_alloc.destroy(m_spheresAabbBuffer);
  m_alloc.destroy(m_spheresMatColorBuffer);
  m_alloc.destroy(m_spheresMatIndexBuffer);

#ifdef USE_RESTIR_PIPELINE
  // restir uniform buffer
  vkDestroyDescriptorPool(m_device, m_restirUniformDescPool, nullptr);
  vkDestroyDescriptorSetLayout(m_device, m_restirUniformDescSetLayout, nullptr);
  if (m_restirUniformBuffer.buffer != VK_NULL_HANDLE)
    m_alloc.destroy(m_restirUniformBuffer);

  // ReSTIR lights
  vkDestroyDescriptorPool(m_device, m_lightDescPool, nullptr);
  vkDestroyDescriptorSetLayout(m_device, m_lightDescSetLayout, nullptr);
  m_alloc.destroy(m_ptLightsBuffer);
  m_alloc.destroy(m_aliasTableBuffer);
  m_alloc.destroy(m_triangleLightsBuffer);

  // reservoirs
  vkDestroyDescriptorPool(m_device, m_restirDescPool, nullptr);
  vkDestroyDescriptorSetLayout(m_device, m_restirDescSetLayout, nullptr);
  for (auto& t : m_reservoirInfoBuffers) {
    m_alloc.destroy(t);
  }
  for (auto& t : m_reservoirWeightBuffers) {
    m_alloc.destroy(t);
  }
  m_alloc.destroy(m_reservoirTmpInfoBuffer);
  m_alloc.destroy(m_reservoirTmpWeightBuffer);

  // storage images
  m_alloc.destroy(m_storageImage);
  vkDestroyPipeline(m_device, m_restirPostPipeline, nullptr);
  vkDestroyPipelineLayout(m_device, m_restirPostPipelineLayout, nullptr);
  vkDestroyDescriptorPool(m_device, m_restirPostDescPool, nullptr);
  vkDestroyDescriptorSetLayout(m_device, m_restirPostDescSetLayout, nullptr);

  // ReSTIR passes
  m_restirPass.destroy();
  m_spatialReusePass.destroy();
#endif

  // GBuffers
  for (auto& gBuf : m_gBuffers) {
    gBuf.destroy();
  }

  m_alloc.deinit();
}

//--------------------------------------------------------------------------------------------------
// Drawing the scene in raster mode
//
void Renderer::rasterize(const VkCommandBuffer& cmdBuf) {
  VkDeviceSize offset{0};

  m_debug.beginLabel(cmdBuf, "Rasterize");

  // Dynamic Viewport
  setViewport(cmdBuf);

  // Drawing all triangles
  vkCmdBindPipeline(cmdBuf, VK_PIPELINE_BIND_POINT_GRAPHICS,
                    m_graphicsPipeline);
  vkCmdBindDescriptorSets(cmdBuf, VK_PIPELINE_BIND_POINT_GRAPHICS,
                          m_pipelineLayout, 0, 1, &m_descSet, 0, nullptr);

  auto nbInst = static_cast<uint32_t>(m_instances.size() -
                                      1);  // Remove the implicit object
  for (uint32_t i = 0; i < nbInst; ++i) {
    auto& inst             = m_instances[i];
    auto& model            = m_objModel[inst.objIndex];
    m_pcRaster.objIndex    = inst.objIndex;  // Telling which object is drawn
    m_pcRaster.modelMatrix = inst.transform;

    vkCmdPushConstants(
        cmdBuf, m_pipelineLayout,
        VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, 0,
        sizeof(PushConstantRaster), &m_pcRaster);
    vkCmdBindVertexBuffers(cmdBuf, 0, 1, &model.vertexBuffer.buffer, &offset);
    vkCmdBindIndexBuffer(cmdBuf, model.indexBuffer.buffer, 0,
                         VK_INDEX_TYPE_UINT32);
    vkCmdDrawIndexed(cmdBuf, model.nbIndices, 1, 0, 0, 0);
  }
  m_debug.endLabel(cmdBuf);
}

//--------------------------------------------------------------------------------------------------
// Handling resize of the window
//
void Renderer::onResize(int /*w*/, int /*h*/) {
  createOffscreenRender();
  updatePostDescriptorSet();
  updateRtDescriptorSet();
}

//////////////////////////////////////////////////////////////////////////
// Post-processing
//////////////////////////////////////////////////////////////////////////

//--------------------------------------------------------------------------------------------------
// Creating an offscreen frame buffer and the associated render pass
//
void Renderer::createOffscreenRender() {
  m_alloc.destroy(m_offscreenColor);
  m_alloc.destroy(m_offscreenDepth);

  // Creating the color image
  {
    auto colorCreateInfo = nvvk::makeImage2DCreateInfo(
        m_size, m_offscreenColorFormat,
        VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT |
            VK_IMAGE_USAGE_STORAGE_BIT);

    nvvk::Image image = m_alloc.createImage(colorCreateInfo);
    VkImageViewCreateInfo ivInfo =
        nvvk::makeImageViewCreateInfo(image.image, colorCreateInfo);
    VkSamplerCreateInfo sampler{VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO};
    m_offscreenColor = m_alloc.createTexture(image, ivInfo, sampler);
    m_offscreenColor.descriptor.imageLayout = VK_IMAGE_LAYOUT_GENERAL;
  }

  // Creating the depth buffer
  auto depthCreateInfo =
      nvvk::makeImage2DCreateInfo(m_size, m_offscreenDepthFormat,
                                  VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT);
  {
    nvvk::Image image = m_alloc.createImage(depthCreateInfo);

    VkImageViewCreateInfo depthStencilView{
        VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO};
    depthStencilView.viewType         = VK_IMAGE_VIEW_TYPE_2D;
    depthStencilView.format           = m_offscreenDepthFormat;
    depthStencilView.subresourceRange = {VK_IMAGE_ASPECT_DEPTH_BIT, 0, 1, 0, 1};
    depthStencilView.image            = image.image;

    m_offscreenDepth = m_alloc.createTexture(image, depthStencilView);
  }

  // Setting the image layout for both color and depth
  {
    nvvk::CommandPool genCmdBuf(m_device, m_graphicsQueueIndex);
    auto cmdBuf = genCmdBuf.createCommandBuffer();
    nvvk::cmdBarrierImageLayout(cmdBuf, m_offscreenColor.image,
                                VK_IMAGE_LAYOUT_UNDEFINED,
                                VK_IMAGE_LAYOUT_GENERAL);
    nvvk::cmdBarrierImageLayout(
        cmdBuf, m_offscreenDepth.image, VK_IMAGE_LAYOUT_UNDEFINED,
        VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
        VK_IMAGE_ASPECT_DEPTH_BIT);

    genCmdBuf.submitAndWait(cmdBuf);
  }

  // Creating a renderpass for the offscreen
  if (!m_offscreenRenderPass) {
    m_offscreenRenderPass = nvvk::createRenderPass(
        m_device, {m_offscreenColorFormat}, m_offscreenDepthFormat, 1, true,
        true, VK_IMAGE_LAYOUT_GENERAL, VK_IMAGE_LAYOUT_GENERAL);
  }

  // Creating the frame buffer for offscreen
  std::vector<VkImageView> attachments = {
      m_offscreenColor.descriptor.imageView,
      m_offscreenDepth.descriptor.imageView};

  vkDestroyFramebuffer(m_device, m_offscreenFramebuffer, nullptr);
  VkFramebufferCreateInfo info{VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO};
  info.renderPass      = m_offscreenRenderPass;
  info.attachmentCount = 2;
  info.pAttachments    = attachments.data();
  info.width           = m_size.width;
  info.height          = m_size.height;
  info.layers          = 1;
  vkCreateFramebuffer(m_device, &info, nullptr, &m_offscreenFramebuffer);
}

//--------------------------------------------------------------------------------------------------
// The pipeline is how things are rendered, which shaders, type of primitives,
// depth test and more
//
void Renderer::createPostPipeline() {
  // Push constants in the fragment shader
  VkPushConstantRange pushConstantRanges = {VK_SHADER_STAGE_FRAGMENT_BIT, 0,
                                            sizeof(float)};

  // Creating the pipeline layout
  VkPipelineLayoutCreateInfo createInfo{
      VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO};
  createInfo.setLayoutCount         = 1;
  createInfo.pSetLayouts            = &m_postDescSetLayout;
  createInfo.pushConstantRangeCount = 1;
  createInfo.pPushConstantRanges    = &pushConstantRanges;
  vkCreatePipelineLayout(m_device, &createInfo, nullptr, &m_postPipelineLayout);

  // Pipeline: completely generic, no vertices
  nvvk::GraphicsPipelineGeneratorCombined pipelineGenerator(
      m_device, m_postPipelineLayout, m_renderPass);
  pipelineGenerator.addShader(
      nvh::loadFile("spv/passthrough.vert.spv", true, defaultSearchPaths, true),
      VK_SHADER_STAGE_VERTEX_BIT);
  pipelineGenerator.addShader(
      nvh::loadFile("spv/post.frag.spv", true, defaultSearchPaths, true),
      VK_SHADER_STAGE_FRAGMENT_BIT);
  pipelineGenerator.rasterizationState.cullMode = VK_CULL_MODE_NONE;
  m_postPipeline = pipelineGenerator.createPipeline();
  m_debug.setObjectName(m_postPipeline, "post");
}

//--------------------------------------------------------------------------------------------------
// The descriptor layout is the description of the data that is passed to the
// vertex or the fragment program.
//
void Renderer::createPostDescriptor() {
  m_postDescSetLayoutBind.addBinding(0,
                                     VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
                                     1, VK_SHADER_STAGE_FRAGMENT_BIT);
  m_postDescSetLayout = m_postDescSetLayoutBind.createLayout(m_device);
  m_postDescPool      = m_postDescSetLayoutBind.createPool(m_device);
  m_postDescSet       = nvvk::allocateDescriptorSet(m_device, m_postDescPool,
                                              m_postDescSetLayout);
}

//--------------------------------------------------------------------------------------------------
// Update the output
//
void Renderer::updatePostDescriptorSet() {
  VkWriteDescriptorSet writeDescriptorSets = m_postDescSetLayoutBind.makeWrite(
      m_postDescSet, 0, &m_offscreenColor.descriptor);
  vkUpdateDescriptorSets(m_device, 1, &writeDescriptorSets, 0, nullptr);
}

//--------------------------------------------------------------------------------------------------
// The Restir descriptor layout is the description of the data that is passed to
// the vertex or the fragment program.
//
void Renderer::createRestirPostDescriptor() {
  m_restirPostDescSetLayoutBind.addBinding(0, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE,
                                           1, VK_SHADER_STAGE_FRAGMENT_BIT);
  m_restirPostDescSetLayout =
      m_restirPostDescSetLayoutBind.createLayout(m_device);
  m_restirPostDescPool = m_restirPostDescSetLayoutBind.createPool(m_device);
  m_restirPostDescSet  = nvvk::allocateDescriptorSet(
      m_device, m_restirPostDescPool, m_restirPostDescSetLayout);
}

//--------------------------------------------------------------------------------------------------
// Update the Restir output
//
void Renderer::updateRestirPostDescriptorSet() {
  VkWriteDescriptorSet writeDescriptorSets =
      m_restirPostDescSetLayoutBind.makeWrite(m_restirPostDescSet, 0,
                                              &m_storageImage.descriptor);
  vkUpdateDescriptorSets(m_device, 1, &writeDescriptorSets, 0, nullptr);
}

//--------------------------------------------------------------------------------------------------
// Create post pipeline for Restir
//
void Renderer::createRestirPostPipeline() {
  // Creating the pipeline layout

  // pushing time
  VkPushConstantRange constants              = {VK_SHADER_STAGE_FRAGMENT_BIT, 0,
                                   sizeof(PushConstantRestir)};
  std::vector<VkDescriptorSetLayout> layouts = {
      m_restirUniformDescSetLayout, m_lightDescSetLayout, m_restirDescSetLayout,
      m_restirPostDescSetLayout};
  VkPipelineLayoutCreateInfo createInfo{
      VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO};
  createInfo.setLayoutCount         = static_cast<uint32_t>(layouts.size());
  createInfo.pSetLayouts            = layouts.data();
  createInfo.pushConstantRangeCount = 1;
  createInfo.pPushConstantRanges    = &constants;
  vkCreatePipelineLayout(m_device, &createInfo, nullptr,
                         &m_restirPostPipelineLayout);

  // Pipeline: completely generic, no vertices
  nvvk::GraphicsPipelineGeneratorCombined pipelineGenerator(
      m_device, m_restirPostPipelineLayout, m_renderPass);
  pipelineGenerator.addShader(
      nvh::loadFile("spv/restir_quad.vert.spv", true, defaultSearchPaths, true),
      VK_SHADER_STAGE_VERTEX_BIT);
  pipelineGenerator.addShader(
      nvh::loadFile("spv/restir_post.frag.spv", true, defaultSearchPaths, true),
      VK_SHADER_STAGE_FRAGMENT_BIT);
  pipelineGenerator.rasterizationState.cullMode = VK_CULL_MODE_NONE;
  m_restirPostPipeline = pipelineGenerator.createPipeline();
  m_debug.setObjectName(m_restirPostPipeline, "RestirPost");
}

//--------------------------------------------------------------------------------------------------
// Draw a full screen quad with the attached image
//
void Renderer::drawPost(VkCommandBuffer cmdBuf) {
  m_debug.beginLabel(cmdBuf, "Post");

  setViewport(cmdBuf);

  auto aspectRatio =
      static_cast<float>(m_size.width) / static_cast<float>(m_size.height);
  vkCmdPushConstants(cmdBuf, m_postPipelineLayout, VK_SHADER_STAGE_FRAGMENT_BIT,
                     0, sizeof(float), &aspectRatio);
  vkCmdBindPipeline(cmdBuf, VK_PIPELINE_BIND_POINT_GRAPHICS, m_postPipeline);
  vkCmdBindDescriptorSets(cmdBuf, VK_PIPELINE_BIND_POINT_GRAPHICS,
                          m_postPipelineLayout, 0, 1, &m_postDescSet, 0,
                          nullptr);
  vkCmdDraw(cmdBuf, 3, 1, 0, 0);

  m_debug.endLabel(cmdBuf);
}

//--------------------------------------------------------------------------------------------------
// Draw a full screen quad with the attached image
//
void Renderer::restirDrawPost(VkCommandBuffer cmdBuf) {
  m_debug.beginLabel(cmdBuf, "restirPost");

  setViewport(cmdBuf);

  vkCmdBindPipeline(cmdBuf, VK_PIPELINE_BIND_POINT_GRAPHICS,
                    m_restirPostPipeline);
  std::vector<VkDescriptorSet> descriptorSets = {
      m_restirUniformDescSet, m_lightDescSet,
      m_restirDescSets[getCurrentFrameIdx()], m_restirPostDescSet};
  vkCmdBindDescriptorSets(cmdBuf, VK_PIPELINE_BIND_POINT_GRAPHICS,
                          m_restirPostPipelineLayout, 0,
                          static_cast<uint32_t>(descriptorSets.size()),
                          descriptorSets.data(), 0, nullptr);
  vkCmdDraw(cmdBuf, 3, 1, 0, 0);

  m_debug.endLabel(cmdBuf);
}

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

//--------------------------------------------------------------------------------------------------
// Initialize Vulkan ray tracing
// #VKRay
void Renderer::initRayTracing() {
  // Requesting ray tracing properties
  VkPhysicalDeviceProperties2 prop2{
      VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROPERTIES_2};
  prop2.pNext = &m_rtProperties;
  vkGetPhysicalDeviceProperties2(m_physicalDevice, &prop2);

  m_rtBuilder.setup(m_device, &m_alloc, m_graphicsQueueIndex);
  spdlog::info("Set up m_rtBuilder");
}

//--------------------------------------------------------------------------------------------------
// Convert an OBJ model into the ray tracing geometry used to build the BLAS
//
auto Renderer::objectToVkGeometryKHR(const ObjModel& model) {
  // BLAS builder requires raw device addresses.
  VkDeviceAddress vertexAddress =
      nvvk::getBufferDeviceAddress(m_device, model.vertexBuffer.buffer);
  VkDeviceAddress indexAddress =
      nvvk::getBufferDeviceAddress(m_device, model.indexBuffer.buffer);

  uint32_t maxPrimitiveCount = model.nbIndices / 3;

  // Describe buffer as array of VertexObj.
  VkAccelerationStructureGeometryTrianglesDataKHR triangles{
      VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_TRIANGLES_DATA_KHR};
  triangles.vertexFormat =
      VK_FORMAT_R32G32B32_SFLOAT;  // vec3 vertex position data.
  triangles.vertexData.deviceAddress = vertexAddress;
  triangles.vertexStride             = sizeof(VertexObj);
  // Describe index data (32-bit unsigned int)
  triangles.indexType               = VK_INDEX_TYPE_UINT32;
  triangles.indexData.deviceAddress = indexAddress;
  // Indicate identity transform by setting transformData to null device
  // pointer.
  // triangles.transformData = {};
  triangles.maxVertex = model.nbVertices;

  // Identify the above data as containing opaque triangles.
  VkAccelerationStructureGeometryKHR asGeom{
      VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_KHR};
  asGeom.geometryType       = VK_GEOMETRY_TYPE_TRIANGLES_KHR;
  asGeom.flags              = VK_GEOMETRY_OPAQUE_BIT_KHR;
  asGeom.geometry.triangles = triangles;

  // The entire array will be used to build the BLAS.
  VkAccelerationStructureBuildRangeInfoKHR offset;
  offset.firstVertex     = 0;
  offset.primitiveCount  = maxPrimitiveCount;
  offset.primitiveOffset = 0;
  offset.transformOffset = 0;

  // Our blas is made from only one geometry, but could be made of many
  // geometries
  nvvk::RaytracingBuilderKHR::BlasInput input;
  input.asGeometry.emplace_back(asGeom);
  input.asBuildOffsetInfo.emplace_back(offset);

  return input;
}

//--------------------------------------------------------------------------------------------------
// Convert an GLTF model into the ray tracing geometry used to build the BLAS
//
nvvk::RaytracingBuilderKHR::BlasInput Renderer::gltfToGeometryKHR(
    const VkDevice& device, const nvh::GltfPrimMesh& prim) {
  // Building part
  VkDeviceAddress vertexAddress =
      nvvk::getBufferDeviceAddress(device, m_gltfVertices.buffer);
  VkDeviceAddress indexAddress =
      nvvk::getBufferDeviceAddress(device, m_gltfIndices.buffer);

  VkAccelerationStructureGeometryTrianglesDataKHR triangles{
      VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_TRIANGLES_DATA_KHR};
  triangles.vertexFormat             = VK_FORMAT_R32G32B32_SFLOAT;
  triangles.vertexData.deviceAddress = vertexAddress;
  triangles.vertexStride             = sizeof(nvmath::vec3f);
  triangles.indexType                = VK_INDEX_TYPE_UINT32;
  triangles.indexData.deviceAddress  = indexAddress;
  triangles.maxVertex                = prim.vertexCount;

  // Setting up the build info of the acceleration
  VkAccelerationStructureGeometryKHR asGeom{
      VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_KHR};
  asGeom.geometryType       = VK_GEOMETRY_TYPE_TRIANGLES_KHR;
  asGeom.flags              = VK_GEOMETRY_OPAQUE_BIT_KHR;
  asGeom.geometry.triangles = triangles;

  VkAccelerationStructureBuildRangeInfoKHR offset;
  offset.firstVertex     = prim.vertexOffset;
  offset.primitiveCount  = prim.indexCount / 3;
  offset.primitiveOffset = prim.firstIndex * sizeof(uint32_t);
  offset.transformOffset = 0;

  nvvk::RaytracingBuilderKHR::BlasInput input;
  input.asGeometry.emplace_back(asGeom);
  input.asBuildOffsetInfo.emplace_back(offset);
  return input;
}

//--------------------------------------------------------------------------------------------------
// Returning the ray tracing geometry used for the BLAS, containing all spheres
//
auto Renderer::sphereToVkGeometryKHR() {
  VkDeviceAddress dataAddress =
      nvvk::getBufferDeviceAddress(m_device, m_spheresAabbBuffer.buffer);

  VkAccelerationStructureGeometryAabbsDataKHR aabbs{
      VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_AABBS_DATA_KHR};
  aabbs.data.deviceAddress = dataAddress;
  aabbs.stride             = sizeof(Aabb);

  // Setting up the build info of the acceleration (C version, c++ gives wrong
  // type)
  VkAccelerationStructureGeometryKHR asGeom{
      VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_KHR};
  asGeom.geometryType   = VK_GEOMETRY_TYPE_AABBS_KHR;
  asGeom.flags          = VK_GEOMETRY_OPAQUE_BIT_KHR;
  asGeom.geometry.aabbs = aabbs;

  VkAccelerationStructureBuildRangeInfoKHR offset{};
  offset.firstVertex     = 0;
  offset.primitiveCount  = (uint32_t)m_spheres.size();  // Nb aabb
  offset.primitiveOffset = 0;
  offset.transformOffset = 0;

  nvvk::RaytracingBuilderKHR::BlasInput input;
  input.asGeometry.emplace_back(asGeom);
  input.asBuildOffsetInfo.emplace_back(offset);
  return input;
}

//--------------------------------------------------------------------------------------------------
// Creating all spheres
//
void Renderer::createVDBBuffer() {
  // All VDB points
  VDB* vdb = nullptr;
  if (SingletonManager::GetVDBLoader().IsVDBLoaded()) {
    vdb = SingletonManager::GetVDBLoader().GetPtr();
  }

  if (!vdb) {
    spdlog::warn("VDB is not loaded; Exiting...");
    return;
  }

  float scaleFactor         = 0.05;
  nvmath::mat3f scaleMatrix = nvmath::mat3f(1.0);
  scaleMatrix *= scaleFactor;
  nvmath::vec3f translation{-2.5, 0.5f, 0};
  nvmath::vec3f translation2{1.0f, 0.0f, 0};

  uint32_t nbSpheres = static_cast<uint32_t>(vdb->AllPoints.size());
  // nbSpheres = 1000;
  m_spheres.resize(nbSpheres);
  for (size_t i = 0; i < nbSpheres; i++) {
    Sphere s;
    s.center =
        scaleMatrix * nvmath::vec3f(vdb->AllPoints[i].x, vdb->AllPoints[i].y,
                                    vdb->AllPoints[i].z);
    s.center += translation;
    s.radius          = 0.005;
    nvmath::vec3f vel = nvmath::vec3f(
        vdb->AllPoints[i].vx, vdb->AllPoints[i].vy, vdb->AllPoints[i].vz);
    m_spheres[i] = std::move(s);
  }
#ifdef USE_ANIMATION
  int sphereAnimate = nbSpheres / 10;
  m_spheresVelocity.resize(nbSpheres);
  for (size_t i = 0; i < sphereAnimate; i++) {
    Velocity v;
    v.velocity = nvmath::vec3f(vdb->AllPoints[i].vx, vdb->AllPoints[i].vy,
                               vdb->AllPoints[i].vz);

    // s.acceleration = nvmath::vec3f(0.f, -9.8f, 0.f);  // gravity
    m_spheresVelocity[i] = std::move(v);
  }

  for (int i = sphereAnimate; i < nbSpheres; i++) {
    Velocity v;
    v.velocity = nvmath::vec3f(0.0f, 0.0f, 0.0f);

    // s.acceleration = nvmath::vec3f(0.f, -9.8f, 0.f);  // gravity
    m_spheresVelocity[i] = std::move(v);
  }

#endif

  // Axis aligned bounding box of each sphere
  std::vector<Aabb> aabbs;
  aabbs.reserve(nbSpheres);
  for (const auto& s : m_spheres) {
    Aabb aabb;
    nvmath::vec3f minimum = s.center - nvmath::vec3f(s.radius);
    nvmath::vec3f maximum = s.center + nvmath::vec3f(s.radius);
    aabb.minimum_x        = minimum.x;
    aabb.minimum_y        = minimum.y;
    aabb.minimum_z        = minimum.z;

    aabb.maximum_x = maximum.x;
    aabb.maximum_y = maximum.y;
    aabb.maximum_z = maximum.z;

    aabbs.emplace_back(aabb);
  }

  // Create array of materials
  std::vector<MaterialObj> materials;
  materials.reserve(nbSpheres);
  for (size_t i = 0; i < m_spheres.size(); ++i) {
    MaterialObj mat;
    mat.diffuse = nvmath::vec3f(vdb->AllPoints[i].cx, vdb->AllPoints[i].cy,
                                vdb->AllPoints[i].cz);
    materials.emplace_back(mat);
  }

  // create Sphere GLTF MAetrial
  for (size_t i = 0; i < m_spheres.size(); ++i) {
    // Create Material for Sphere
    GltfMaterials spheremat;
    spheremat.pbrBaseColorFactor = nvmath::normalize(
        nvmath::vec4(vdb->AllPoints[i].cx, vdb->AllPoints[i].cy,
                     vdb->AllPoints[i].cz, 1));       // Main Color
    spheremat.pbrBaseColorTexture         = 0.0001f;  // For mettalic Color
    spheremat.pbrMetallicFactor           = 0.0001f;  // For mettalic factor
    spheremat.pbrRoughnessFactor          = 0.9;
    spheremat.pbrMetallicRoughnessTexture = -1;
    spheremat.khrDiffuseFactor =
        nvmath::vec4(0.5, 0.1, 0.2, 1);  //// Main Color
    spheremat.khrSpecularFactor   = nvmath::vec3(0.5, 0.5, 0.5);  // Specular
    spheremat.khrDiffuseTexture   = -1;  // emissiveTexture make it -2
    spheremat.shadingModel        = SHADING_MODEL_SPECULAR_GLOSSINESS;  //
    spheremat.khrGlossinessFactor = 0.3;
    spheremat.khrSpecularGlossinessTexture = -1;
    spheremat.emissiveTexture              = -1;  // emissiveTexture make it -2
    spheremat.emissiveFactor = nvmath::vec3(0.3, 0.3, 0.3);  // emmisiveFactor
    // spheremat.alphaMode = m.alphaMode;
    // spheremat.alphaCutoff = m.alphaCutoff;
    // spheremat.doubleSided = m.doubleSided;
    // spheremat.normalTexture = m.normalTexture;
    // spheremat.normalTextureScale = m.normalTextureScale;
    // spheremat.uvTransform = m.uvTransform;
    m_sphereMaterials.emplace_back(spheremat);
  }

  // ORIGINAL -- Creating two materials
  //
  // MaterialObj mat;
  // mat.diffuse = nvmath::vec3f(0, 1, 1);
  // std::vector<MaterialObj> materials;
  //
  // materials.emplace_back(mat);
  // mat.diffuse = nvmath::vec3f(1, 1, 0);
  // materials.emplace_back(mat);

  // Assign a material to each sphere
  std::vector<int> matIdx(nbSpheres);
  for (size_t i = 0; i < m_spheres.size(); i++) {
    matIdx[i] = i;
  }

  // Creating all buffers
  VkBufferUsageFlags flag = VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT;
  VkBufferUsageFlags
      rayTracingFlags =  // used also for building acceleration structures
      flag |
      VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_BUILD_INPUT_READ_ONLY_BIT_KHR |
      VK_BUFFER_USAGE_STORAGE_BUFFER_BIT;
  using vkBU = VkBufferUsageFlagBits;
  nvvk::CommandPool genCmdBuf(m_device, m_graphicsQueueIndex);
  auto cmdBuf         = genCmdBuf.createCommandBuffer();
  m_spheresBuffer     = m_alloc.createBuffer(cmdBuf, m_spheres,
                                         VK_BUFFER_USAGE_STORAGE_BUFFER_BIT);
  m_spheresAabbBuffer = m_alloc.createBuffer(cmdBuf, aabbs, rayTracingFlags);
  m_spheresMatIndexBuffer = m_alloc.createBuffer(
      cmdBuf, matIdx, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | flag);
  m_spheresMatColorBuffer = m_alloc.createBuffer(
      cmdBuf, materials, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | flag);
  m_sphereMaterialsBuffer = m_alloc.createBuffer(
      cmdBuf, m_sphereMaterials, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | flag);

#ifdef USE_ANIMATION
  m_spheresVelocityBuffer = m_alloc.createBuffer(
      cmdBuf, m_spheresVelocity, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | flag);
#endif
  genCmdBuf.submitAndWait(cmdBuf);

  // Debug information
  m_debug.setObjectName(m_spheresBuffer.buffer, "spheres");
  m_debug.setObjectName(m_spheresAabbBuffer.buffer, "spheresAabb");
  m_debug.setObjectName(m_spheresMatColorBuffer.buffer, "spheresMat");
  m_debug.setObjectName(m_spheresMatIndexBuffer.buffer, "spheresMatIdx");
  m_debug.setObjectName(m_sphereMaterialsBuffer.buffer, "spheresMatIdx");

  // Adding an extra instance to get access to the material buffers
  ObjDesc objDesc{};
  objDesc.materialAddress =
      nvvk::getBufferDeviceAddress(m_device, m_spheresMatColorBuffer.buffer);
  objDesc.materialIndexAddress =
      nvvk::getBufferDeviceAddress(m_device, m_spheresMatIndexBuffer.buffer);
  m_objDesc.emplace_back(objDesc);

  ObjInstance instance{};
  instance.transform = nvmath::mat4f(1.f);
  instance.objIndex  = static_cast<uint32_t>(m_objModel.size());
  m_instances.emplace_back(instance);

  spdlog::info("VDB Buffer created");
}

//--------------------------------------------------------------------------------------------------
// Creating ReSTIR Point Lights
//
void Renderer::createRestirLights() {
  // Create the buffers on Device and copy vertices, indices and materials
  nvvk::CommandPool cmdBufGet(m_device, m_graphicsQueueIndex);
  VkCommandBuffer cmdBuf  = cmdBufGet.createCommandBuffer();
  VkBufferUsageFlags flag = VK_BUFFER_USAGE_STORAGE_BUFFER_BIT;

  if (m_pointLights.empty()) {
    // generate random point lights
    nvmath::vec3f min_range, max_range;
    if (SingletonManager::GetGLTFLoader().isSceneLoaded()) {
      auto gltfScene = SingletonManager::GetGLTFLoader().getGLTFScene();
      m_pointLights  = generatePointLights(gltfScene.m_dimensions.min,
                                          gltfScene.m_dimensions.max);
      min_range      = gltfScene.m_dimensions.min;
      max_range      = gltfScene.m_dimensions.max;
    } else {
      m_pointLights = generatePointLights(nvmath::vec3f(-10, -10, -10),
                                          nvmath::vec3f(10, 10, 10));
      min_range     = nvmath::vec3f(-10, -10, -10);
      max_range     = nvmath::vec3f(10, 10, 10);
    }
    spdlog::info("Generated point lights of size {} in range {}, {} :",
                 m_pointLights.size(), min_range, max_range);
  }

  m_pointLights = generatePointLights(nvmath::vec3f(-10, -10, -10),
                                      nvmath::vec3f(10, 10, 10), false, 1000);

  VDB* vdb         = nullptr;
  int lightCounter = 0;
  if (SingletonManager::GetVDBLoader().IsVDBLoaded()) {
    vdb = SingletonManager::GetVDBLoader().GetPtr();

    uint32_t nbSpheres = static_cast<uint32_t>(vdb->AllPoints.size());
    for (int i = 0; i < nbSpheres; i++) {
      if (lightCounter > 1000) {
        break;
      }
      if (vdb->AllPoints[i].temp > 275) {
        lightCounter++;
        PointLight currLight;
        currLight.pos                = m_spheres[i].center;
        currLight.emission_luminance = nvmath::vec4f(0.6, 0.2, 0.1, 1.0);

        currLight.emission_luminance.w = shader::luminance(
            currLight.emission_luminance.x, currLight.emission_luminance.y,
            currLight.emission_luminance.z);
        m_pointLights.push_back(currLight);
      }
    }
  }

  // min_range     = nvmath::vec3f(-10, -10, -10);
  // max_range     = nvmath::vec3f(10, 10, 10);

  if (m_triangleLights.empty()) {
    // triangle lights are only created with a GLTF scene
    // m_triangleLights = collectTriangleLights(gltfScene);
    spdlog::warn(
        "No triangle lights are populated in GLTF scene. Adding one dummy "
        "triangle light...");
    m_triangleLights.push_back(TriangleLight{});
  }

  // create alias table
  std::vector<float> pdf;
  if (!m_pointLights.empty()) {
    for (const auto& pl : m_pointLights) {
      pdf.push_back(pl.emission_luminance.w);
    }
  } else {
    for (const auto& lt : m_triangleLights) {
      float triangleLightPower = lt.emission_luminance.w * lt.normalArea.w;
      pdf.push_back(triangleLightPower);
    }
  }
  std::vector<AliasTableCell> aliasTable = createAliasTable(pdf);
  m_aliasTableCount  = static_cast<uint32_t>(aliasTable.size());
  m_aliasTableBuffer = m_alloc.createBuffer(cmdBuf, aliasTable, flag);
  spdlog::debug("Created aliasTableBuffer");

  // create point light buffers
  if (m_pointLights.size() > 0) {
    m_ptLightsBuffer = m_alloc.createBuffer(cmdBuf, m_pointLights, flag);
    spdlog::debug("Created pointLightsBuffer");
  }

  if (m_triangleLights.size() > 0) {
    m_triangleLightsBuffer =
        m_alloc.createBuffer(cmdBuf, m_triangleLights, flag);
  }
  spdlog::debug("Created triangleLightsBuffer");

  cmdBufGet.submitAndWait(cmdBuf);
  m_alloc.finalizeAndReleaseStaging();

  // debug information
  if (m_pointLights.size() > 0) {
    m_debug.setObjectName(m_ptLightsBuffer.buffer, "pointLights");
  }
  if (m_triangleLights.size() > 0) {
    m_debug.setObjectName(m_triangleLightsBuffer.buffer, "triangleLights");
  }
  m_debug.setObjectName(m_aliasTableBuffer.buffer, "aliasTable");
}

//--------------------------------------------------------------------------------------------------
//
//
void Renderer::createBottomLevelAS() {
  // BLAS - Storing each primitive in a geometry

#ifdef USE_GLTF
  const auto gltfScene = SingletonManager::GetGLTFLoader().getGLTFScene();
  allBlas.reserve(gltfScene.m_primMeshes.size() + 1);
  for (auto& primMesh : gltfScene.m_primMeshes) {
    auto geo = gltfToGeometryKHR(m_device, primMesh);
    allBlas.push_back({geo});
  }
#else
  allBlas.reserve(m_objModel.size() + 1);
  for (const auto& obj : m_objModel) {
    auto blas = objectToVkGeometryKHR(obj);

    // We could add more geometry in each BLAS, but we add only one for now
    allBlas.emplace_back(blas);
  }
#endif

  // Spheres
#ifdef USE_VDB
  {
    auto blas = sphereToVkGeometryKHR();
    allBlas.emplace_back(blas);
  }
#endif  // USE_VDB

  m_rtBuilder.buildBlas(
      allBlas, VK_BUILD_ACCELERATION_STRUCTURE_ALLOW_UPDATE_BIT_KHR |
                   VK_BUILD_ACCELERATION_STRUCTURE_PREFER_FAST_BUILD_BIT_KHR);
  spdlog::info("Created BLAS for ray tracing");
}

//--------------------------------------------------------------------------------------------------
//
//
void Renderer::createTopLevelAS() {
#ifdef USE_GLTF
  const auto gltfScene = SingletonManager::GetGLTFLoader().getGLTFScene();
#ifdef USE_VDB
  tlas.reserve(gltfScene.m_nodes.size() + 1);
#else
  tlas.reserve(gltfScene.m_nodes.size());
#endif  // USE_VDB
  for (auto& node : gltfScene.m_nodes) {
    VkAccelerationStructureInstanceKHR rayInst;
    rayInst.transform = nvvk::toTransformMatrixKHR(node.worldMatrix);
    rayInst.instanceCustomIndex =
        node.primMesh;  // gl_InstanceCustomIndexEXT: to find which primitive
    rayInst.accelerationStructureReference =
        m_rtBuilder.getBlasDeviceAddress(node.primMesh);
    rayInst.flags = VK_GEOMETRY_INSTANCE_TRIANGLE_FACING_CULL_DISABLE_BIT_KHR;
    rayInst.mask  = 0xFF;  //  Only be hit if rayMask & instance.mask != 0
    rayInst.instanceShaderBindingTableRecordOffset =
        0;  // We will use the same hit group for all objects
    tlas.emplace_back(rayInst);
  }
#else
#ifdef USE_VDB
  tlas.reserve(m_objModel.size() + 1);
#else
  tlas.reserve(m_objModel.size());
#endif
  for (size_t i = 0; i < m_objModel.size(); i++) {
    const auto& inst = m_instances[i];

    VkAccelerationStructureInstanceKHR rayInst{};
    rayInst.transform =
        nvvk::toTransformMatrixKHR(inst.transform);  // Position of the instance
    rayInst.instanceCustomIndex = inst.objIndex;  // gl_InstanceCustomIndexEXT
    rayInst.accelerationStructureReference =
        m_rtBuilder.getBlasDeviceAddress(inst.objIndex);
    rayInst.flags = VK_GEOMETRY_INSTANCE_TRIANGLE_FACING_CULL_DISABLE_BIT_KHR;
    rayInst.mask  = 0xFF;  //  Only be hit if rayMask & instance.mask != 0
    rayInst.instanceShaderBindingTableRecordOffset =
        0;  // We will use the same hit group for all objects
    tlas.emplace_back(rayInst);
  }
#endif
  // Add the blas containing all implicit objects
#ifdef USE_VDB
  {
    SphereBlasID = gltfScene.m_primMeshes.size();
    VkAccelerationStructureInstanceKHR rayInst{};
    rayInst.transform =
        nvvk::toTransformMatrixKHR(nvmath::mat4f(1));  // (identity)
#ifdef USE_GLTF
    rayInst.instanceCustomIndex =
        static_cast<uint32_t>(gltfScene.m_primMeshes.size());
    rayInst.accelerationStructureReference = m_rtBuilder.getBlasDeviceAddress(
        static_cast<uint32_t>(gltfScene.m_primMeshes.size()));
#else
    rayInst.instanceCustomIndex = static_cast<uint32_t>(
        m_objModel.size());  // nbObj == last object == implicit
    rayInst.accelerationStructureReference = m_rtBuilder.getBlasDeviceAddress(
        static_cast<uint32_t>(m_objModel.size()));
#endif
    rayInst.flags = VK_GEOMETRY_INSTANCE_TRIANGLE_FACING_CULL_DISABLE_BIT_KHR;
    rayInst.mask  = 0xFF;  //  Only be hit if rayMask & instance.mask != 0
    rayInst.instanceShaderBindingTableRecordOffset =
        1;  // We will use the same hit group for all objects
    tlas.emplace_back(rayInst);
  }
#endif  // USE_VDB

  m_rtBuilder.buildTlas(
      tlas, VK_BUILD_ACCELERATION_STRUCTURE_ALLOW_UPDATE_BIT_KHR |
                VK_BUILD_ACCELERATION_STRUCTURE_PREFER_FAST_BUILD_BIT_KHR);
  spdlog::info("Created TLAS for ray tracing");
}

//--------------------------------------------------------------------------------------------------
// This descriptor set holds the Acceleration structure and the output image
//
void Renderer::createRtDescriptorSet() {
  // Top-level acceleration structure, usable by both the ray generation and
  // the closest hit (to shoot shadow rays)
  m_rtDescSetLayoutBind.addBinding(
      RtxBindings::eTlas, VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_KHR, 1,
      VK_SHADER_STAGE_RAYGEN_BIT_KHR |
          VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR);  // TLAS
  m_rtDescSetLayoutBind.addBinding(
      RtxBindings::eOutImage, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 1,
      VK_SHADER_STAGE_RAYGEN_BIT_KHR);  // Output image

  m_rtDescPool      = m_rtDescSetLayoutBind.createPool(m_device);
  m_rtDescSetLayout = m_rtDescSetLayoutBind.createLayout(m_device);

  VkDescriptorSetAllocateInfo allocateInfo{
      VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO};
  allocateInfo.descriptorPool     = m_rtDescPool;
  allocateInfo.descriptorSetCount = 1;
  allocateInfo.pSetLayouts        = &m_rtDescSetLayout;
  vkAllocateDescriptorSets(m_device, &allocateInfo, &m_rtDescSet);

  VkAccelerationStructureKHR tlas = m_rtBuilder.getAccelerationStructure();
  VkWriteDescriptorSetAccelerationStructureKHR descASInfo{
      VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET_ACCELERATION_STRUCTURE_KHR};
  descASInfo.accelerationStructureCount = 1;
  descASInfo.pAccelerationStructures    = &tlas;
  VkDescriptorImageInfo imageInfo{
      {}, m_offscreenColor.descriptor.imageView, VK_IMAGE_LAYOUT_GENERAL};

  std::vector<VkWriteDescriptorSet> writes;
  writes.emplace_back(m_rtDescSetLayoutBind.makeWrite(
      m_rtDescSet, RtxBindings::eTlas, &descASInfo));
  writes.emplace_back(m_rtDescSetLayoutBind.makeWrite(
      m_rtDescSet, RtxBindings::eOutImage, &imageInfo));
  vkUpdateDescriptorSets(m_device, static_cast<uint32_t>(writes.size()),
                         writes.data(), 0, nullptr);
  spdlog::info("Created Ray Tracing AccelerationStructure descriptor set");
}

//--------------------------------------------------------------------------------------------------
// This descriptor set holds the Acceleration structure and the output image
//
void Renderer::createLightDescriptorSet() {
  m_lightDescSetLayoutBind.addBinding(
      LightBindings::ePointLights, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1,
      VK_SHADER_STAGE_FRAGMENT_BIT | VK_SHADER_STAGE_RAYGEN_BIT_KHR |
          VK_SHADER_STAGE_COMPUTE_BIT);
  m_lightDescSetLayoutBind.addBinding(
      LightBindings::eTriangleLights, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1,
      VK_SHADER_STAGE_FRAGMENT_BIT | VK_SHADER_STAGE_RAYGEN_BIT_KHR |
          VK_SHADER_STAGE_COMPUTE_BIT);
  m_lightDescSetLayoutBind.addBinding(
      LightBindings::eAliasTable, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1,
      VK_SHADER_STAGE_FRAGMENT_BIT | VK_SHADER_STAGE_RAYGEN_BIT_KHR |
          VK_SHADER_STAGE_COMPUTE_BIT);

  m_lightDescSetLayout = m_lightDescSetLayoutBind.createLayout(m_device);
  m_lightDescPool      = m_lightDescSetLayoutBind.createPool(m_device);
  m_lightDescSet       = nvvk::allocateDescriptorSet(m_device, m_lightDescPool,
                                               m_lightDescSetLayout);

  // write to the allocated descriptor sets
  std::vector<VkWriteDescriptorSet> writes;

  VkDescriptorBufferInfo dbiPointLights{m_ptLightsBuffer.buffer, 0,
                                        VK_WHOLE_SIZE};
  writes.emplace_back(m_lightDescSetLayoutBind.makeWrite(
      m_lightDescSet, LightBindings::ePointLights, &dbiPointLights));

  VkDescriptorBufferInfo dbiTriangleLights{m_triangleLightsBuffer.buffer, 0,
                                           VK_WHOLE_SIZE};
  writes.emplace_back(m_lightDescSetLayoutBind.makeWrite(
      m_lightDescSet, LightBindings::eTriangleLights, &dbiTriangleLights));

  VkDescriptorBufferInfo dbialiasTable{m_aliasTableBuffer.buffer, 0,
                                       VK_WHOLE_SIZE};
  writes.emplace_back(m_lightDescSetLayoutBind.makeWrite(
      m_lightDescSet, LightBindings::eAliasTable, &dbialiasTable));

  vkUpdateDescriptorSets(m_device, static_cast<uint32_t>(writes.size()),
                         writes.data(), 0, nullptr);
  spdlog::info("Created ReSTIR Light descriptor set");
}

void Renderer::createRestirDescriptorSet() {
  // IMPORTANT:
  // allocate enough memory for output images for this descriptor pool
  uint32_t maxSets = 100;
  std::vector<VkDescriptorPoolSize> poolSizes{
      {VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, maxSets},
      {VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, maxSets}};
  m_restirDescPool = nvvk::createDescriptorPool(m_device, poolSizes, maxSets);

  m_restirDescSetLayoutBind.addBinding(
      RestirBindings::eFrameWorldPosition, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 1,
      VK_SHADER_STAGE_RAYGEN_BIT_KHR | VK_SHADER_STAGE_FRAGMENT_BIT |
          VK_SHADER_STAGE_COMPUTE_BIT);
  m_restirDescSetLayoutBind.addBinding(
      RestirBindings::eFrameAlbedo, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 1,
      VK_SHADER_STAGE_RAYGEN_BIT_KHR | VK_SHADER_STAGE_FRAGMENT_BIT |
          VK_SHADER_STAGE_COMPUTE_BIT);
  m_restirDescSetLayoutBind.addBinding(
      RestirBindings::eFrameNormal, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 1,
      VK_SHADER_STAGE_RAYGEN_BIT_KHR | VK_SHADER_STAGE_FRAGMENT_BIT |
          VK_SHADER_STAGE_COMPUTE_BIT);
  m_restirDescSetLayoutBind.addBinding(
      RestirBindings::eFrameMaterialProps, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 1,
      VK_SHADER_STAGE_RAYGEN_BIT_KHR | VK_SHADER_STAGE_FRAGMENT_BIT |
          VK_SHADER_STAGE_COMPUTE_BIT);
  m_restirDescSetLayoutBind.addBinding(RestirBindings::ePrevFrameWorldPosition,
                                       VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 1,
                                       VK_SHADER_STAGE_RAYGEN_BIT_KHR |
                                           VK_SHADER_STAGE_FRAGMENT_BIT |
                                           VK_SHADER_STAGE_COMPUTE_BIT);
  m_restirDescSetLayoutBind.addBinding(
      RestirBindings::ePrevFrameAlbedo, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 1,
      VK_SHADER_STAGE_RAYGEN_BIT_KHR | VK_SHADER_STAGE_FRAGMENT_BIT |
          VK_SHADER_STAGE_COMPUTE_BIT);
  m_restirDescSetLayoutBind.addBinding(
      RestirBindings::ePrevFrameNormal, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 1,
      VK_SHADER_STAGE_RAYGEN_BIT_KHR | VK_SHADER_STAGE_FRAGMENT_BIT |
          VK_SHADER_STAGE_COMPUTE_BIT);
  m_restirDescSetLayoutBind.addBinding(RestirBindings::ePrevFrameMaterialProps,
                                       VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 1,
                                       VK_SHADER_STAGE_RAYGEN_BIT_KHR |
                                           VK_SHADER_STAGE_FRAGMENT_BIT |
                                           VK_SHADER_STAGE_COMPUTE_BIT);
  m_restirDescSetLayoutBind.addBinding(
      RestirBindings::eReservoirsInfo, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 1,
      VK_SHADER_STAGE_RAYGEN_BIT_KHR | VK_SHADER_STAGE_FRAGMENT_BIT |
          VK_SHADER_STAGE_COMPUTE_BIT);
  m_restirDescSetLayoutBind.addBinding(
      RestirBindings::eReservoirWeights, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 1,
      VK_SHADER_STAGE_RAYGEN_BIT_KHR | VK_SHADER_STAGE_FRAGMENT_BIT |
          VK_SHADER_STAGE_COMPUTE_BIT);
  m_restirDescSetLayoutBind.addBinding(
      RestirBindings::ePrevReservoirInfo, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 1,
      VK_SHADER_STAGE_RAYGEN_BIT_KHR | VK_SHADER_STAGE_FRAGMENT_BIT |
          VK_SHADER_STAGE_COMPUTE_BIT);
  m_restirDescSetLayoutBind.addBinding(
      RestirBindings::ePrevReservoirWeight, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 1,
      VK_SHADER_STAGE_RAYGEN_BIT_KHR | VK_SHADER_STAGE_FRAGMENT_BIT |
          VK_SHADER_STAGE_COMPUTE_BIT);
  m_restirDescSetLayoutBind.addBinding(
      RestirBindings::eTmpReservoirInfo, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 1,
      VK_SHADER_STAGE_RAYGEN_BIT_KHR | VK_SHADER_STAGE_FRAGMENT_BIT |
          VK_SHADER_STAGE_COMPUTE_BIT);
  m_restirDescSetLayoutBind.addBinding(
      RestirBindings::eTmpReservoirWeight, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 1,
      VK_SHADER_STAGE_RAYGEN_BIT_KHR | VK_SHADER_STAGE_FRAGMENT_BIT |
          VK_SHADER_STAGE_COMPUTE_BIT);
  // TODO: investigate whether we bind this to fragment shader
  m_restirDescSetLayoutBind.addBinding(
      RestirBindings::eStorageImage, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 1,
      VK_SHADER_STAGE_RAYGEN_BIT_KHR | VK_SHADER_STAGE_FRAGMENT_BIT |
          VK_SHADER_STAGE_COMPUTE_BIT);
  m_restirDescSetLayout = m_restirDescSetLayoutBind.createLayout(m_device);

  // allocate descriptor sets
  m_restirDescSets.resize(static_config::kNumGBuffers);
  nvvk::allocateDescriptorSets(m_device, m_restirDescPool,
                               m_restirDescSetLayout,
                               static_config::kNumGBuffers, m_restirDescSets);
  spdlog::info("Created ReSTIR (reservoir) descriptor set");
}

void Renderer::updateRestirDescriptorSet() {
  std::vector<VkWriteDescriptorSet> writes;

  for (uint32_t i = 0; i < static_config::kNumGBuffers; i++) {
    VkDescriptorSet& set   = m_restirDescSets[i];
    const GBuffer& buf     = m_gBuffers[i];
    const GBuffer& bufprev = m_gBuffers[(static_config::kNumGBuffers + i - 1) %
                                        static_config::kNumGBuffers];

    writes.emplace_back(m_restirDescSetLayoutBind.makeWrite(
        set, RestirBindings::eFrameWorldPosition,
        &buf.getWorldPosTexture().descriptor));
    writes.emplace_back(m_restirDescSetLayoutBind.makeWrite(
        set, RestirBindings::eFrameAlbedo, &buf.getAlbedoTexture().descriptor));
    writes.emplace_back(m_restirDescSetLayoutBind.makeWrite(
        set, RestirBindings::eFrameNormal, &buf.getNormalTexture().descriptor));
    writes.emplace_back(m_restirDescSetLayoutBind.makeWrite(
        set, RestirBindings::eFrameMaterialProps,
        &buf.getMaterialPropertiesTexture().descriptor));
    writes.emplace_back(m_restirDescSetLayoutBind.makeWrite(
        set, RestirBindings::ePrevFrameWorldPosition,
        &bufprev.getWorldPosTexture().descriptor));
    writes.emplace_back(m_restirDescSetLayoutBind.makeWrite(
        set, RestirBindings::ePrevFrameAlbedo,
        &bufprev.getAlbedoTexture().descriptor));
    writes.emplace_back(m_restirDescSetLayoutBind.makeWrite(
        set, RestirBindings::ePrevFrameNormal,
        &bufprev.getNormalTexture().descriptor));
    writes.emplace_back(m_restirDescSetLayoutBind.makeWrite(
        set, RestirBindings::ePrevFrameMaterialProps,
        &bufprev.getMaterialPropertiesTexture().descriptor));

    // VkDescriptorImageInfo imgInfo{
    //    {}, m_storageImage.descriptor.imageView, VK_IMAGE_LAYOUT_GENERAL};
    // writes.emplace_back(m_restirDescSetLayoutBind.makeWrite(
    //    set, RestirBindings::eStorageImage, &imgInfo));

    writes.emplace_back(m_restirDescSetLayoutBind.makeWrite(
        set, RestirBindings::eReservoirsInfo,
        &m_reservoirInfoBuffers[i].descriptor));
    writes.emplace_back(m_restirDescSetLayoutBind.makeWrite(
        set, RestirBindings::eReservoirWeights,
        &m_reservoirWeightBuffers[i].descriptor));
    writes.emplace_back(m_restirDescSetLayoutBind.makeWrite(
        set, RestirBindings::ePrevReservoirInfo,
        &m_reservoirInfoBuffers[(static_config::kNumGBuffers + i - 1) %
                                static_config::kNumGBuffers]
             .descriptor));
    writes.emplace_back(m_restirDescSetLayoutBind.makeWrite(
        set, RestirBindings::ePrevReservoirWeight,
        &m_reservoirWeightBuffers[(static_config::kNumGBuffers + i - 1) %
                                  static_config::kNumGBuffers]
             .descriptor));
    writes.emplace_back(m_restirDescSetLayoutBind.makeWrite(
        set, RestirBindings::eTmpReservoirInfo,
        &m_reservoirTmpInfoBuffer.descriptor));
    writes.emplace_back(m_restirDescSetLayoutBind.makeWrite(
        set, RestirBindings::eTmpReservoirWeight,
        &m_reservoirTmpWeightBuffer.descriptor));
  }
  vkUpdateDescriptorSets(m_device, static_cast<uint32_t>(writes.size()),
                         writes.data(), 0, nullptr);
  spdlog::info("Updated ReSTIR (reservoir) descriptor set");
}

//--------------------------------------------------------------------------------------------------
// Writes the output image to the descriptor set
// - Required when changing resolution
//
void Renderer::updateRtDescriptorSet() {
  // (1) Output buffer
  VkDescriptorImageInfo imageInfo{
      {}, m_offscreenColor.descriptor.imageView, VK_IMAGE_LAYOUT_GENERAL};
  VkWriteDescriptorSet wds = m_rtDescSetLayoutBind.makeWrite(
      m_rtDescSet, RtxBindings::eOutImage, &imageInfo);
  vkUpdateDescriptorSets(m_device, 1, &wds, 0, nullptr);
}

//--------------------------------------------------------------------------------------------------
// Pipeline for the ray tracer: all shaders, raygen, chit, miss
//
void Renderer::createRtPipeline() {
  enum StageIndices {
    eRaygen,
    eMiss,
    eMiss2,
    eClosestHit,
    eClosestHit2,
    eIntersection,
    eShaderGroupCount
  };

  // All stages
  std::array<VkPipelineShaderStageCreateInfo, eShaderGroupCount> stages{};
  VkPipelineShaderStageCreateInfo stage{
      VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO};
  stage.pName = "main";  // All the same entry point
  // Raygen
  stage.module = nvvk::createShaderModule(
      m_device,
      nvh::loadFile("spv/raytrace.rgen.spv", true, defaultSearchPaths, true));
  stage.stage     = VK_SHADER_STAGE_RAYGEN_BIT_KHR;
  stages[eRaygen] = stage;
  // Miss
  stage.module = nvvk::createShaderModule(
      m_device,
      nvh::loadFile("spv/raytrace.rmiss.spv", true, defaultSearchPaths, true));
  stage.stage   = VK_SHADER_STAGE_MISS_BIT_KHR;
  stages[eMiss] = stage;
  // The second miss shader is invoked when a shadow ray misses the geometry.
  // It simply indicates that no occlusion has been found
  stage.module = nvvk::createShaderModule(
      m_device, nvh::loadFile("spv/raytraceShadow.rmiss.spv", true,
                              defaultSearchPaths, true));
  stage.stage    = VK_SHADER_STAGE_MISS_BIT_KHR;
  stages[eMiss2] = stage;
  // Hit Group - Closest Hit
  stage.module = nvvk::createShaderModule(
      m_device,
      nvh::loadFile("spv/raytrace.rchit.spv", true, defaultSearchPaths, true));
  stage.stage         = VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR;
  stages[eClosestHit] = stage;
  // Closest hit
  stage.module = nvvk::createShaderModule(
      m_device,
      nvh::loadFile("spv/raytrace2.rchit.spv", true, defaultSearchPaths, true));
  stage.stage          = VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR;
  stages[eClosestHit2] = stage;
  // Intersection
  stage.module = nvvk::createShaderModule(
      m_device,
      nvh::loadFile("spv/raytrace.rint.spv", true, defaultSearchPaths, true));
  stage.stage           = VK_SHADER_STAGE_INTERSECTION_BIT_KHR;
  stages[eIntersection] = stage;

  // Shader groups
  VkRayTracingShaderGroupCreateInfoKHR group{
      VK_STRUCTURE_TYPE_RAY_TRACING_SHADER_GROUP_CREATE_INFO_KHR};
  group.anyHitShader       = VK_SHADER_UNUSED_KHR;
  group.closestHitShader   = VK_SHADER_UNUSED_KHR;
  group.generalShader      = VK_SHADER_UNUSED_KHR;
  group.intersectionShader = VK_SHADER_UNUSED_KHR;

  // Raygen
  group.type          = VK_RAY_TRACING_SHADER_GROUP_TYPE_GENERAL_KHR;
  group.generalShader = eRaygen;
  m_rtShaderGroups.push_back(group);

  // Miss
  group.type          = VK_RAY_TRACING_SHADER_GROUP_TYPE_GENERAL_KHR;
  group.generalShader = eMiss;
  m_rtShaderGroups.push_back(group);

  // Shadow Miss
  group.type          = VK_RAY_TRACING_SHADER_GROUP_TYPE_GENERAL_KHR;
  group.generalShader = eMiss2;
  m_rtShaderGroups.push_back(group);

  // closest hit shader
  group.type = VK_RAY_TRACING_SHADER_GROUP_TYPE_TRIANGLES_HIT_GROUP_KHR;
  group.generalShader    = VK_SHADER_UNUSED_KHR;
  group.closestHitShader = eClosestHit;
  m_rtShaderGroups.push_back(group);

  // closest hit shader + Intersection
  group.type = VK_RAY_TRACING_SHADER_GROUP_TYPE_PROCEDURAL_HIT_GROUP_KHR;
  group.closestHitShader   = eClosestHit2;
  group.intersectionShader = eIntersection;
  m_rtShaderGroups.push_back(group);

  // Push constant: we want to be able to update constants used by the shaders
  VkPushConstantRange pushConstant{VK_SHADER_STAGE_RAYGEN_BIT_KHR |
                                       VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR |
                                       VK_SHADER_STAGE_MISS_BIT_KHR,
                                   0, sizeof(PushConstantRay)};

  VkPipelineLayoutCreateInfo pipelineLayoutCreateInfo{
      VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO};
  pipelineLayoutCreateInfo.pushConstantRangeCount = 1;
  pipelineLayoutCreateInfo.pPushConstantRanges    = &pushConstant;

  // Descriptor sets: one specific to ray tracing, and one shared with the
  // rasterization pipeline
  std::vector<VkDescriptorSetLayout> rtDescSetLayouts = {m_rtDescSetLayout,
                                                         m_descSetLayout};
  pipelineLayoutCreateInfo.setLayoutCount =
      static_cast<uint32_t>(rtDescSetLayouts.size());
  pipelineLayoutCreateInfo.pSetLayouts = rtDescSetLayouts.data();

  vkCreatePipelineLayout(m_device, &pipelineLayoutCreateInfo, nullptr,
                         &m_rtPipelineLayout);

  // Assemble the shader stages and recursion depth info into the ray tracing
  // pipeline
  VkRayTracingPipelineCreateInfoKHR rayPipelineInfo{
      VK_STRUCTURE_TYPE_RAY_TRACING_PIPELINE_CREATE_INFO_KHR};
  rayPipelineInfo.stageCount =
      static_cast<uint32_t>(stages.size());  // Stages are shaders
  rayPipelineInfo.pStages = stages.data();

  // In this case, m_rtShaderGroups.size() == 4: we have one raygen group,
  // two miss shader groups, and one hit group.
  rayPipelineInfo.groupCount = static_cast<uint32_t>(m_rtShaderGroups.size());
  rayPipelineInfo.pGroups    = m_rtShaderGroups.data();

  // The ray tracing process can shoot rays from the camera, and a shadow ray
  // can be shot from the hit points of the camera rays, hence a recursion
  // level of 2. This number should be kept as low as possible for performance
  // reasons. Even recursive ray tracing should be flattened into a loop in
  // the ray generation to avoid deep recursion.
  rayPipelineInfo.maxPipelineRayRecursionDepth = 2;  // Ray depth
  rayPipelineInfo.layout                       = m_rtPipelineLayout;

  vkCreateRayTracingPipelinesKHR(m_device, {}, {}, 1, &rayPipelineInfo, nullptr,
                                 &m_rtPipeline);

  for (auto& s : stages) vkDestroyShaderModule(m_device, s.module, nullptr);
  spdlog::info("Created ray tracing pipeline");
}

//--------------------------------------------------------------------------------------------------
// The Shader Binding Table (SBT)
// - getting all shader handles and write them in a SBT buffer
// - Besides exception, this could be always done like this
//
void Renderer::createRtShaderBindingTable() {
  uint32_t missCount{2};
  uint32_t hitCount{2};
  auto handleCount    = 1 + missCount + hitCount;
  uint32_t handleSize = m_rtProperties.shaderGroupHandleSize;

  // The SBT (buffer) need to have starting groups to be aligned and handles
  // in the group to be aligned.
  uint32_t handleSizeAligned =
      nvh::align_up(handleSize, m_rtProperties.shaderGroupHandleAlignment);

  m_rgenRegion.stride =
      nvh::align_up(handleSizeAligned, m_rtProperties.shaderGroupBaseAlignment);
  m_rgenRegion.size =
      m_rgenRegion.stride;  // The size member of pRayGenShaderBindingTable
                            // must be equal to its stride member
  m_missRegion.stride = handleSizeAligned;
  m_missRegion.size   = nvh::align_up(missCount * handleSizeAligned,
                                    m_rtProperties.shaderGroupBaseAlignment);
  m_hitRegion.stride  = handleSizeAligned;
  m_hitRegion.size    = nvh::align_up(hitCount * handleSizeAligned,
                                   m_rtProperties.shaderGroupBaseAlignment);

  // Get the shader group handles
  uint32_t dataSize = handleCount * handleSize;
  std::vector<uint8_t> handles(dataSize);
  auto result = vkGetRayTracingShaderGroupHandlesKHR(
      m_device, m_rtPipeline, 0, handleCount, dataSize, handles.data());
  assert(result == VK_SUCCESS);

  // Allocate a buffer for storing the SBT.
  VkDeviceSize sbtSize = m_rgenRegion.size + m_missRegion.size +
                         m_hitRegion.size + m_callRegion.size;
  m_rtSBTBuffer =
      m_alloc.createBuffer(sbtSize,
                           VK_BUFFER_USAGE_TRANSFER_SRC_BIT |
                               VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT |
                               VK_BUFFER_USAGE_SHADER_BINDING_TABLE_BIT_KHR,
                           VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
                               VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
  m_debug.setObjectName(
      m_rtSBTBuffer.buffer,
      std::string("SBT"));  // Give it a debug name for NSight.

  // Find the SBT addresses of each group
  VkBufferDeviceAddressInfo info{VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO,
                                 nullptr, m_rtSBTBuffer.buffer};
  VkDeviceAddress sbtAddress = vkGetBufferDeviceAddress(m_device, &info);
  m_rgenRegion.deviceAddress = sbtAddress;
  m_missRegion.deviceAddress = sbtAddress + m_rgenRegion.size;
  m_hitRegion.deviceAddress =
      sbtAddress + m_rgenRegion.size + m_missRegion.size;

  // Helper to retrieve the handle data
  auto getHandle = [&](int i) { return handles.data() + i * handleSize; };

  // Map the SBT buffer and write in the handles.
  auto* pSBTBuffer = reinterpret_cast<uint8_t*>(m_alloc.map(m_rtSBTBuffer));
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

  m_alloc.unmap(m_rtSBTBuffer);
  m_alloc.finalizeAndReleaseStaging();
  spdlog::info("Created ray tracing SBT");
}

//--------------------------------------------------------------------------------------------------
// Ray Tracing the scene
//
void Renderer::raytrace(const VkCommandBuffer& cmdBuf,
                        const nvmath::vec4f& clearColor) {
  m_debug.beginLabel(cmdBuf, "Ray trace");
  // Initializing push constant values
  m_pcRay.clearColor     = clearColor;
  m_pcRay.lightPosition  = m_pcRaster.lightPosition;
  m_pcRay.lightIntensity = m_pcRaster.lightIntensity;
  m_pcRay.lightType      = m_pcRaster.lightType;

  std::vector<VkDescriptorSet> descSets{m_rtDescSet, m_descSet};
  vkCmdBindPipeline(cmdBuf, VK_PIPELINE_BIND_POINT_RAY_TRACING_KHR,
                    m_rtPipeline);
  vkCmdBindDescriptorSets(cmdBuf, VK_PIPELINE_BIND_POINT_RAY_TRACING_KHR,
                          m_rtPipelineLayout, 0, (uint32_t)descSets.size(),
                          descSets.data(), 0, nullptr);
  vkCmdPushConstants(cmdBuf, m_rtPipelineLayout,
                     VK_SHADER_STAGE_RAYGEN_BIT_KHR |
                         VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR |
                         VK_SHADER_STAGE_MISS_BIT_KHR,
                     0, sizeof(PushConstantRay), &m_pcRay);

  vkCmdTraceRaysKHR(cmdBuf, &m_rgenRegion, &m_missRegion, &m_hitRegion,
                    &m_callRegion, m_size.width, m_size.height, 1);

  m_debug.endLabel(cmdBuf);
}

//--------------------------------------------------------------------------------------------------
// Create ReSTIR pipeline
//
void Renderer::createRestirPipeline() {
  m_restirPass.setup(m_device, m_physicalDevice, m_graphicsQueueIndex,
                     &m_alloc);
  m_restirPass.createRenderPass(m_size);
  m_restirPass.createPipeline(m_rtDescSetLayout, m_descSetLayout,
                              m_restirUniformDescSetLayout,
                              m_lightDescSetLayout, m_restirDescSetLayout);
  spdlog::info("Created ReSTIR pass pipeline");
}

//--------------------------------------------------------------------------------------------------
// Create Spatial Reuse pipeline
//
void Renderer::createSpatialReusePipeline() {
  m_spatialReusePass.setup(m_device, m_physicalDevice, m_graphicsQueueIndex,
                           &m_alloc);
  m_spatialReusePass.createRenderPass(m_size);
  m_spatialReusePass.createPipeline(
      m_rtDescSetLayout, m_descSetLayout, m_restirUniformDescSetLayout,
      m_lightDescSetLayout, m_restirDescSetLayout);
  spdlog::info("Created Spatial Reuse pass pipeline");
}

void Renderer::createRestirUniformBuffer() {
  const float aspectRatio    = m_size.width / static_cast<float>(m_size.height);
  m_restirUniforms.debugMode = 0;
  m_restirUniforms.gamma     = 4.0;  // const but i think can be changed by UI
  m_restirUniforms.screenSize = nvmath::vec2ui(m_size.width, m_size.height);
  m_restirUniforms.flags      = RESTIR_VISIBILITY_REUSE_FLAG |
                           RESTIR_TEMPORAL_REUSE_FLAG |
                           RESTIR_SPATIAL_REUSE_FLAG;   // const
  m_restirUniforms.spatialNeighbors        = 4;         // const
  m_restirUniforms.spatialRadius           = 30.0f;     // const
  m_restirUniforms.initialLightSampleCount = (1 << 6);  // const
  m_restirUniforms.pointLightCount =
      static_cast<int>(m_pointLights.size());  // const
  m_restirUniforms.triangleLightCount =
      static_cast<int>(m_triangleLights.size());  // const

  m_restirUniforms.aliasTableCount               = m_aliasTableCount;  // const
  m_restirUniforms.environmentalPower            = 1.0;  // don't need
  m_restirUniforms.fireflyClampThreshold         = 2.0;  // don't need
  m_restirUniforms.temporalSampleCountMultiplier = 20;   // const
  m_restirUniforms.currCamPos                    = CameraManip.getCamera().eye;
  m_restirUniforms.currFrameProjectionViewMatrix =
      nvmath::perspectiveVK(CameraManip.getFov(), aspectRatio, 0.1f, 1000.0f) *
      CameraManip.getMatrix();
  m_restirUniforms.prevCamPos = CameraManip.getCamera().eye;
  m_restirUniforms.prevFrameProjectionViewMatrix =
      nvmath::perspectiveVK(CameraManip.getFov(), aspectRatio, 0.1f, 1000.0f) *
      CameraManip.getMatrix();

  m_restirUniformBuffer = m_alloc.createBuffer(
      sizeof(RestirUniforms),
      VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
      VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
  m_debug.setObjectName(m_restirUniformBuffer.buffer, "restirUniformBuffer");
  spdlog::info("Created ReSTIR uniform buffer");
}

void Renderer::updateRestirUniformBuffer(const VkCommandBuffer& cmdBuf) {
  // Prepare new UBO contents on host.
  m_restirUniforms.prevCamPos = m_restirUniforms.currCamPos;
  m_restirUniforms.prevFrameProjectionViewMatrix =
      m_restirUniforms.currFrameProjectionViewMatrix;
  m_restirUniforms.screenSize = nvmath::vec2ui(m_size.width, m_size.height);

  const float aspectRatio = m_size.width / static_cast<float>(m_size.height);
  const auto& view        = CameraManip.getMatrix();
  const auto& proj =
      nvmath::perspectiveVK(CameraManip.getFov(), aspectRatio, 0.1f, 1000.0f);
  // proj[1][1] *= -1;  // Inverting Y for Vulkan (not needed with
  // perspectiveVK).
  m_restirUniforms.currCamPos                    = CameraManip.getCamera().eye;
  m_restirUniforms.currFrameProjectionViewMatrix = proj * view;

  // UBO on the device, and what stages access it.
  VkBuffer restirVkBuffer      = m_restirUniformBuffer.buffer;
  auto restirUniformUsageMasks = VK_PIPELINE_STAGE_VERTEX_SHADER_BIT |
                                 VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT |
                                 VK_PIPELINE_STAGE_RAY_TRACING_SHADER_BIT_KHR;

  // Ensure that the modified UBO is not visible to previous frames.
  VkBufferMemoryBarrier beforeBarrier{VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER};
  beforeBarrier.srcAccessMask = VK_ACCESS_SHADER_READ_BIT;
  beforeBarrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
  beforeBarrier.buffer        = restirVkBuffer;
  beforeBarrier.offset        = 0;
  beforeBarrier.size          = sizeof(m_restirUniforms);
  vkCmdPipelineBarrier(cmdBuf, restirUniformUsageMasks,
                       VK_PIPELINE_STAGE_TRANSFER_BIT,
                       VK_DEPENDENCY_DEVICE_GROUP_BIT, 0, nullptr, 1,
                       &beforeBarrier, 0, nullptr);

  // Schedule the host-to-device upload. (m_restirUniforms is copied into the
  // cmd buffer so it is okay to deallocate when the function returns).
  vkCmdUpdateBuffer(cmdBuf, m_restirUniformBuffer.buffer, 0,
                    sizeof(RestirUniforms), &m_restirUniforms);

  // Making sure the updated UBO will be visible.
  VkBufferMemoryBarrier afterBarrier{VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER};
  afterBarrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
  afterBarrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
  afterBarrier.buffer        = restirVkBuffer;
  afterBarrier.offset        = 0;
  afterBarrier.size          = sizeof(m_restirUniforms);
  vkCmdPipelineBarrier(cmdBuf, VK_PIPELINE_STAGE_TRANSFER_BIT,
                       restirUniformUsageMasks, VK_DEPENDENCY_DEVICE_GROUP_BIT,
                       0, nullptr, 1, &afterBarrier, 0, nullptr);
}

void Renderer::createRestirUniformDescriptorSet() {
  m_restirUniformDescSetLayoutBind.addBinding(
      RestirUniformBindings::eUniform, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1,
      VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT |
          VK_SHADER_STAGE_RAYGEN_BIT_KHR | VK_SHADER_STAGE_COMPUTE_BIT |
          VK_SHADER_STAGE_MISS_BIT_KHR);
  m_restirUniformDescSetLayout =
      m_restirUniformDescSetLayoutBind.createLayout(m_device);
  m_restirUniformDescPool =
      m_restirUniformDescSetLayoutBind.createPool(m_device);
  m_restirUniformDescSet = nvvk::allocateDescriptorSet(
      m_device, m_restirUniformDescPool, m_restirUniformDescSetLayout);
}

void Renderer::updateRestirUniformDescriptorSet() {
  std::vector<VkWriteDescriptorSet> writes;

  // Camera matrices and scene description
  VkDescriptorBufferInfo dbiUnif{m_restirUniformBuffer.buffer, 0,
                                 VK_WHOLE_SIZE};
  writes.emplace_back(m_restirUniformDescSetLayoutBind.makeWrite(
      m_restirUniformDescSet, RestirUniformBindings::eUniform, &dbiUnif));

  // Writing the information
  vkUpdateDescriptorSets(m_device, static_cast<uint32_t>(writes.size()),
                         writes.data(), 0, nullptr);
  spdlog::info("Created ReSTIR uniform descriptor set");
}

void Renderer::updateFrame() {
  static nvmath::mat4f refCamMatrix;
  static float refFov{CameraManip.getFov()};

  const auto& m  = CameraManip.getMatrix();
  const auto fov = CameraManip.getFov();

  if (memcmp(&refCamMatrix.a00, &m.a00, sizeof(nvmath::mat4f)) != 0 ||
      refFov != fov) {
    resetFrame();
    refCamMatrix = m;
    refFov       = fov;
  }
  m_pcRestirPost.frame++;
}

void Renderer::resetFrame() { m_pcRestirPost.frame = -1; }
//////////////////////////////////////////////////////////////////////////
// #VK_compute
void Renderer::createCompDescriptors() {
  m_compDescSetLayoutBind.addBinding(0, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1,
                                     VK_SHADER_STAGE_COMPUTE_BIT);
  m_compDescSetLayoutBind.addBinding(1, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1,
                                     VK_SHADER_STAGE_COMPUTE_BIT);
  m_compDescSetLayoutBind.addBinding(2, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1,
                                     VK_SHADER_STAGE_COMPUTE_BIT);

  m_compDescSetLayout = m_compDescSetLayoutBind.createLayout(m_device);
  m_compDescPool      = m_compDescSetLayoutBind.createPool(m_device, 1);
  m_compDescSet       = nvvk::allocateDescriptorSet(m_device, m_compDescPool,
                                              m_compDescSetLayout);
}

void Renderer::updateCompDescriptors(nvvk::Buffer& a_spheresAabbBuffer,
                                     nvvk::Buffer& a_spheres) {
  std::vector<VkWriteDescriptorSet> writes;

  VkDescriptorBufferInfo dbiUnifAabb{a_spheresAabbBuffer.buffer, 0,
                                     VK_WHOLE_SIZE};
  writes.emplace_back(
      m_compDescSetLayoutBind.makeWrite(m_compDescSet, 0, &dbiUnifAabb));

  VkDescriptorBufferInfo dbiUnifSphere{a_spheres.buffer, 0, VK_WHOLE_SIZE};
  writes.emplace_back(
      m_compDescSetLayoutBind.makeWrite(m_compDescSet, 1, &dbiUnifSphere));

  VkDescriptorBufferInfo dbiUnifVelocity{m_spheresVelocityBuffer.buffer, 0,
                                         VK_WHOLE_SIZE};
  writes.emplace_back(
      m_compDescSetLayoutBind.makeWrite(m_compDescSet, 2, &dbiUnifVelocity));

  vkUpdateDescriptorSets(m_device, static_cast<uint32_t>(writes.size()),
                         writes.data(), 0, nullptr);
}

void Renderer::createCompPipelines() {
  // pushing time
  VkPushConstantRange push_constants = {VK_SHADER_STAGE_COMPUTE_BIT, 0,
                                        sizeof(CompPushConstant)};

  VkPipelineLayoutCreateInfo createInfo{
      VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO};
  createInfo.setLayoutCount         = 1;
  createInfo.pSetLayouts            = &m_compDescSetLayout;
  createInfo.pushConstantRangeCount = 1;
  createInfo.pPushConstantRanges    = &push_constants;
  vkCreatePipelineLayout(m_device, &createInfo, nullptr, &m_compPipelineLayout);

  VkComputePipelineCreateInfo computePipelineCreateInfo{
      VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO};
  computePipelineCreateInfo.layout = m_compPipelineLayout;

  computePipelineCreateInfo.stage = nvvk::createShaderStageInfo(
      m_device,
      nvh::loadFile("spv/anim.comp.spv", true, defaultSearchPaths, true),
      VK_SHADER_STAGE_COMPUTE_BIT);

  vkCreateComputePipelines(m_device, {}, 1, &computePipelineCreateInfo, nullptr,
                           &m_compPipeline);

  updateCompDescriptors(m_spheresAabbBuffer, m_spheresBuffer);

  vkDestroyShaderModule(m_device, computePipelineCreateInfo.stage.module,
                        nullptr);
}

//--------------------------------------------------------------------------------------------------
// Animating the sphere vertices using a compute shader
//
void Renderer::animationObject(float t1) {
  // const uint32_t sphereId =
  //    1;  // Get from where we are loading spheres, HArd code for now

  nvvk::CommandPool genCmdBuf(m_device, m_graphicsQueueIndex);
  VkCommandBuffer cmdBuf = genCmdBuf.createCommandBuffer();

  vkCmdBindPipeline(cmdBuf, VK_PIPELINE_BIND_POINT_COMPUTE, m_compPipeline);
  vkCmdBindDescriptorSets(cmdBuf, VK_PIPELINE_BIND_POINT_COMPUTE,
                          m_compPipelineLayout, 0, 1, &m_compDescSet, 0,
                          nullptr);
  float deltaT;
  if (t1 < t0) {
    deltaT = t1;
  } else {
    deltaT = t1 - t0;
  }
  t0 = t1;
  CompPushConstant abc{deltaT, m_spheres.size()};
  // CompPushConstant abc{t1, 9920};
  vkCmdPushConstants(cmdBuf, m_compPipelineLayout, VK_SHADER_STAGE_COMPUTE_BIT,
                     0, sizeof(CompPushConstant), &abc);

  // int height = WORKGROUP_SIZE;
  // int width = m_spheres.size()/ (1024 * 512) + 1;

  int total = m_spheres.size();
  int width = std::ceil(total / 1024);

  vkCmdDispatch(cmdBuf, width, 1,
                1);  // size of sphere is the size we need

  genCmdBuf.submitAndWait(cmdBuf);
  m_rtBuilder.updateBlas(
      SphereBlasID, allBlas[SphereBlasID],
      VK_BUILD_ACCELERATION_STRUCTURE_ALLOW_UPDATE_BIT_KHR |
          VK_BUILD_ACCELERATION_STRUCTURE_PREFER_FAST_BUILD_BIT_KHR);

  VkAccelerationStructureInstanceKHR& tinst = tlas[SphereBlasID];
  tinst.transform = nvvk::toTransformMatrixKHR(nvmath::mat4f(1));
  m_rtBuilder.buildTlas(
      tlas,
      VK_BUILD_ACCELERATION_STRUCTURE_ALLOW_UPDATE_BIT_KHR |
          VK_BUILD_ACCELERATION_STRUCTURE_PREFER_FAST_BUILD_BIT_KHR,
      true);
}
