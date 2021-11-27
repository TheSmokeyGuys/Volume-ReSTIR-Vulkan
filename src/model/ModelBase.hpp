#ifndef __VOLUME_RESTIR_MODEL_BASE_HPP__
#define __VOLUME_RESTIR_MODEL_BASE_HPP__

/**
 * @file ModelBase.hpp
 * @author Zhihao Ruan (ruanzh@seas.upenn.edu)
 *
 * @brief ModelBase is like the Drawable class in CIS 560 classic OpenGL
 * rendering class, which serves as a base class to hole all allocated device
 * memory handlers for vertex shader inputs.
 *
 * @date 2021-11-27
 */

#include <vector>

#include "RenderContext.hpp"
#include "Vertex.hpp"

namespace volume_restir {

class ModelBase {
public:
  ModelBase() = delete;
  ModelBase(RenderContext* render_context, VkCommandPool command_pool);
  ModelBase(RenderContext* render_context, VkCommandPool command_pool,
            const std::vector<Vertex>& vertices,
            const std::vector<uint32_t>& indices);
  virtual ~ModelBase();

  virtual void SetTexture(VkImage texture);

  const std::vector<Vertex>& GetVertices() const noexcept { return vertices_; }
  VkBuffer GetVertexBuffer() const noexcept { return vertex_buffer_; }

  const std::vector<uint32_t> GetIndices() const noexcept { return indices_; }
  VkBuffer GetIndexBuffer() const noexcept { return index_buffer_; }

  const glm::mat4& GetModelMatrix() const noexcept { return model_matrix_; }
  VkBuffer GetModelBuffer() const noexcept { return model_buffer_; }

  VkImageView GetTextureView() const noexcept { return texture_view_; }
  VkSampler GetTextureSampler() const noexcept { return texture_sampler_; }

protected:
  RenderContext* render_context_;
  VkCommandPool command_pool_;

  std::vector<Vertex> vertices_;
  VkBuffer vertex_buffer_;
  VkDeviceMemory vertex_buffer_memory_;

  std::vector<uint32_t> indices_;
  VkBuffer index_buffer_;
  VkDeviceMemory index_buffer_memory_;

  glm::mat4 model_matrix_;
  VkBuffer model_buffer_;
  VkDeviceMemory model_buffer_memory_;

  VkImage texture_           = VK_NULL_HANDLE;
  VkImageView texture_view_  = VK_NULL_HANDLE;
  VkSampler texture_sampler_ = VK_NULL_HANDLE;

  virtual void CreateVertexBuffer(const std::vector<Vertex>& vertices);
  virtual void CreateIndexBuffer(const std::vector<uint32_t>& indices);
  virtual void CreateModelBuffer(const glm::mat4& model_matrix);
};

}  // namespace volume_restir

#endif /* __VOLUME_RESTIR_MODEL_BASE_HPP__ */
