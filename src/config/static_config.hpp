#ifndef __VOLUME_RESTIR_CONFIG_STATIC_CONFIG_HPP__
#define __VOLUME_RESTIR_CONFIG_STATIC_CONFIG_HPP__

#include <string>
#include <vector>

namespace volume_restir {
namespace static_config {

extern const std::string kApplicationName;
extern const int kWindowWidth;
extern const int kWindowHeight;
extern const int kMaxFrameInFlight;
extern const float kFOVInDegrees;
extern const float kCameraMoveSpeed;
extern const float kCameraRotateSensitivity;
extern const int kShaderMode;
extern const std::vector<std::string> kDefaultSearchPaths;
extern const bool kGenerateWhiteLight;
extern const bool kIgnorePointLight;
extern const uint32_t kNumPointLightGenerates;

}  // namespace static_config
}  // namespace volume_restir

#endif /* __VOLUME_RESTIR_CONFIG_STATIC_CONFIG_HPP__ */
