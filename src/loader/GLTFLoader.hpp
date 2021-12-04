#pragma once
#include <string>
#include <vulkan/vulkan.hpp>

#include "nvh/gltfscene.hpp"
namespace volume_restir {
class GLTFLoader {
public:
  void LoadScene(const std::string &fileName);

  nvh::GltfScene m_gltfScene;
  tinygltf::Model m_tmodel;

private:
  // Resources
};

}  // namespace volume_restir
