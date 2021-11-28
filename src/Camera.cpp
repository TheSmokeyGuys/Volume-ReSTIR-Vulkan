#include "Camera.hpp"

#include "utils/buffer.hpp"
#include "utils/logging.hpp"

#define GLM_FORCE_RADIANS
// Use Vulkan depth range of 0.0 to 1.0 instead of OpenGL
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/gtc/matrix_transform.hpp>

namespace volume_restir {

Camera::Camera(RenderContext* render_context, float fov, float aspect_ratio)
    : metadata_({render_context, fov, aspect_ratio}),
      pos_(0.f, 10.f, 10.f),
      has_device_memory_(false),
      buffer_mapped_data_(nullptr) {
  buffer_object_.view_matrix         = glm::lookAt(pos_, CENTER, UP);
  buffer_object_.view_matrix_inverse = glm::inverse(buffer_object_.view_matrix);
  buffer_object_.projection_matrix =
      glm::perspective(glm::radians(45.f), aspect_ratio, 0.1f, 100.f);
  buffer_object_.projection_matrix[1][1] *= -1;  // y-coordinate is flipped

  view_  = glm::vec3(buffer_object_.view_matrix_inverse *
                    glm::vec4(0.f, 0.f, -1.f, 0.f));
  right_ = glm::cross(view_, UP);
  up_    = glm::cross(right_, view_);

  spdlog::info("Created camera at position {}", pos_);
  spdlog::info("Camera is looking in direction {}", view_);

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

}  // namespace volume_restir
