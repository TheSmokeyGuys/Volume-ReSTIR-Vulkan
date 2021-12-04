#include <cstdio>
#include <cstdlib>
#include <filesystem>

#include "Camera.hpp"
#include "Renderer.hpp"
#include "SingtonManager.hpp"
#include "config/build_config.h"
#include "config/static_config.hpp"
#include "spdlog/spdlog.h"
#include "vdb/Utilities.h"
#include "vdb/vdb.h"

VULKAN_HPP_DEFAULT_DISPATCH_LOADER_DYNAMIC_STORAGE

namespace fs = std::filesystem;
using namespace volume_restir;

const std::string vdb_filename = "fire.vdb";

static std::string cornellBox = "media/cornellBox/cornellBox.gltf";
static std::string sponza     = "media/Sponza/glTF/Sponza.gltf";

// Since these files are too heavy, please download them yourself
// And make .gltf file by using Blender, etc.
// https://developer.nvidia.com/ue4-sun-temple
static std::string tample = "media/sun_tample/tample.gltf";
// https://www.dropbox.com/s/ka378kmu62i669m/Bistro_Godot.glb?dl=0
static std::string bistro = "media/bistro/bistro.gltf";

static std::string loadScene = sponza;

const fs::path asset_dir = fs::path(PROJECT_DIRECTORY) / fs::path("assets");
const std::string file   = (asset_dir / vdb_filename).string();

int main() {
#ifdef NDEBUG
  spdlog::set_level(spdlog::level::info);
#else
  spdlog::set_level(spdlog::level::debug);
#endif
  spdlog::info("Hello from Volumetric-ReSTIR project!");

  //SingletonManager::GetVDBLoader().Load(file);

  Renderer renderer;

  while (!SingletonManager::GetWindow().ShouldQuit()) {
    glfwPollEvents();
    renderer.Draw();
  }

#ifdef _WIN32
  system("pause");
#endif

  return 0;
}
