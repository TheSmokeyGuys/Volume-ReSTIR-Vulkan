#define TINYGLTF_IMPLEMENTATION
#define STB_IMAGE_WRITE_IMPLEMENTATION

#include "gltf_loader.hpp"

#include <filesystem>

#include "imgui.h"
#include "imgui/imgui_camera_widget.h"
#include "nvh/cameramanipulator.hpp"
#include "utils/logging.hpp"
namespace fs = std::filesystem;

void GLTFLoader::loadScene(const std::string filename) {
  tinygltf::TinyGLTF gltfContext;
  std::string warn, error;

  spdlog::info("Loading GLTF from file: {}", filename);
  if (!gltfContext.LoadASCIIFromFile(&m_tinyGLTFModel, &error, &warn,
                                     filename)) {
    spdlog::error("Error when loading GLTF: {}", error);
  }
  if (!warn.empty()) {
    spdlog::warn("Warning when loading GLTF: {}", warn);
  }

  m_gltfScene.importMaterials(m_tinyGLTFModel);
  m_gltfScene.importDrawableNodes(
      m_tinyGLTFModel,
      nvh::GltfAttributes::Normal | nvh::GltfAttributes::Texcoord_0 |
          nvh::GltfAttributes::Color_0 | nvh::GltfAttributes::Tangent);

  ImGuiH::SetCameraJsonFile(fs::path(filename).stem().string());
  if (!m_gltfScene.m_cameras.empty()) {
    auto& c = m_gltfScene.m_cameras[0];
    CameraManip.setCamera(
        {c.eye, c.center, c.up, (float)rad2deg(c.cam.perspective.yfov)});
    ImGuiH::SetHomeCamera(
        {c.eye, c.center, c.up, (float)rad2deg(c.cam.perspective.yfov)});

    for (auto& c : m_gltfScene.m_cameras) {
      ImGuiH::AddCamera(
          {c.eye, c.center, c.up, (float)rad2deg(c.cam.perspective.yfov)});
    }
  } else {
    // Re-adjusting camera to fit the new scene
    CameraManip.fit(m_gltfScene.m_dimensions.min, m_gltfScene.m_dimensions.max,
                    true);
  }

  m_isSceneLoaded = true;
}
