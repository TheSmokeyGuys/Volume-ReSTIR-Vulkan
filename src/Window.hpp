#ifndef __VOLUME_RESTIR_WINDOW_HPP__
#define __VOLUME_RESTIR_WINDOW_HPP__

#ifdef _WIN32
#pragma comment(linker, "/subsystem:console")
#include <windows.h>
#elif defined(__linux__)
#include <xcb/xcb.h>
#endif

#include <GLFW/glfw3.h>

#include <string>

#include "Camera.hpp"

namespace volume_restir {

class Window {
public:
  Window();
  ~Window();

  bool ShouldQuit() const { return !!glfwWindowShouldClose(window_); }
  GLFWwindow* WindowPtr() const noexcept { return window_; }
  int Width() const noexcept { return width_; }
  int Height() const noexcept { return height_; }
  std::string Name() const noexcept { return name_; }

  void BindCamera(Camera* camera) { camera_ = camera; }
  void CameraMoveCallback(int key, int action);
  void CameraRotateCallback(float xPos, float yPos);
  void MousePressCallback(int button, int action);

private:
  GLFWwindow* window_;
  int width_;
  int height_;
  std::string name_;

  Camera* camera_;
  bool left_mouse_down_;
  bool right_mouse_down_;
  double mouse_pos_x_;
  double mouse_pos_y_;
};

}  // namespace volume_restir

#endif /* __VOLUME_RESTIR_WINDOW_HPP__ */
