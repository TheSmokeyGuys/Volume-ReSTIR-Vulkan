#pragma once
#include <string>
#include <vulkan/vulkan.hpp>

#include "nvh/gltfscene.hpp"

#define TINYGLTF_IMPLEMENTATION
#define STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_WRITE_IMPLEMENTATION
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
