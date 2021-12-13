#ifndef __VOLUME_RESTIR_CONFIG_STATIC_CONFIG_HPP__
#define __VOLUME_RESTIR_CONFIG_STATIC_CONFIG_HPP__

#include <string>
#include <vector>

#define VOLUME_RESTIR_USE_VDB
#define USE_GLTF
// #define USE_RT_PIPELINE
#define USE_RESTIR_PIPELINE

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

constexpr size_t kNumGBuffers = 2;

}  // namespace static_config

#endif /* __VOLUME_RESTIR_CONFIG_STATIC_CONFIG_HPP__ */

// descSetLayout should also contain uniforms

// std::vector<VkDescriptorSetLayout> rtDescSetLayouts{
//   descSetLayout, uniformDescSetLayout, lightDescSetLayout,
//   restirDescSetLayout};

// Spatial Reuse: std::vector<VkDescriptorSetLayout> setlayouts{descSetLayout,
// lightDescSetLayout, restirDescSetLayout};

// Restir Post Process: No descriptor sets

// m_descSet:       0x612f93000000004e
// m_postDescSet:   0xc079b30000000088
// m_rtDescSet:     0x051820000000007b
// m_lightDescSet:  0xec25c90000000093
// m_restirDescSet: {0x0f799000000000a9, 0xd0e29300000000aa}
// m_restirPoseSet:  0xc9ccfc00000000ad
// m_restirUniform:   0xe150c50000000097
