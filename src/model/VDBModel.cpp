#include "VDBModel.hpp"
#include <cstdio>
#include <cstdlib>

namespace volume_restir {

VDBModel::VDBModel(RenderContext* render_context, VkCommandPool command_pool,
                   VDB* p_vdb)
    : ModelBase(render_context, command_pool), vdb_(p_vdb) {

  //std::vector<volume_restir::Vertex> v = vdb_->ToVertexArray(); 

  //std::cout << "V size: " << v.size()
  //          << "\n"; 

  //for (auto &i : v) {
  //  std::cout << "\tPos: < " << i.pos.x << ", " << i.pos.y << ", " <<
  //                   i.pos.z << ">\n"; 
  //}

  vertices_ = vdb_->ToVertexArray();

  //indices_ = { 0, 1, 2 };

  CreateVertexBuffer(vertices_);
  //CreateIndexBuffer(indices_);

  glm::mat4 model_matrix{1.0f};
  CreateModelBuffer(model_matrix);
}

}  // namespace volume_restir
