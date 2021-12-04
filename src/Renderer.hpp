#ifndef __VOLUME_RESTIR_RENDERER_HPP__
#define __VOLUME_RESTIR_RENDERER_HPP__

#define NVVK_ALLOC_DEDICATED
#include "nvvk/allocator_vk.hpp"
#include "nvvk/debug_util_vk.hpp"
#include "nvvk/descriptorsets_vk.hpp"
#include "nvvk/context_vk.hpp"
#include "nvvk/profiler_vk.hpp"
#include "nvvk/raytraceKHR_vk.hpp"
// #VKRay
#include "nvh/gltfscene.hpp"
#include "nvvk/raytraceKHR_vk.hpp"

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
#include "loader/GLTFLoader.hpp"
#include "shaders/Headers/binding.glsl"


#include "GLTFSceneBuffers.h"
#define VULKAN_HPP
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
  void CreateDescriptorSetScene();
  void CreateScene(std::string scene);


  std::unique_ptr<RenderContext> render_context_;
  std::unique_ptr<nvvk::SwapChain> swapchain_;
  std::unique_ptr<Camera> camera_;
  std::unique_ptr<Scene> scene_;

  // image views & frame buffers
  /*std::vector<VkImageView> swapchain_image_views_;*/
  std::vector<VkFramebuffer> framebuffers_;

  // descriptor set layouts
  vk::DescriptorSetLayout camera_descriptorset_layout_;
  vk::DescriptorSetLayout scene_descriptorset_layout_;

  // descriptor sets
  vk::DescriptorSet camera_descriptorset_;
  vk::DescriptorSet scene_descriptorset_;

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
  std::vector<VkCommandBuffer> command_buffers_;

  size_t current_frame_idx_  = 0;
  bool frame_buffer_resized_ = false;


  //NVVK Stuff
  nvvk::AllocatorDedicated m_alloc;
  nvvk::DebugUtil m_debug;
  GLTFLoader m_gltfLoad;
  std::vector<nvvk::Texture> m_textures;
  nvvk::RaytracingBuilderKHR m_rtBuilder;

  GLTFScene m_sceneBuffers;

  ////GLTF Scene Stuff
  //nvvk::Buffer m_primlooks;
  //nvvk::Buffer m_vertices;
  //nvvk::Buffer m_normals;
  //nvvk::Buffer m_texcoords;
  //nvvk::Buffer m_indices;
  //nvvk::Buffer m_materials;
  //nvvk::Buffer m_matrices;
  //std::vector<nvvk::Texture> m_textures;
  //nvvk::Buffer m_tangents;
  //nvvk::Buffer m_colors;
};

}  // namespace volume_restir

#endif /* __VOLUME_RESTIR_RENDERER_HPP__ */
