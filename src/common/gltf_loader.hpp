#pragma once

#include "nvh/gltfscene.hpp"

class GLTFLoader {
public:
  GLTFLoader() : m_isSceneLoaded(false) {}

  void loadScene(const std::string filename);

  const tinygltf::Model& getTModel() const { return m_tinyGLTFModel; }
  const nvh::GltfScene& getGLTFScene() const { return m_gltfScene; }
  bool isSceneLoaded() const { return m_isSceneLoaded; }

private:
  nvh::GltfScene m_gltfScene;
  tinygltf::Model m_tinyGLTFModel;
  bool m_isSceneLoaded;
};