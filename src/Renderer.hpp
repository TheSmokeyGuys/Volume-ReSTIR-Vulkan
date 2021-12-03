#ifndef __VOLUME_RESTIR_RENDERER_HPP__
#define __VOLUME_RESTIR_RENDERER_HPP__

#include <array>
#include <memory>
#include <vector>
#include <vulkan/vulkan.hpp>

#include "Camera.hpp"
#include "RenderContext.hpp"
#include "Scene.hpp"
#include "config/build_config.h"
#include "config/static_config.hpp"
#include "utils/vkqueue_utils.hpp"

#include "nvvk/swapchain_vk.hpp"

namespace volume_restir {

class Renderer {
public:
  Renderer();
  ~Renderer();

  RenderContext* RenderContextPtr() const noexcept;
  void SetFrameBufferResized(bool val);
  void Draw();

private:
  void CreateRenderPass();
  void CreateGraphicsPipeline();
  void CreateFrameResources();
  void CreateCommandPools();
  void RecordCommandBuffers();

  void CreateDescriptorPool();
  void CreateCameraDiscriptorSetLayout();
  void CreateCameraDescriptorSet();

  void CreateSwapChain();
  void RecreateSwapChain();
  void CleanupSwapChain();

  std::unique_ptr<RenderContext> render_context_;
  std::unique_ptr<nvvk::SwapChain> swapchain_;
  std::unique_ptr<Camera> camera_;
  std::unique_ptr<Scene> scene_;

  // image views & frame buffers
  /*std::vector<VkImageView> swapchain_image_views_;*/
  std::vector<VkFramebuffer> framebuffers_;

  // descriptor set layouts
  VkDescriptorSetLayout camera_descriptorset_layout_;

  // descriptor sets
  VkDescriptorSet camera_descriptorset_;

  // descriptor pools
  VkDescriptorPool descriptor_pool_;

  // pipeline layouts
  VkPipelineLayout graphics_pipeline_layout_;

  // pipelines
  VkPipeline graphics_pipeline_;

  // render pass
  VkRenderPass render_pass_;

  // command pools
  VkCommandPool graphics_command_pool_;

  // command buffer
  std::vector<VkCommandBuffer> command_buffers_;

  size_t current_frame_idx_  = 0;
  bool frame_buffer_resized_ = false;
};

}  // namespace volume_restir

#endif /* __VOLUME_RESTIR_RENDERER_HPP__ */
