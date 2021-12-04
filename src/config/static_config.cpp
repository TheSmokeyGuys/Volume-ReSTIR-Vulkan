#include "static_config.hpp"

#include "config/build_config.h"

namespace volume_restir {
namespace static_config {

const std::string kApplicationName   = "Volume ReSTIR";
const int kWindowWidth               = 1920;
const int kWindowHeight              = 1080;
const int kMaxFrameInFlight          = 2;
const float kFOVInDegrees            = 45.0f;
const float kCameraMoveSpeed         = 0.05f;
const float kCameraRotateSensitivity = 0.1f;

// default seacrh paths
const std::vector<std::string> kDefaultSearchPaths = {PROJECT_DIRECTORY,
                                                      BUILD_DIRECTORY};

// restir light config
const bool kGenerateWhiteLight         = true;
const bool kIgnorePointLight           = true;
const uint32_t kNumPointLightGenerates = 100;

// kShaderMode = 0 for graphics
// kShaderMode = 1 for lambert
const int kShaderMode = 0;

}  // namespace static_config
}  // namespace volume_restir
