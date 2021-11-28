#include "Scene.hpp"

#include "model/Cube.hpp"
#include "model/VDBModel.hpp"

namespace volume_restir {

Scene::Scene(RenderContext* render_context, VDB* p_vdb)
    : render_context_(render_context) {
  VkCommandPoolCreateInfo transferPoolInfo{};
  transferPoolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
  transferPoolInfo.queueFamilyIndex =
      render_context_->Device()
          .get_queue_index(vkb::QueueType::graphics)
          .value();
  transferPoolInfo.flags = 0;

  VkCommandPool transferCommandPool;
  if (vkCreateCommandPool(render_context_->Device().device, &transferPoolInfo,
                          nullptr, &transferCommandPool) != VK_SUCCESS) {
    throw std::runtime_error("Failed to create command pool");
  }

  // std::unique_ptr<ModelBase> cube =
  //     std::make_unique<Cube>(render_context_, transferCommandPool);

  std::unique_ptr<ModelBase> vdb_model =
      std::make_unique<VDBModel>(render_context_, transferCommandPool, p_vdb);

  vkDestroyCommandPool(render_context_->Device().device, transferCommandPool,
                       nullptr);

  // AddObject(cube);
  AddObject(vdb_model);
}

}  // namespace volume_restir
