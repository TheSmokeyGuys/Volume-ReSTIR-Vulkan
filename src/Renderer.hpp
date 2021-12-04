#ifndef __VOLUME_RESTIR_RENDERER_HPP__
#define __VOLUME_RESTIR_RENDERER_HPP__

#include <vulkan/vulkan.hpp>

#ifndef NVVK_ALLOC_DEDICATED
#define NVVK_ALLOC_DEDICATED
#include "nvvk/allocator_vk.hpp"
#endif

#include "nvvk/context_vk.hpp"
#include "nvvk/debug_util_vk.hpp"
#include "nvvk/descriptorsets_vk.hpp"
#include "nvvk/profiler_vk.hpp"
#include "nvvk/raytraceKHR_vk.hpp"

// #VKRay
#include <array>
#include <memory>
#include <vector>

#include "Camera.hpp"
#include "GBuffer.hpp"
#include "GLTFSceneBuffers.h"
#include "RenderContext.hpp"
#include "Scene.hpp"
#include "config/build_config.h"
#include "config/static_config.hpp"
#include "loader/GLTFLoader.hpp"
#include "nvh/gltfscene.hpp"
#include "nvvk/pipeline_vk.hpp"
#include "nvvk/raytraceKHR_vk.hpp"
#include "nvvk/swapchain_vk.hpp"
#include "passes/restirPass.h"
#include "passes/spatialReusePass.h"
#include "shaders/Headers/binding.glsl"
#include "utils/vkqueue_utils.hpp"

namespace volume_restir {

class Renderer : public nvvk::AppBase {
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
  void CreateDescriptorSetScene();
  void CreateScene(std::string scene);

  // nvvk functions
  void _createUniformBuffer();
  void _createDescriptorSet();
  void _updateUniformBuffer(const vk::CommandBuffer& cmdBuf);
  void _createPostPipeline();
  void _updateRestirDescriptorSet();

  // Testing Functions
  void render();  // Function just to test the pipeline
  void destroyResources();
  void _drawPost(vk::CommandBuffer cmdBuf, uint32_t currentGFrame);
  void _submitMainCommand();
  void onResize(int /*w*/, int /*h*/) override;
  void _updateFrame();
  void _resetFrame();

  std::unique_ptr<RenderContext> render_context_;
  std::unique_ptr<nvvk::SwapChain> swapchain_;
  std::unique_ptr<Camera> camera_;
  std::unique_ptr<Scene> scene_;

  // image views & frame buffers
  /*std::vector<VkImageView> swapchain_image_views_;*/
  std::vector<vk::Framebuffer> framebuffers_;

  // descriptor set layouts
  VkDescriptorSetLayout camera_descriptorset_layout_;
  VkDescriptorSetLayout scene_descriptorset_layout_;

  // descriptor sets
  VkDescriptorSet camera_descriptorset_;
  VkDescriptorSet scene_descriptorset_;

  // descriptor pools
  vk::DescriptorPool descriptor_pool_;

  // pipeline layouts
  vk::PipelineLayout graphics_pipeline_layout_;

  // pipelines
  vk::Pipeline graphics_pipeline_;

  // render pass
  vk::RenderPass render_pass_;

  // command pools
  vk::CommandPool graphics_command_pool_;

  // command buffer
  std::vector<vk::CommandBuffer> command_buffers_;

  size_t current_frame_idx_  = 0;
  bool frame_buffer_resized_ = false;

  // NVVK Stuff
  nvvk::AllocatorDedicated m_alloc;

  nvvk::DebugUtil m_debug;

  GLTFLoader m_gltfLoad;

  nvvk::RaytracingBuilderKHR m_rtBuilder;

  bool m_enableTemporalReuse = true;
  bool m_enableSpatialReuse  = true;
  bool m_enableVisibleTest   = true;
  bool m_enableEnvironment   = false;

  int m_log2InitialLightSamples            = 5;
  int m_temporalReuseSampleMultiplier      = 20;
  constexpr static std::size_t numGBuffers = 2;

  GBuffer m_gBuffers[numGBuffers];
  vk::Extent2D m_windowSize{0, 0};

  uint32_t m_currentGBufferFrame = 0;
  shader::PushConstant m_pushC;
  shader::SceneUniforms m_sceneUniforms;
  nvvk::Buffer m_sceneUniformBuffer;
  std::vector<nvvk::Texture> m_textures;
  std::vector<nvvk::Texture> m_reservoirInfoBuffers;
  std::vector<nvvk::Texture> m_reservoirWeightBuffers;

  nvvk::Texture m_reservoirTmpInfoBuffer;
  nvvk::Texture m_reservoirTmpWeightBuffer;
  nvvk::Texture m_storageImage;

  nvvk::DescriptorSetBindings m_sceneSetLayoutBind;
  vk::DescriptorSetLayout m_sceneSetLayout;
  vk::DescriptorSet m_sceneSet;

  nvvk::DescriptorSetBindings m_lightSetLayoutBind;
  vk::DescriptorSetLayout m_lightSetLayout;
  vk::DescriptorSet m_lightSet;

  nvvk::DescriptorSetBindings m_restirSetLayoutBind;
  vk::DescriptorSetLayout m_restirSetLayout;
  std::vector<vk::DescriptorSet> m_restirSets;

  GLTFSceneBuffers m_sceneBuffers;

  RestirPass m_restirPass;
  SpatialReusePass m_spatialReusePass;

  vk::Pipeline m_postPipeline;
  vk::PipelineLayout m_postPipelineLayout;

  vk::Fence m_mainFence;
};

}  // namespace volume_restir

#endif /* __VOLUME_RESTIR_RENDERER_HPP__ */
