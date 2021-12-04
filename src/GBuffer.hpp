#pragma once

#include <vulkan/vulkan.hpp>

#ifndef NVVK_ALLOC_DEDICATED
#define NVVK_ALLOC_DEDICATED
#include "nvvk/allocator_vk.hpp"
#endif

#include "GLTFSceneBuffers.h"
#include "shaders/headers/binding.glsl"
#include "utils/nvvkUtils.h"

class GBuffer {
public:
  GBuffer(){};
  [[nodiscard]] vk::Framebuffer getFramebuffer() const { return m_framebuffer; }

  [[nodiscard]] nvvk::Texture getAlbedoTexture() const {
    return m_albedoTexture;
  }
  [[nodiscard]] nvvk::Texture getNormalTexture() const {
    return m_normalTexture;
  }
  [[nodiscard]] nvvk::Texture getMaterialPropertiesTexture() const {
    return m_materialPropertiesTexture;
  }
  [[nodiscard]] nvvk::Texture getWorldPosTexture() const {
    return m_worldPosTexture;
  }

  void transitionLayout();

  void resize(nvvk::AllocatorDedicated* allocator, vk::Device device,
              uint32_t graphicsQueueIndex, vk::Extent2D extent,
              vk::RenderPass& pass);

  [[nodiscard]] void create(nvvk::AllocatorDedicated* allocator,
                            vk::Device device, uint32_t graphicsQueueIndex,
                            vk::Extent2D bufferExtent, vk::RenderPass& pass) {
    resize(allocator, device, graphicsQueueIndex, bufferExtent, pass);
  }

  void destroy();

private:
  vk::Device m_device;
  uint32_t m_graphicsQueueIndex;
  nvvk::AllocatorDedicated* m_allocator;

  nvvk::Texture m_albedoTexture;
  nvvk::Texture m_normalTexture;
  nvvk::Texture m_materialPropertiesTexture;
  nvvk::Texture m_worldPosTexture;

  vk::Framebuffer m_framebuffer;
};
