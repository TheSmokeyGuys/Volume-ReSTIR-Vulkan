#ifndef __VOLUME_RESTIR_MODEL_VERTEX_HPP__
#define __VOLUME_RESTIR_MODEL_VERTEX_HPP__

/**
 * @file Vertex.hpp
 * @author Zhihao Ruan (ruanzh@seas.upenn.edu)
 *
 * @brief A general `Vertex` class to hold all per-vertex attributes for vertex
 * shader inputs.
 *
 * @date 2021-11-27
 */

#include <nvmath/nvmath.h>
#include <vulkan/vulkan.h>

#include <array>

namespace volume_restir {

struct Vertex {
  nvmath::vec3f pos;
  nvmath::vec3f color;
  nvmath::vec3f normal;
  nvmath::vec2f tex_coord;

  // Get the binding description, which describes the rate to load data from
  // memory
  static VkVertexInputBindingDescription getBindingDescription() {
    VkVertexInputBindingDescription bindingDescription = {};
    bindingDescription.binding                         = 0;
    bindingDescription.stride                          = sizeof(Vertex);
    bindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

    return bindingDescription;
  }

  // Get the attribute descriptions, which describe how to handle vertex input
  static std::array<VkVertexInputAttributeDescription, 4>
  getAttributeDescriptions() {
    std::array<VkVertexInputAttributeDescription, 4> attributeDescriptions = {};

    // Position
    attributeDescriptions[0].binding  = 0;
    attributeDescriptions[0].location = 0;
    attributeDescriptions[0].format   = VK_FORMAT_R32G32B32_SFLOAT;
    attributeDescriptions[0].offset   = offsetof(Vertex, pos);

    // Color
    attributeDescriptions[1].binding  = 0;
    attributeDescriptions[1].location = 1;
    attributeDescriptions[1].format   = VK_FORMAT_R32G32B32_SFLOAT;
    attributeDescriptions[1].offset   = offsetof(Vertex, color);

    // Normal
    attributeDescriptions[2].binding  = 0;
    attributeDescriptions[2].location = 2;
    attributeDescriptions[2].format   = VK_FORMAT_R32G32B32_SFLOAT;
    attributeDescriptions[2].offset   = offsetof(Vertex, normal);

    // Texture coordinate
    attributeDescriptions[3].binding  = 0;
    attributeDescriptions[3].location = 3;
    attributeDescriptions[3].format   = VK_FORMAT_R32G32_SFLOAT;
    attributeDescriptions[3].offset   = offsetof(Vertex, tex_coord);

    return attributeDescriptions;
  }
};

}  // namespace volume_restir

#endif /* __VOLUME_RESTIR_MODEL_VERTEX_HPP__ */
