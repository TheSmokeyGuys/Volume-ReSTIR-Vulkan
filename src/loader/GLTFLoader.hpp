#pragma once
#include <string>
#include <vulkan/vulkan.hpp>
#include "nvh/gltfscene.hpp"
namespace volume_restir {
class GLTFLoader
{
  void LoadScene(const std::string &fileName);

  private:
  tinygltf::Model m_tmodel;
};

}  // namespace volume_restir
