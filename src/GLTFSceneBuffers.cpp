#include "GLTFSceneBuffers.h"

#include <cmath>

#include "fileformats/stb_image.h"
#include "nvh/fileoperations.hpp"
#include "shaders/headers/common.glsl"

#define STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_WRITE_IMPLEMENTATION
extern std::string environmentalTextureFile;
extern std::vector<std::string> defaultSearchPaths;

namespace volume_restir {

void GLTFSceneBuffers::_loadEnvironment() {
  std::string fileName =
      nvh::findFile(environmentalTextureFile, defaultSearchPaths);
  std::cout << fileName << std::endl;
  m_alloc->destroy(m_environmentalTexture);
  int width, height, component;

  float* pixels =
      stbi_loadf(fileName.c_str(), &width, &height, &component, STBI_rgb_alpha);
  vk::DeviceSize bufferSize = width * height * 4 * sizeof(float);
  vk::Extent2D imageSize(width, height);
  vk::SamplerCreateInfo samplerCreateInfo{{},
                                          vk::Filter::eLinear,
                                          vk::Filter::eLinear,
                                          vk::SamplerMipmapMode::eLinear};
  vk::Format format          = vk::Format::eR32G32B32A32Sfloat;
  vk::ImageCreateInfo icInfo = nvvk::makeImage2DCreateInfo(imageSize, format);
  {
    nvvk::ScopeCommandBuffer cmdBuf(m_device, m_graphicsQueueIndex);
    nvvk::Image image =
        m_alloc->createImage(cmdBuf, bufferSize, pixels, icInfo);
    vk::ImageViewCreateInfo ivInfo =
        nvvk::makeImageViewCreateInfo(image.image, icInfo);
    m_environmentalTexture =
        m_alloc->createTexture(image, ivInfo, samplerCreateInfo);
  }
  m_alloc->finalizeAndReleaseStaging();

  const uint32_t rx = width;
  const uint32_t ry = height;
  std::vector<float> environmentalPdf(rx * ry);

  float pre_cos_theta    = 1.0f;
  const float step_phi   = float(2.0 * M_PI) / float(rx);
  const float step_theta = float(M_PI) / float(ry);
  double total           = 0;
  for (uint32_t y = 0; y < ry; ++y) {
    const float theta1    = float(y + 1) * step_theta;
    const float cos_theta = std::cos(theta1);
    const float area      = (pre_cos_theta - cos_theta) * step_phi;
    pre_cos_theta         = cos_theta;

    for (uint32_t x = 0; x < rx; ++x) {
      const uint32_t idx  = y * rx + x;
      const uint32_t idx4 = idx * 4;
      environmentalPdf[idx] =
          std::sin(theta1) *
          shader::luminance(pixels[idx4], pixels[idx4 + 1], pixels[idx4 + 2]);
      total += environmentalPdf[idx];
      pixels[idx4 + 3] =
          shader::luminance(pixels[idx4], pixels[idx4 + 1], pixels[idx4 + 2]);
    }
  }

  std::vector<shader::aliasTableCell> aliasTable =
      createAliasTable(environmentalPdf);

  {
    nvvk::ScopeCommandBuffer cmdBuf(m_device, m_graphicsQueueIndex);
    vk::SamplerCreateInfo samplerCreateInfo{};
    vk::Format format          = vk::Format::eR32G32B32A32Sfloat;
    vk::ImageCreateInfo icInfo = nvvk::makeImage2DCreateInfo({rx, ry}, format);
    vk::DeviceSize bufferSize  = rx * ry * sizeof(shader::aliasTableCell);

    nvvk::Image image =
        m_alloc->createImage(cmdBuf, bufferSize, aliasTable.data(), icInfo);
    vk::ImageViewCreateInfo ivInfo =
        nvvk::makeImageViewCreateInfo(image.image, icInfo);
    m_environmentAliasMap =
        m_alloc->createTexture(image, ivInfo, samplerCreateInfo);
  }
  m_alloc->finalizeAndReleaseStaging();
  std::cout << "etotal: " << total << std::endl;
  stbi_image_free(pixels);
}

}  // namespace volume_restir
