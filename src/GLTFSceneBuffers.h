#pragma once
#include <nvmath/nvmath.h>
#include <nvmath/nvmath_glsltypes.h>

#include <queue>
#include <vulkan/vulkan.hpp>

#include "nvh/gltfscene.hpp"
#include "nvvk/allocator_vk.hpp"
#include "nvvk/appbase_vkpp.hpp"
#include "nvvk/commands_vk.hpp"
#include "nvvk/descriptorsets_vk.hpp"
#include "nvvk/raytraceKHR_vk.hpp"
#include "shaders/headers/binding.glsl"
#include "utils/nvvkUtils.h"
extern bool GeneratePointLight;

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
                            uint32_t graphicsQueueIndex) {
    m_debug.setup(device);
    m_alloc              = alloc;
    m_device             = device;
    m_graphicsQueueIndex = graphicsQueueIndex;
    m_physicalDevice     = physicalDevice;

    using vkBU = vk::BufferUsageFlagBits;
    using vkMP = vk::MemoryPropertyFlagBits;
    nvvk::CommandPool cmdBufGet(device, graphicsQueueIndex);
    vk::CommandBuffer cmdBuf = cmdBufGet.createCommandBuffer();

    m_pointLights    = collectPointLights(gltfScene);
    m_triangleLights = collectTriangleLights(gltfScene);
    if (m_pointLights.empty() && m_triangleLights.empty()) {
      m_pointLights = generatePointLights(gltfScene.m_dimensions.min,
                                          gltfScene.m_dimensions.max);
    }

    _loadEnvironment();

    // Lights
    m_pointLightCount = m_pointLights.size();
    std::vector<float> pdf;
    if (!m_pointLights.empty()) {
      for (auto& lt : m_pointLights) {
        pdf.push_back(lt.emission_luminance.w);
      }
    } else {
      for (auto& lt : m_triangleLights) {
        float triangleLightPower = lt.emission_luminance.w * lt.normalArea.w;
        pdf.push_back(triangleLightPower);
      }
    }
    std::vector<shader::aliasTableCell> aliasTable = createAliasTable(pdf);
    m_aliasTableCount  = static_cast<uint32_t>(aliasTable.size());
    m_aliasTableBuffer = alloc->createBuffer(
        cmdBuf, aliasTable, vkBU::eStorageBuffer, vkMP::eDeviceLocal);

    std::cout << "Point Lights Num: " << m_pointLightCount << std::endl;
    if (m_pointLightCount == 0) {
      // Dummy
      m_pointLights.push_back(shader::pointLight{});
    }
    m_ptLightsBuffer = alloc->createBuffer(
        cmdBuf, m_pointLights, vkBU::eStorageBuffer, vkMP::eDeviceLocal);

    m_triangleLightCount = m_triangleLights.size();
    std::cout << "Tri Lights Num: " << m_triangleLightCount << std::endl;
    if (m_triangleLightCount == 0) {
      // Dummy
      m_triangleLights.push_back(shader::triangleLight{});
    }
    m_triangleLightsBuffer = alloc->createBuffer(
        cmdBuf, m_triangleLights, vkBU::eStorageBuffer, vkMP::eDeviceLocal);

    m_vertices =
        alloc->createBuffer(cmdBuf, gltfScene.m_positions,
                            vkBU::eStorageBuffer | vkBU::eShaderDeviceAddress);
    m_indices =
        alloc->createBuffer(cmdBuf, gltfScene.m_indices,
                            vkBU::eStorageBuffer | vkBU::eShaderDeviceAddress);
    m_normals =
        alloc->createBuffer(cmdBuf, gltfScene.m_normals, vkBU::eStorageBuffer);
    m_texcoords = alloc->createBuffer(cmdBuf, gltfScene.m_texcoords0,
                                      vkBU::eStorageBuffer);
    m_tangents =
        alloc->createBuffer(cmdBuf, gltfScene.m_tangents, vkBU::eStorageBuffer);
    m_colors =
        alloc->createBuffer(cmdBuf, gltfScene.m_colors0, vkBU::eStorageBuffer);

    std::vector<shader::GltfMaterials> shadeMaterials;
    for (auto& m : gltfScene.m_materials) {
      shader::GltfMaterials smat;
      smat.pbrBaseColorFactor           = m.pbrBaseColorFactor;
      smat.pbrBaseColorTexture          = m.pbrBaseColorTexture;
      smat.pbrMetallicFactor            = m.pbrMetallicFactor;
      smat.pbrRoughnessFactor           = m.pbrRoughnessFactor;
      smat.pbrMetallicRoughnessTexture  = m.pbrMetallicRoughnessTexture;
      smat.khrDiffuseFactor             = m.khrDiffuseFactor;
      smat.khrSpecularFactor            = m.khrSpecularFactor;
      smat.khrDiffuseTexture            = m.khrDiffuseTexture;
      smat.shadingModel                 = m.shadingModel;
      smat.khrGlossinessFactor          = m.khrGlossinessFactor;
      smat.khrSpecularGlossinessTexture = m.khrSpecularGlossinessTexture;
      smat.emissiveTexture              = m.emissiveTexture;
      smat.emissiveFactor               = m.emissiveFactor;
      smat.alphaMode                    = m.alphaMode;
      smat.alphaCutoff                  = m.alphaCutoff;
      smat.doubleSided                  = m.doubleSided;
      smat.normalTexture                = m.normalTexture;
      smat.normalTextureScale           = m.normalTextureScale;
      smat.uvTransform                  = m.uvTransform;
      shadeMaterials.emplace_back(smat);
    }

    m_materials =
        alloc->createBuffer(cmdBuf, shadeMaterials, vkBU::eStorageBuffer);
    std::vector<shader::ModelMatrices> nodeMatrices;
    for (auto& node : gltfScene.m_nodes) {
      shader::ModelMatrices mat;
      mat.transform                  = node.worldMatrix;
      mat.transformInverseTransposed = invert(node.worldMatrix);
      nodeMatrices.emplace_back(mat);
    }
    m_matrices =
        alloc->createBuffer(cmdBuf, nodeMatrices, vkBU::eStorageBuffer);

    vk::Format format = vk::Format::eR8G8B8A8Unorm;

    using vkIU = vk::ImageUsageFlagBits;

    vk::SamplerCreateInfo samplerCreateInfo{{},
                                            vk::Filter::eLinear,
                                            vk::Filter::eLinear,
                                            vk::SamplerMipmapMode::eLinear};
    samplerCreateInfo.setMaxLod(FLT_MAX);
    // format = vk::Format::eR8G8B8A8Srgb;

    auto addDefaultTexture = [this, cmdBuf, alloc]() {
      std::array<uint8_t, 4> white = {255, 255, 255, 255};
      m_textures.emplace_back(alloc->createTexture(
          cmdBuf, 4, white.data(),
          nvvk::makeImage2DCreateInfo(vk::Extent2D{1, 1}), {}));
      m_debug.setObjectName(m_textures.back().image, "dummy");
    };
    if (tmodel.images.empty()) {
      // No images, add a default one.
      addDefaultTexture();
    } else {
      m_textures.resize(tmodel.textures.size());
      // load textures
      for (int i = 0; i < tmodel.textures.size(); ++i) {
        int sourceImage = tmodel.textures[i].source;
        if (sourceImage >= tmodel.images.size() || sourceImage < 0) {
          // Incorrect source image
          addDefaultTexture();
          continue;
        }
        auto& gltfimage = tmodel.images[sourceImage];
        if (gltfimage.width == -1 || gltfimage.height == -1 ||
            gltfimage.image.empty()) {
          // Image not present or incorrectly loaded (image.empty)
          addDefaultTexture();
          continue;
        }
        void* buffer            = &gltfimage.image[0];
        VkDeviceSize bufferSize = gltfimage.image.size();
        auto imgSize = vk::Extent2D(gltfimage.width, gltfimage.height);

        // std::cout << "Loading Texture: " << gltfimage.uri << std::endl;
        if (tmodel.textures[i].sampler > -1) {
          // Retrieve the texture sampler
          auto gltfSampler  = tmodel.samplers[tmodel.textures[i].sampler];
          samplerCreateInfo = _gltfSamplerToVulkan(gltfSampler);
        }
        vk::ImageCreateInfo imageCreateInfo =
            nvvk::makeImage2DCreateInfo(imgSize, format, vkIU::eSampled, true);

        nvvk::Image image =
            alloc->createImage(cmdBuf, bufferSize, buffer, imageCreateInfo);
        nvvk::cmdGenerateMipmaps(cmdBuf, image.image, format, imgSize,
                                 imageCreateInfo.mipLevels);
        vk::ImageViewCreateInfo ivInfo =
            nvvk::makeImageViewCreateInfo(image.image, imageCreateInfo);
        m_textures[i] = alloc->createTexture(image, ivInfo, samplerCreateInfo);
        m_debug.setObjectName(m_textures[i].image,
                              std::string("Txt" + std::to_string(i)).c_str());
      }
    }
    cmdBufGet.submitAndWait(cmdBuf);
    alloc->finalizeAndReleaseStaging();

    _createRtBuffer(gltfScene);
  }

  void createDescriptorSet(vk::DescriptorPool& staticDescPool) {
    using vkDT      = vk::DescriptorType;
    using vkSS      = vk::ShaderStageFlagBits;
    using vkDSLB    = vk::DescriptorSetLayoutBinding;
    auto nbTextures = static_cast<uint32_t>(m_textures.size());

    nvvk::DescriptorSetBindings bind;
    bind.addBinding(vkDSLB(B_ACCELERATION_STRUCTURE,
                           vkDT::eAccelerationStructureKHR, 1,
                           vkSS::eRaygenKHR | vkSS::eClosestHitKHR));  // TLAS
    bind.addBinding(
        vkDSLB(B_PLIM_LOOK_UP, vkDT::eStorageBuffer, 1, vkSS::eClosestHitKHR));
    bind.addBinding(
        vkDSLB(B_VERTICES, vkDT::eStorageBuffer, 1, vkSS::eClosestHitKHR));
    bind.addBinding(
        vkDSLB(B_NORMALS, vkDT::eStorageBuffer, 1, vkSS::eClosestHitKHR));
    bind.addBinding(
        vkDSLB(B_TEXCOORDS, vkDT::eStorageBuffer, 1, vkSS::eClosestHitKHR));
    bind.addBinding(
        vkDSLB(B_INDICES, vkDT::eStorageBuffer, 1, vkSS::eClosestHitKHR));
    bind.addBinding(
        vkDSLB(B_MATERIALS, vkDT::eStorageBuffer, 1, vkSS::eClosestHitKHR));
    bind.addBinding(
        vkDSLB(B_MATRICES, vkDT::eStorageBuffer, 1, vkSS::eClosestHitKHR));
    bind.addBinding(vkDSLB(B_TEXTURES, vkDT::eCombinedImageSampler, nbTextures,
                           vkSS::eClosestHitKHR));
    bind.addBinding(
        vkDSLB(B_TANGENTS, vkDT::eStorageBuffer, 1, vkSS::eClosestHitKHR));
    bind.addBinding(
        vkDSLB(B_COLORS, vkDT::eStorageBuffer, 1, vkSS::eClosestHitKHR));

    m_sceneDescSetLayout = bind.createLayout(m_device);
    m_sceneDescPool      = bind.createPool(m_device);

    m_sceneDescSet = m_device.allocateDescriptorSets(
        {m_sceneDescPool, 1, &m_sceneDescSetLayout})[0];

    vk::AccelerationStructureKHR tlas = m_rtBuilder.getAccelerationStructure();
    vk::WriteDescriptorSetAccelerationStructureKHR descASInfo;
    descASInfo.setAccelerationStructureCount(1);
    descASInfo.setPAccelerationStructures(&tlas);

    vk::DescriptorBufferInfo primInfo{m_primlooks.buffer, 0, VK_WHOLE_SIZE};
    vk::DescriptorBufferInfo verInfo{m_vertices.buffer, 0, VK_WHOLE_SIZE};
    vk::DescriptorBufferInfo norInfo{m_normals.buffer, 0, VK_WHOLE_SIZE};
    vk::DescriptorBufferInfo texInfo{m_texcoords.buffer, 0, VK_WHOLE_SIZE};
    vk::DescriptorBufferInfo idxInfo{m_indices.buffer, 0, VK_WHOLE_SIZE};
    vk::DescriptorBufferInfo mateInfo{m_materials.buffer, 0, VK_WHOLE_SIZE};
    vk::DescriptorBufferInfo mtxInfo{m_matrices.buffer, 0, VK_WHOLE_SIZE};
    vk::DescriptorBufferInfo tanInfo{m_tangents.buffer, 0, VK_WHOLE_SIZE};
    vk::DescriptorBufferInfo colInfo{m_colors.buffer, 0, VK_WHOLE_SIZE};

    std::vector<vk::WriteDescriptorSet> writes;
    writes.emplace_back(
        bind.makeWrite(m_sceneDescSet, B_ACCELERATION_STRUCTURE, &descASInfo));
    writes.emplace_back(
        bind.makeWrite(m_sceneDescSet, B_PLIM_LOOK_UP, &primInfo));
    writes.emplace_back(bind.makeWrite(m_sceneDescSet, B_VERTICES, &verInfo));
    writes.emplace_back(bind.makeWrite(m_sceneDescSet, B_NORMALS, &norInfo));
    writes.emplace_back(bind.makeWrite(m_sceneDescSet, B_TEXCOORDS, &texInfo));
    writes.emplace_back(bind.makeWrite(m_sceneDescSet, B_INDICES, &idxInfo));
    writes.emplace_back(bind.makeWrite(m_sceneDescSet, B_MATERIALS, &mateInfo));
    writes.emplace_back(bind.makeWrite(m_sceneDescSet, B_MATRICES, &mtxInfo));
    writes.emplace_back(bind.makeWrite(m_sceneDescSet, B_TANGENTS, &tanInfo));
    writes.emplace_back(bind.makeWrite(m_sceneDescSet, B_COLORS, &colInfo));

    std::vector<vk::DescriptorImageInfo> diit;
    for (auto& texture : m_textures) diit.emplace_back(texture.descriptor);
    writes.emplace_back(
        bind.makeWriteArray(m_sceneDescSet, B_TEXTURES, diit.data()));

    m_device.updateDescriptorSets(static_cast<uint32_t>(writes.size()),
                                  writes.data(), 0, nullptr);
  };

  void destroy() {
    m_alloc->destroy(m_vertices);
    m_alloc->destroy(m_normals);
    m_alloc->destroy(m_texcoords);
    m_alloc->destroy(m_indices);
    m_alloc->destroy(m_matrices);
    m_alloc->destroy(m_materials);
    m_alloc->destroy(m_tangents);
    m_alloc->destroy(m_colors);
    m_alloc->destroy(m_ptLightsBuffer);
    m_alloc->destroy(m_triangleLightsBuffer);
    m_alloc->destroy(m_aliasTableBuffer);
    m_alloc->destroy(m_primlooks);
    for (auto& t : m_textures) {
      m_alloc->destroy(t);
    }
    m_alloc->destroy(m_environmentalTexture);
    m_alloc->destroy(m_environmentAliasMap);

    m_alloc->destroy(m_defaultNormal);
    m_alloc->destroy(m_defaultWhite);

    m_device.destroy(m_sceneDescSetLayout);
    m_device.destroy(m_sceneDescPool);

    m_rtBuilder.destroy();
  }

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

  [[nodiscard]] void _createRtBuffer(const nvh::GltfScene& gltfScene) {
    nvvk::CommandPool cmdBufGet(m_device, m_graphicsQueueIndex);
    vk::CommandBuffer cmdBuf = cmdBufGet.createCommandBuffer();
    auto properties          = m_physicalDevice.getProperties2<
        vk::PhysicalDeviceProperties2,
        vk::PhysicalDeviceRayTracingPipelinePropertiesKHR>();
    m_rtProperties =
        properties.get<vk::PhysicalDeviceRayTracingPipelinePropertiesKHR>();
    m_rtBuilder.setup(m_device, m_alloc, m_graphicsQueueIndex);
    // BLAS - Storing each primitive in a geometry
    std::vector<nvvk::RaytracingBuilderKHR::BlasInput> allBlas;
    allBlas.reserve(gltfScene.m_primMeshes.size());
    for (auto& primMesh : gltfScene.m_primMeshes) {
      auto geo = _primitiveToGeometry(m_device, primMesh);
      allBlas.push_back({geo});
    }
    m_rtBuilder.buildBlas(
        allBlas, vk::BuildAccelerationStructureFlagBitsKHR::ePreferFastTrace);

    std::vector<nvvk::RaytracingBuilderKHR::Instance> tlas;
    tlas.reserve(gltfScene.m_nodes.size());
    uint32_t instID = 0;
    for (auto& node : gltfScene.m_nodes) {
      nvvk::RaytracingBuilderKHR::Instance rayInst;
      rayInst.transform = node.worldMatrix;
      rayInst.instanceCustomId =
          node.primMesh;  // gl_InstanceCustomIndexEXT: to find which primitive
      rayInst.blasId = node.primMesh;
      rayInst.flags = VK_GEOMETRY_INSTANCE_TRIANGLE_FACING_CULL_DISABLE_BIT_KHR;
      rayInst.hitGroupId = 0;  // We will use the same hit group for all objects
      tlas.emplace_back(rayInst);
    }
    m_rtBuilder.buildTlas(
        tlas, vk::BuildAccelerationStructureFlagBitsKHR::ePreferFastTrace);
    std::vector<shader::RtPrimitiveLookup> primLookup;
    for (auto& primMesh : gltfScene.m_primMeshes)
      primLookup.push_back(
          {primMesh.firstIndex, primMesh.vertexOffset, primMesh.materialIndex});
    m_primlooks = m_alloc->createBuffer(
        cmdBuf, primLookup, vk::BufferUsageFlagBits::eStorageBuffer);

    cmdBufGet.submitAndWait(cmdBuf);
    m_alloc->finalizeAndReleaseStaging();
  }
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

  vk::SamplerCreateInfo _gltfSamplerToVulkan(tinygltf::Sampler& tsampler) {
    vk::SamplerCreateInfo vk_sampler;

    std::map<int, vk::Filter> filters;
    filters[9728] = vk::Filter::eNearest;  // NEAREST
    filters[9729] = vk::Filter::eLinear;   // LINEAR
    filters[9984] = vk::Filter::eNearest;  // NEAREST_MIPMAP_NEAREST
    filters[9985] = vk::Filter::eLinear;   // LINEAR_MIPMAP_NEAREST
    filters[9986] = vk::Filter::eNearest;  // NEAREST_MIPMAP_LINEAR
    filters[9987] = vk::Filter::eLinear;   // LINEAR_MIPMAP_LINEAR

    std::map<int, vk::SamplerMipmapMode> mipmap;
    mipmap[9728] = vk::SamplerMipmapMode::eNearest;  // NEAREST
    mipmap[9729] = vk::SamplerMipmapMode::eNearest;  // LINEAR
    mipmap[9984] = vk::SamplerMipmapMode::eNearest;  // NEAREST_MIPMAP_NEAREST
    mipmap[9985] = vk::SamplerMipmapMode::eNearest;  // LINEAR_MIPMAP_NEAREST
    mipmap[9986] = vk::SamplerMipmapMode::eLinear;   // NEAREST_MIPMAP_LINEAR
    mipmap[9987] = vk::SamplerMipmapMode::eLinear;   // LINEAR_MIPMAP_LINEAR

    std::map<int, vk::SamplerAddressMode> addressMode;
    addressMode[33071] = vk::SamplerAddressMode::eClampToEdge;
    addressMode[33648] = vk::SamplerAddressMode::eMirroredRepeat;
    addressMode[10497] = vk::SamplerAddressMode::eRepeat;

    vk_sampler.setMagFilter(filters[tsampler.magFilter]);
    vk_sampler.setMinFilter(filters[tsampler.minFilter]);
    vk_sampler.setMipmapMode(mipmap[tsampler.minFilter]);

    vk_sampler.setAddressModeU(addressMode[tsampler.wrapS]);
    vk_sampler.setAddressModeV(addressMode[tsampler.wrapT]);
    vk_sampler.setAddressModeW(addressMode[tsampler.wrapR]);

    // Always allow LOD
    vk_sampler.maxLod = FLT_MAX;
    return vk_sampler;
  }
  void _loadEnvironment();
};

}  // namespace volume_restir
