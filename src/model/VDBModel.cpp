#include "VDBModel.hpp"

namespace volume_restir {

VDBModel::VDBModel(RenderContext* render_context, VkCommandPool command_pool,
                   VDB* p_vdb)
    : ModelBase(render_context, command_pool), vdb_(p_vdb) {
  CreateVertexBuffer(vdb_->ToVertexArray());
  glm::mat4 model_matrix{1.0f};
  CreateModelBuffer(model_matrix);
}

}  // namespace volume_restir
