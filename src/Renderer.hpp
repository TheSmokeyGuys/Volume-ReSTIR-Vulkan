#ifndef __VOLUME_RESTIR_RENDERER_HPP__
#define __VOLUME_RESTIR_RENDERER_HPP__

#include <vulkan/vulkan.h>

#include <array>
#include <memory>
#include <vector>

#include "RenderContext.hpp"
#include "ShaderModule.hpp"
#include "SwapChain.hpp"
#include "VkBootstrap.h"
#include "Window.hpp"
#include "config/build_config.h"
#include "config/static_config.hpp"
#include "utils/vkqueue_utils.hpp"

namespace volume_restir {

class Renderer {
public:
  Renderer();
  ~Renderer();

  void SetFrameBufferResized(bool val); 

  void Draw();
  RenderContext& getRenderContext() const { return *render_context_; }

private:
  using Queues = std::array<VkQueue, sizeof(vkb::QueueType)>;

  void InitQueues();
  void CreateRenderPass();
  void CreateGraphicsPipeline();
  void CreateFrameResources();
  void CreateCommandPools();
  void RecordCommandBuffers();

  void CreateVertexBuffer(); 
  uint32_t FineMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties); 

  void CreateSwapChain();
  void RecreateSwapChain();
  void CleanupSwapChain(); 

  void Cleanup(); 

  std::unique_ptr<RenderContext> render_context_;
  std::unique_ptr<SwapChain> swapchain_;

  Queues queues_;

  std::vector<VkImageView> swapchain_image_views_;
  std::vector<VkFramebuffer> framebuffers_;

  VkPipelineLayout graphics_pipeline_layout_;
  VkPipeline graphics_pipeline_;

  VkRenderPass render_pass_;

  VkCommandPool graphics_command_pool_;
  std::vector<VkCommandBuffer> command_buffers_;

  size_t current_frame_idx_ = 0;

  bool frame_buffer_resized_ = false;

  std::unique_ptr<VertexManager> vertex_manager_; 
  VkBuffer vertex_buffer_;
  VkMemoryRequirements mem_requirements_;
  VkDeviceMemory vertex_buffer_memory_;

  void* data_;


};

}  // namespace volume_restir

#endif /* __VOLUME_RESTIR_RENDERER_HPP__ */
