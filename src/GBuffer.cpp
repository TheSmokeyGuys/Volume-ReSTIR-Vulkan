#include "GBuffer.hpp"

#include <cassert>

#include "nvh/fileoperations.hpp"
#include "nvvk/commands_vk.hpp"
#include "nvvk/images_vk.hpp"
#include "nvvk/pipeline_vk.hpp"
#include "nvvk/renderpasses_vk.hpp"
#include "nvvk/shaders_vk.hpp"

extern std::vector<std::string> defaultSearchPaths;

void GBuffer::resize(nvvk::ResourceAllocatorDma* allocator, VkDevice device,
                     uint32_t graphicsQueueIndex, VkExtent2D extent,
                     VkRenderPass& pass) {
  m_allocator          = allocator;
  m_device             = device;
  m_graphicsQueueIndex = graphicsQueueIndex;

  if (m_framebuffer != VK_NULL_HANDLE)
    vkDestroyFramebuffer(m_device, m_framebuffer, nullptr);

  allocator->destroy(m_albedoTexture);
  allocator->destroy(m_normalTexture);
  allocator->destroy(m_materialPropertiesTexture);
  allocator->destroy(m_worldPosTexture);

  nvvk::CommandPool cmdBufGet(m_device, m_graphicsQueueIndex);
  VkCommandBuffer cmdBuf = cmdBufGet.createCommandBuffer();

  VkSamplerCreateInfo samplerCreateInfo{VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO};
  samplerCreateInfo.minFilter  = VK_FILTER_NEAREST;
  samplerCreateInfo.magFilter  = VK_FILTER_NEAREST;
  samplerCreateInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_NEAREST;

  VkImageCreateInfo imageCreateInfo = nvvk::makeImage2DCreateInfo(
      extent, VK_FORMAT_R32G32B32A32_SFLOAT,
      VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT |
          VK_IMAGE_USAGE_STORAGE_BIT);
  {
    nvvk::Image image = allocator->createImage(imageCreateInfo);
    VkImageViewCreateInfo ivInfo =
        nvvk::makeImageViewCreateInfo(image.image, imageCreateInfo);
    m_albedoTexture =
        allocator->createTexture(image, ivInfo, samplerCreateInfo);
    m_albedoTexture.descriptor.imageLayout = VK_IMAGE_LAYOUT_GENERAL;
    nvvk::cmdBarrierImageLayout(cmdBuf, m_albedoTexture.image,
                                VK_IMAGE_LAYOUT_UNDEFINED,
                                VK_IMAGE_LAYOUT_GENERAL);
  }
  {
    nvvk::Image image = allocator->createImage(imageCreateInfo);
    VkImageViewCreateInfo ivInfo =
        nvvk::makeImageViewCreateInfo(image.image, imageCreateInfo);
    m_normalTexture =
        allocator->createTexture(image, ivInfo, samplerCreateInfo);
    m_normalTexture.descriptor.imageLayout = VK_IMAGE_LAYOUT_GENERAL;
    nvvk::cmdBarrierImageLayout(cmdBuf, m_normalTexture.image,
                                VK_IMAGE_LAYOUT_UNDEFINED,
                                VK_IMAGE_LAYOUT_GENERAL);
  }
  {
    nvvk::Image image = allocator->createImage(imageCreateInfo);
    VkImageViewCreateInfo ivInfo =
        nvvk::makeImageViewCreateInfo(image.image, imageCreateInfo);
    m_materialPropertiesTexture =
        allocator->createTexture(image, ivInfo, samplerCreateInfo);
    m_materialPropertiesTexture.descriptor.imageLayout =
        VK_IMAGE_LAYOUT_GENERAL;
    nvvk::cmdBarrierImageLayout(cmdBuf, m_materialPropertiesTexture.image,
                                VK_IMAGE_LAYOUT_UNDEFINED,
                                VK_IMAGE_LAYOUT_GENERAL);
  }
  {
    nvvk::Image image = allocator->createImage(imageCreateInfo);
    VkImageViewCreateInfo ivInfo =
        nvvk::makeImageViewCreateInfo(image.image, imageCreateInfo);
    m_worldPosTexture =
        allocator->createTexture(image, ivInfo, samplerCreateInfo);
    m_worldPosTexture.descriptor.imageLayout = VK_IMAGE_LAYOUT_GENERAL;
    nvvk::cmdBarrierImageLayout(cmdBuf, m_worldPosTexture.image,
                                VK_IMAGE_LAYOUT_UNDEFINED,
                                VK_IMAGE_LAYOUT_GENERAL);
  }
  cmdBufGet.submitAndWait(cmdBuf);
  allocator->finalizeAndReleaseStaging();
}

void GBuffer::transitionLayout() {
  {
    nvvk::ScopeCommandBuffer cmdBuf(m_device, m_graphicsQueueIndex);
    nvvk::cmdBarrierImageLayout(cmdBuf, m_albedoTexture.image,
                                VK_IMAGE_LAYOUT_UNDEFINED,
                                VK_IMAGE_LAYOUT_GENERAL);
    nvvk::cmdBarrierImageLayout(cmdBuf, m_normalTexture.image,
                                VK_IMAGE_LAYOUT_UNDEFINED,
                                VK_IMAGE_LAYOUT_GENERAL);
    nvvk::cmdBarrierImageLayout(cmdBuf, m_materialPropertiesTexture.image,
                                VK_IMAGE_LAYOUT_UNDEFINED,
                                VK_IMAGE_LAYOUT_GENERAL);
    nvvk::cmdBarrierImageLayout(cmdBuf, m_worldPosTexture.image,
                                VK_IMAGE_LAYOUT_UNDEFINED,
                                VK_IMAGE_LAYOUT_GENERAL);
  }
  m_allocator->finalizeAndReleaseStaging();
}

void GBuffer::destroy() {
  if (m_framebuffer != VK_NULL_HANDLE)
    vkDestroyFramebuffer(m_device, m_framebuffer, nullptr);
  m_allocator->destroy(m_albedoTexture);
  m_allocator->destroy(m_normalTexture);
  m_allocator->destroy(m_materialPropertiesTexture);
  m_allocator->destroy(m_worldPosTexture);
}