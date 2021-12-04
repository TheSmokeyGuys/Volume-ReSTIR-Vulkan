#define TINYGLTF_IMPLEMENTATION
#define STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_WRITE_IMPLEMENTATION

#include "GLTFLoader.hpp"

#include "nvh/gltfscene.hpp"
#include "utils/logging.hpp"

namespace volume_restir {

void GLTFLoader::LoadScene(const std::string& filename) {
  using vkBU = vk::BufferUsageFlagBits;
  tinygltf::TinyGLTF tcontext;
  std::string warn, error;

  spdlog::info("Loading file: {}", filename.c_str());
  if (!tcontext.LoadASCIIFromFile(&m_tmodel, &error, &warn, filename)) {
    assert(!"Error while loading scene");
  }
  spdlog::info(warn.c_str());
  spdlog::info(error.c_str());

  m_gltfScene.importMaterials(m_tmodel);
  m_gltfScene.importDrawableNodes(
      m_tmodel, nvh::GltfAttributes::Normal | nvh::GltfAttributes::Texcoord_0 |
                    nvh::GltfAttributes::Color_0 |
                    nvh::GltfAttributes::Tangent);

  // No clue what this is
  // ImGuiH::SetCameraJsonFile(fs::path(filename).stem().string());
  // if (!m_gltfScene.m_cameras.empty()) {
  //  auto& c = m_gltfScene.m_cameras[0];
  //  CameraManip.setCamera(
  //      {c.eye, c.center, c.up, (float)rad2deg(c.cam.perspective.yfov)});
  //  ImGuiH::SetHomeCamera(
  //      {c.eye, c.center, c.up, (float)rad2deg(c.cam.perspective.yfov)});

  //  for (auto& c : m_gltfScene.m_cameras) {
  //    ImGuiH::AddCamera(
  //        {c.eye, c.center, c.up, (float)rad2deg(c.cam.perspective.yfov)});
  //  }
  //} else {
  //  // Re-adjusting camera to fit the new scene
  //  CameraManip.fit(m_gltfScene.m_dimensions.min,
  //  m_gltfScene.m_dimensions.max,
  //                  true);
  //}
}
}  // namespace volume_restir