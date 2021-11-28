#include "Camera.hpp"

#include "utils/buffer.hpp"
#include "utils/logging.hpp"

#define GLM_FORCE_RADIANS
// Use Vulkan depth range of 0.0 to 1.0 instead of OpenGL
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <cmath>
#include <glm/gtc/matrix_transform.hpp>

#ifndef M_PI_2
#define M_PI_2 1.5707963267948966192313216916398
#endif

#ifndef EPSILON
#define EPSILON 0.001f
#endif

namespace volume_restir {

namespace {
/**
 * @brief Get the Yaw Pitch Euler Angles
 *  referenced at
 * https://gamedev.stackexchange.com/questions/50963/how-to-extract-euler-angles-from-transformation-matrix
 *
 * @param transformation: transformation matrix
 * @param yaw:            output yaw
 * @param pitch:          output pitch
 */
inline void GetYawPitch(const glm::mat4& transformation, float& yaw,
                        float& pitch) {
  // if (transformation[0][0] == 1.0f) {
  //   yaw   = atan2f(transformation[0][2], transformation[2][3]);
  //   pitch = 0;
  // } else if (transformation[0][0] == -1.0f) {
  //   yaw   = atan2f(transformation[0][2], transformation[2][3]);
  //   pitch = 0;
  // } else {
  //   yaw   = atan2(-transformation[2][0], transformation[0][0]);
  //   pitch = asin(transformation[1][0]);
  // }

  // yaw = -asin(transformation[2][0]);
  // if (yaw > -M_PI_2 + EPSILON) {
  //   if (yaw < M_PI_2 - EPSILON) {
  //     pitch = atan2(transformation[1][0], transformation[0][0]);
  //   } else {
  //     pitch = atan2(-transformation[0][1], transformation[0][2]);
  //   }
  // } else {
  //   pitch = atan2(transformation[0][1], transformation[0][2]);
  // }

  pitch = asin(transformation[1][0]);
  if (pitch > -M_PI_2 + EPSILON) {
    if (pitch < M_PI_2 - EPSILON) {
      yaw = atan2(-transformation[2][0], transformation[0][0]);
    } else {
      // WARNING.  Not a unique solution.
      yaw = atan2(transformation[2][1], transformation[2][2]);
    }
  } else {
    // WARNING.  Not a unique solution.
    yaw = -atan2(transformation[2][1], transformation[2][2]);
  }
}
}  // namespace

Camera::Camera(RenderContext* render_context, float fov, float aspect_ratio)
    : metadata_({render_context, fov, aspect_ratio}),
      pos_(0.f, 0.0f, -1.0f),
      yaw_(0.f),
      pitch_(0.f),
      has_device_memory_(false),
      buffer_mapped_data_(nullptr) {
  // Initially Align with the pos of camera
  glm::vec3 ref_pt{0.f, 0.f, 0.f};
  view_  = glm::normalize(ref_pt - pos_);
  right_ = glm::cross(view_, UP);
  up_    = glm::cross(right_, view_);

  buffer_object_.view_matrix         = glm::lookAt(pos_, ref_pt, up_);
  buffer_object_.view_matrix_inverse = glm::inverse(buffer_object_.view_matrix);
  buffer_object_.projection_matrix =
      glm::perspective(glm::radians(45.f), aspect_ratio, 0.1f, 100.f);
  buffer_object_.projection_matrix[1][1] *= -1;  // y-coordinate is flipped

  GetYawPitch(buffer_object_.view_matrix_inverse, yaw_, pitch_);
  yaw_   = glm::degrees(yaw_);
  pitch_ = glm::degrees(pitch_);

  spdlog::info("Created camera at position {}", pos_);
  spdlog::info("Camera is looking in direction {}", view_);
  spdlog::info("Camera yaw: {}, pitch: {}", yaw_, pitch_);

  CreateDeviceBuffer();
}

Camera::~Camera() {
  if (has_device_memory_) {
    vkUnmapMemory(metadata_.context->Device().device, buffer_memory_);
    vkDestroyBuffer(metadata_.context->Device().device, buffer_, nullptr);
    vkFreeMemory(metadata_.context->Device().device, buffer_memory_, nullptr);
  } else {
    spdlog::info(
        "Camera has not allocated device memory. No memory destruction needed");
  }
  spdlog::info("Successfully destroyed camera memory buffer");
}

void Camera::CreateDeviceBuffer() {
  buffer::CreateBuffer(metadata_.context, sizeof(CameraBufferObject),
                       VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
                       VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
                           VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                       buffer_, buffer_memory_);
  vkMapMemory(metadata_.context->Device().device, buffer_memory_, 0,
              sizeof(CameraBufferObject), 0, &buffer_mapped_data_);
  std::memcpy(buffer_mapped_data_, &buffer_object_, sizeof(CameraBufferObject));

  spdlog::info("Created CameraBufferObject on device memory");
  has_device_memory_ = true;
}

void Camera::UpdatePos(glm::vec3 step) {
  pos_ += step;
  buffer_object_.view_matrix         = glm::lookAt(pos_, pos_ + view_, up_);
  buffer_object_.view_matrix_inverse = glm::inverse(buffer_object_.view_matrix);
  std::memcpy(buffer_mapped_data_, &buffer_object_, sizeof(CameraBufferObject));
}

void Camera::UpdateEulerAngles(float dx, float dy) {
  // Referenced at:
  // https://learnopengl.com/code_viewer_gh.php?code=includes/learnopengl/camera.h
  //

  yaw_ += dx;
  pitch_ += dy;
  pitch_ = glm::clamp(pitch_, -89.0f, 89.0f);

  spdlog::info("Camera yaw: {}, pitch: {}", yaw_, pitch_);

  // calculate the new Front vector
  glm::vec3 front;
  front.x = cos(glm::radians(yaw_)) * cos(glm::radians(pitch_));
  front.y = sin(glm::radians(pitch_));
  front.z = sin(glm::radians(yaw_)) * cos(glm::radians(pitch_));
  view_   = glm::normalize(front);
  right_  = glm::normalize(glm::cross(view_, UP));
  up_     = glm::normalize(glm::cross(right_, view_));

  buffer_object_.view_matrix         = glm::lookAt(pos_, pos_ + view_, up_);
  buffer_object_.view_matrix_inverse = glm::inverse(buffer_object_.view_matrix);
  std::memcpy(buffer_mapped_data_, &buffer_object_, sizeof(CameraBufferObject));
}

}  // namespace volume_restir
