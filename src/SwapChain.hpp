#pragma once

#include <vulkan/vulkan.h>

#include <vector>

#include "RenderContext.hpp"
#include "VkBootstrap.h"

namespace volume_restir {
class SwapChain {
public:
  vkb::Swapchain GetVkBSwapChain() const;
  VkFormat GetVkImageFormat() const;
  VkSurfaceKHR GetVkSurface() const;
  VkExtent2D GetVkExtent() const;
  uint32_t GetIndex() const;
  uint32_t GetCount() const;
  VkImage GetVkImage(uint32_t index) const;

  SwapChain(RenderContext* renderContext);
  void Recreate();
  bool Acquire();
  bool Present();
  ~SwapChain();

  std::vector<VkSemaphore> available_semaphores_;
  std::vector<VkSemaphore> finished_semaphores_;
  std::vector<VkFence> fences_in_flight_;
  std::vector<VkFence> images_in_flight_;

private:
  void Create();
  void Destroy();

  RenderContext* renderContext;
  VkSurfaceKHR vkSurface;
  // unsigned int numBuffers;
  vkb::Swapchain swapchain_;
  std::vector<VkImage> vkSwapChainImages;
  VkFormat vkSwapChainImageFormat;
  VkExtent2D vkSwapChainExtent;
  uint32_t imageIndex = 0;
};
}  // namespace volume_restir