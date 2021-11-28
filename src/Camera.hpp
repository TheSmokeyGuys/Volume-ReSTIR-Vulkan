#ifndef __VOLUME_RESTIR_CAMERA_HPP__
#define __VOLUME_RESTIR_CAMERA_HPP__

#include <vulkan/vulkan.h>

#include "RenderContext.hpp"
#include "glm/glm.hpp"

namespace volume_restir {

struct CameraBufferObject {
  glm::mat4 view_matrix;
  glm::mat4 view_matrix_inverse;
  glm::mat4 projection_matrix;
};

struct CameraMetaData {
  RenderContext* context;
  float fov;  // in degrees
  float aspect_ratio;
};

class Camera {
public:
  static constexpr glm::vec3 RIGHT   = glm::vec3(1.f, 0.f, 0.f);
  static constexpr glm::vec3 UP      = glm::vec3(0.f, 1.f, 0.f);
  static constexpr glm::vec3 FORWARD = glm::vec3(0.f, 0.f, 1.f);
  static constexpr glm::vec3 CENTER  = glm::vec3(0.f, 0.f, 0.f);

  VkBuffer GetBuffer() const { return buffer_; }

  /**
   * @brief Construct a new Camera object
   *
   * @param render_context: pointer to RenderContext (for device memory binding)
   * @param fov:            field of view angle, in degrees
   * @param aspect_ratio:   aspect ratio
   */
  Camera(RenderContext* render_context, float fov, float aspect_ratio);
  ~Camera();

  glm::vec3 GetPos() const { return pos_; }
  glm::vec3 GetViewDir() const { return view_; }
  glm::vec3 GetRightDir() const { return right_; }
  glm::vec3 GetUpDir() const { return up_; }

  // Referenced at:
  //  https://learnopengl.com/code_viewer_gh.php?code=includes/learnopengl/camera.h
  //
  void UpdatePos(glm::vec3 new_pos);
  void UpdateEulerAngles(float dx, float dy);

  /**
   * @brief Allocates memory buffer on device
   *
   */
  void CreateDeviceBuffer();

private:
  CameraMetaData metadata_;

  glm::vec3 pos_;    // position of camera in world space
  glm::vec3 view_;   // view direction of camera
  glm::vec3 right_;  // right direction of camera
  glm::vec3 up_;     // up direction of camera
  float yaw_;        // yaw angle of camera
  float pitch_;      // pitch angle of camera
  float roll_;       // pitch angle of camera

  // Device memory related
  bool has_device_memory_;
  CameraBufferObject buffer_object_;
  VkBuffer buffer_;
  VkDeviceMemory buffer_memory_;
  void* buffer_mapped_data_;
};

}  // namespace volume_restir

#endif /* __VOLUME_RESTIR_CAMERA_HPP__ */
