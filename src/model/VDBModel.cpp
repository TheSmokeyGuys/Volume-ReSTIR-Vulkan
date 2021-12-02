#include "VDBModel.hpp"

#include <cstdio>
#include <cstdlib>

namespace volume_restir {

VDBModel::VDBModel(RenderContext* render_context, VkCommandPool command_pool,
                   VDB* p_vdb)
    : ModelBase(render_context, command_pool), vdb_(p_vdb) {
  vertices_ = vdb_->ToVertexArray();

  CreateVertexBuffer(vertices_);

  glm::mat4 model_matrix{1.0f};
  CreateModelBuffer(model_matrix);
}

}  // namespace volume_restir
