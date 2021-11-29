#include "ModelBase.hpp"

#include "utils/buffer.hpp"
#include "utils/logging.hpp"

namespace volume_restir {

ModelBase::ModelBase(RenderContext* render_context, VkCommandPool command_pool)
    : render_context_(render_context), command_pool_(command_pool) {}

ModelBase::ModelBase(RenderContext* render_context, VkCommandPool command_pool,
                     const std::vector<Vertex>& vertices,
                     const std::vector<uint32_t>& indices)
    : render_context_(render_context),
      command_pool_(command_pool),
      vertices_(vertices),
      indices_(indices) {
  CreateVertexBuffer(vertices_);
  CreateIndexBuffer(indices_);
  glm::mat4 model_matrix{1.0f};
  CreateModelBuffer(model_matrix);
}

void ModelBase::CreateVertexBuffer(std::vector<Vertex>& vertices) {
  if (!vertices.empty()) {
    buffer::CreateBufferFromData(
        render_context_, command_pool_, vertices.data(),
        vertices.size() * sizeof(Vertex), VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
        vertex_buffer_, vertex_buffer_memory_);
    spdlog::debug("Successfully created vertex buffer");
  }
  else
  {
    spdlog::debug("ModelBase::CreateVertexBuffer() failed, empty vertex array!"); 
  }
}

void ModelBase::CreateIndexBuffer(std::vector<uint32_t>& indices) {
  if (!indices.empty()) {
    buffer::CreateBufferFromData(render_context_, command_pool_, indices.data(),
                                 indices.size() * sizeof(uint32_t),
                                 VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
                                 index_buffer_, index_buffer_memory_);
    spdlog::debug("Successfully created index buffer");
  }
}

void ModelBase::CreateModelBuffer(glm::mat4& model_matrix) {
  buffer::CreateBufferFromData(
      render_context_, command_pool_, &model_matrix, sizeof(glm::mat4),
      VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, model_buffer_, model_buffer_memory_);
}

ModelBase::~ModelBase() {
  if (!indices_.empty()) {
    vkDestroyBuffer(render_context_->Device().device, index_buffer_, nullptr);
    vkFreeMemory(render_context_->Device().device, index_buffer_memory_,
                 nullptr);
  }

  if (!vertices_.empty()) {
    vkDestroyBuffer(render_context_->Device().device, vertex_buffer_, nullptr);
    vkFreeMemory(render_context_->Device().device, vertex_buffer_memory_,
                 nullptr);
  }

  vkDestroyBuffer(render_context_->Device().device, model_buffer_, nullptr);
  vkFreeMemory(render_context_->Device().device, model_buffer_memory_, nullptr);

  if (texture_view_ != VK_NULL_HANDLE) {
    vkDestroyImageView(render_context_->Device().device, texture_view_,
                       nullptr);
  }

  if (texture_sampler_ != VK_NULL_HANDLE) {
    vkDestroySampler(render_context_->Device().device, texture_sampler_,
                     nullptr);
  }
}

void ModelBase::SetTexture(VkImage texture) {
  texture_ = texture;

  // TODO: rewrite this into our code base classes
  //
  // this->texture     = texture;
  // this->textureView = Image::CreateView(
  //     device, texture, VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_ASPECT_COLOR_BIT);

  // // --- Specify all filters and transformations ---
  // VkSamplerCreateInfo samplerInfo = {};
  // samplerInfo.sType               = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;

  // // Interpolation of texels that are magnified or minified
  // samplerInfo.magFilter = VK_FILTER_LINEAR;
  // samplerInfo.minFilter = VK_FILTER_LINEAR;

  // // Addressing mode
  // samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
  // samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
  // samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;

  // // Anisotropic filtering
  // samplerInfo.anisotropyEnable = VK_TRUE;
  // samplerInfo.maxAnisotropy    = 16;

  // // Border color
  // samplerInfo.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;

  // // Choose coordinate system for addressing texels --> [0, 1) here
  // samplerInfo.unnormalizedCoordinates = VK_FALSE;

  // // Comparison function used for filtering operations
  // samplerInfo.compareEnable = VK_FALSE;
  // samplerInfo.compareOp     = VK_COMPARE_OP_ALWAYS;

  // // Mipmapping
  // samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
  // samplerInfo.mipLodBias = 0.0f;
  // samplerInfo.minLod     = 0.0f;
  // samplerInfo.maxLod     = 0.0f;

  // if (vkCreateSampler(device->GetVkDevice(), &samplerInfo, nullptr,
  //                     &textureSampler) != VK_SUCCESS) {
  //   throw std::runtime_error("Failed to create texture sampler");
  // }
}

}  // namespace volume_restir
