#pragma once

#include <vulkan/vulkan.h>

#include "VkBootstrap.h"

#include <vector>
#include "../src/RenderContext.hpp"

namespace volume_restir {
//class Device;
class SwapChain {
  //friend class Device;

public:
  vkb::Swapchain GetVkBSwapChain() const;
  VkFormat GetVkImageFormat() const;
  VkExtent2D GetVkExtent() const;
  uint32_t GetIndex() const;
  uint32_t GetCount() const;
  VkImage GetVkImage(uint32_t index) const;
  VkSemaphore GetImageAvailableVkSemaphore() const;
  VkSemaphore GetRenderFinishedVkSemaphore() const;

  void Recreate();
  bool Acquire();
  bool Present();
  ~SwapChain();

private:
  SwapChain(RenderContext* renderContext, VkSurfaceKHR vkSurface,
            unsigned int numBuffers);
  void Create();
  void Destroy();

  RenderContext* renderContext;
  VkSurfaceKHR vkSurface;
  unsigned int numBuffers;
  vkb::Swapchain swapchain_;
  std::vector<VkImage> vkSwapChainImages;
  VkFormat vkSwapChainImageFormat;
  VkExtent2D vkSwapChainExtent;
  uint32_t imageIndex = 0;

  VkSemaphore imageAvailableSemaphore;
  VkSemaphore renderFinishedSemaphore;
};
}  // namespace volume_restir