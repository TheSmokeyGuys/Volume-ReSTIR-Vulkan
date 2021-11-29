#include "static_config.hpp"

namespace volume_restir {
namespace static_config {

const std::string kApplicationName   = "Volume ReSTIR";
const int kWindowWidth               = 1920;
const int kWindowHeight              = 1080;
const int kMaxFrameInFlight          = 2;
const float kFOVInDegrees            = 45.0f;
const float kCameraMoveSpeed         = 0.05f;
const float kCameraRotateSensitivity = 0.1f;

}  // namespace static_config
}  // namespace volume_restir
