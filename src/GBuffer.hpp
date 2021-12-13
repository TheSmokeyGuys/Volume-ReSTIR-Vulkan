#include "utils/restir_utils.h"

class GBuffer {
public:
  GBuffer(){};
  [[nodiscard]] VkFramebuffer getFramebuffer() const { return m_framebuffer; }

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

  void resize(nvvk::ResourceAllocatorDma* allocator, VkDevice device,
              uint32_t graphicsQueueIndex, VkExtent2D extent,
              VkRenderPass& pass);

  [[nodiscard]] void create(nvvk::ResourceAllocatorDma* allocator,
                            VkDevice device, uint32_t graphicsQueueIndex,
                            VkExtent2D bufferExtent, VkRenderPass& pass) {
    resize(allocator, device, graphicsQueueIndex, bufferExtent, pass);
  }

  void destroy();

private:
  VkDevice m_device;
  uint32_t m_graphicsQueueIndex;
  nvvk::ResourceAllocatorDma* m_allocator;

  nvvk::Texture m_albedoTexture;
  nvvk::Texture m_normalTexture;
  nvvk::Texture m_materialPropertiesTexture;
  nvvk::Texture m_worldPosTexture;

  VkFramebuffer m_framebuffer{VK_NULL_HANDLE};
};
