#include "ModelBase.hpp"

#include "utils/buffer.hpp"

namespace volume_restir {

ModelBase::ModelBase(RenderContext* render_context)
    : render_context_(render_context) {}

ModelBase::ModelBase(RenderContext* render_context, VkCommandPool command_pool,
                     const std::vector<Vertex>& vertices,
                     const std::vector<uint32_t>& indices)
    : render_context_(render_context), vertices_(vertices), indices_(indices) {
  if (!vertices_.empty()) {
    buffer::CreateBufferFromData(
        render_context_, command_pool, this->vertices_.data(),
        vertices_.size() * sizeof(Vertex), VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
        vertex_buffer_, vertex_buffer_memory_);
  }

  if (!indices_.empty()) {
    buffer::CreateBufferFromData(
        render_context_, command_pool, this->indices_.data(),
        indices_.size() * sizeof(uint32_t), VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
        index_buffer_, index_buffer_memory_);
  }

  model_matrix_ = glm::mat4(1.0f);
  buffer::CreateBufferFromData(
      render_context_, command_pool, &model_matrix_, sizeof(glm::mat4),
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
