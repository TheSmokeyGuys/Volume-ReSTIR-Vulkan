#include "GBuffer.hpp"

#include <cassert>

#include "nvh/fileoperations.hpp"
#include "nvvk/pipeline_vk.hpp"
#include "nvvk/renderpasses_vk.hpp"
#include "nvvk/shaders_vk.hpp"

bool m_formatsInitialized = false;

extern std::vector<std::string> defaultSearchPaths;

void GBuffer::resize(nvvk::AllocatorDedicated* allocator, vk::Device device,
                     uint32_t graphicsQueueIndex, vk::Extent2D extent,
                     vk::RenderPass& pass) {
  m_allocator          = allocator;
  m_device             = device;
  m_graphicsQueueIndex = graphicsQueueIndex;
  device.destroy(m_framebuffer);

  allocator->destroy(m_albedoTexture);
  allocator->destroy(m_normalTexture);
  allocator->destroy(m_materialPropertiesTexture);
  allocator->destroy(m_worldPosTexture);

  nvvk::CommandPool cmdBufGet(m_device, m_graphicsQueueIndex);
  vk::CommandBuffer cmdBuf = cmdBufGet.createCommandBuffer();

  vk::SamplerCreateInfo samplerCreateInfo{{},
                                          vk::Filter::eNearest,
                                          vk::Filter::eNearest,
                                          vk::SamplerMipmapMode::eNearest};
  vk::ImageCreateInfo imageCreateInfo = nvvk::makeImage2DCreateInfo(
      extent, vk::Format::eR32G32B32A32Sfloat,
      vk::ImageUsageFlagBits::eColorAttachment |
          vk::ImageUsageFlagBits::eSampled | vk::ImageUsageFlagBits::eStorage);
  {
    nvvk::Image image = allocator->createImage(imageCreateInfo);
    vk::ImageViewCreateInfo ivInfo =
        nvvk::makeImageViewCreateInfo(image.image, imageCreateInfo);
    m_albedoTexture =
        allocator->createTexture(image, ivInfo, samplerCreateInfo);
    m_albedoTexture.descriptor.imageLayout = VK_IMAGE_LAYOUT_GENERAL;
    nvvk::cmdBarrierImageLayout(cmdBuf, m_albedoTexture.image,
                                vk::ImageLayout::eUndefined,
                                vk::ImageLayout::eGeneral);
  }
  {
    nvvk::Image image = allocator->createImage(imageCreateInfo);
    vk::ImageViewCreateInfo ivInfo =
        nvvk::makeImageViewCreateInfo(image.image, imageCreateInfo);
    m_normalTexture =
        allocator->createTexture(image, ivInfo, samplerCreateInfo);
    m_normalTexture.descriptor.imageLayout = VK_IMAGE_LAYOUT_GENERAL;
    nvvk::cmdBarrierImageLayout(cmdBuf, m_normalTexture.image,
                                vk::ImageLayout::eUndefined,
                                vk::ImageLayout::eGeneral);
  }
  {
    nvvk::Image image = allocator->createImage(imageCreateInfo);
    vk::ImageViewCreateInfo ivInfo =
        nvvk::makeImageViewCreateInfo(image.image, imageCreateInfo);
    m_materialPropertiesTexture =
        allocator->createTexture(image, ivInfo, samplerCreateInfo);
    m_materialPropertiesTexture.descriptor.imageLayout =
        VK_IMAGE_LAYOUT_GENERAL;
    nvvk::cmdBarrierImageLayout(cmdBuf, m_materialPropertiesTexture.image,
                                vk::ImageLayout::eUndefined,
                                vk::ImageLayout::eGeneral);
  }
  {
    nvvk::Image image = allocator->createImage(imageCreateInfo);
    vk::ImageViewCreateInfo ivInfo =
        nvvk::makeImageViewCreateInfo(image.image, imageCreateInfo);
    m_worldPosTexture =
        allocator->createTexture(image, ivInfo, samplerCreateInfo);
    m_worldPosTexture.descriptor.imageLayout = VK_IMAGE_LAYOUT_GENERAL;
    nvvk::cmdBarrierImageLayout(cmdBuf, m_worldPosTexture.image,
                                vk::ImageLayout::eUndefined,
                                vk::ImageLayout::eGeneral);
  }
  cmdBufGet.submitAndWait(cmdBuf);
  allocator->finalizeAndReleaseStaging();
}

void GBuffer::transitionLayout() {
  {
    nvvk::ScopeCommandBuffer cmdBuf(m_device, m_graphicsQueueIndex);
    nvvk::cmdBarrierImageLayout(cmdBuf, m_albedoTexture.image,
                                vk::ImageLayout::eUndefined,
                                vk::ImageLayout::eGeneral);
    nvvk::cmdBarrierImageLayout(cmdBuf, m_normalTexture.image,
                                vk::ImageLayout::eUndefined,
                                vk::ImageLayout::eGeneral);
    nvvk::cmdBarrierImageLayout(cmdBuf, m_materialPropertiesTexture.image,
                                vk::ImageLayout::eUndefined,
                                vk::ImageLayout::eGeneral);
    nvvk::cmdBarrierImageLayout(cmdBuf, m_worldPosTexture.image,
                                vk::ImageLayout::eUndefined,
                                vk::ImageLayout::eGeneral);
  }
  m_allocator->finalizeAndReleaseStaging();
}

void GBuffer::destroy() {
  m_device.destroy(m_framebuffer);
  m_allocator->destroy(m_albedoTexture);
  m_allocator->destroy(m_normalTexture);
  m_allocator->destroy(m_materialPropertiesTexture);
  m_allocator->destroy(m_worldPosTexture);
}