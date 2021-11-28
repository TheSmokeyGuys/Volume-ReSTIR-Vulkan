#ifndef __VOLUME_RESTIR_CUBE_HPP__
#define __VOLUME_RESTIR_CUBE_HPP__

/**
 * @file Cube.hpp
 * @author Zhihao Ruan (ruanzh@seas.upenn.edu)
 *
 * @brief `Cube` inherits from `ModelBase` as a basic rendering shape with
 * default length.
 *
 * @date 2021-11-27
 */

#include "ModelBase.hpp"
#include "config/static_config.hpp"

namespace volume_restir {

class Cube : public ModelBase {
public:
  Cube(RenderContext* render_context, VkCommandPool command_pool)
      : ModelBase(render_context, command_pool) {}

private:
  // TODO: write a function to create cube vertices & indices
  //  could hard code default cube length in config/static_config.cpp
  //  Call `ModelBase::CreateVertexBuffer() / CreateIndexBuffer()` to create
  //  buffer once have the vertices
  void DefaultInit();
};

}  // namespace volume_restir

#endif /* __VOLUME_RESTIR_CUBE_HPP__ */
