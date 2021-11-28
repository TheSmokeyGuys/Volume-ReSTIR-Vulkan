#include "Window.hpp"

#include <cstdlib>

#include "Renderer.hpp"
#include "config/static_config.hpp"
#include "spdlog/spdlog.h"

namespace volume_restir {

/*********************************************************
 * STEP01: Pg. 42, integrating GLFW
 **********************************************************/

void Window::CameraMoveCallback(int key, int action) {
  if (action == GLFW_REPEAT) {
    if (!camera_) {
      spdlog::warn("Camera class not bound to application window.");
      return;
    }

    if (key == GLFW_KEY_W) {
      camera_->UpdatePos(camera_->GetViewDir() *
                         static_config::kCameraMoveSpeed);
    } else if (key == GLFW_KEY_A) {
      camera_->UpdatePos(-1.f * camera_->GetRightDir() *
                         static_config::kCameraMoveSpeed);
    } else if (key == GLFW_KEY_S) {
      camera_->UpdatePos(-1.f * camera_->GetViewDir() *
                         static_config::kCameraMoveSpeed);
    } else if (key == GLFW_KEY_D) {
      camera_->UpdatePos(camera_->GetRightDir() *
                         static_config::kCameraMoveSpeed);
    } else if (key == GLFW_KEY_Q) {
      camera_->UpdatePos(camera_->GetUpDir() * static_config::kCameraMoveSpeed);
    } else if (key == GLFW_KEY_E) {
      camera_->UpdatePos(-1.f * camera_->GetUpDir() *
                         static_config::kCameraMoveSpeed);
    }
  }
}

void Window::MousePressCallback(int button, int action) {
  if (button == GLFW_MOUSE_BUTTON_LEFT) {
    if (action == GLFW_PRESS) {
      left_mouse_down_ = true;
      glfwGetCursorPos(window_, &mouse_pos_x_, &mouse_pos_y_);
    } else if (action == GLFW_RELEASE) {
      left_mouse_down_ = false;
    }
  } else if (button == GLFW_MOUSE_BUTTON_RIGHT) {
    if (action == GLFW_PRESS) {
      right_mouse_down_ = true;
      glfwGetCursorPos(window_, &mouse_pos_x_, &mouse_pos_y_);
    } else if (action == GLFW_RELEASE) {
      right_mouse_down_ = false;
    }
  }
}

void Window::CameraRotateCallback(float xPos, float yPos) {
  if (!camera_) {
    spdlog::warn("Camera class not bound to application window.");
    return;
  }

  if (left_mouse_down_) {
    float sensitivity = static_config::kCameraRotateSensitivity;
    float deltaX      = static_cast<float>((xPos - mouse_pos_x_) * sensitivity);
    float deltaY      = static_cast<float>((mouse_pos_y_ - yPos) * sensitivity);

    camera_->UpdateEulerAngles(deltaX, deltaY);

    mouse_pos_x_ = xPos;
    mouse_pos_y_ = yPos;
  }
}

Window::Window()
    : window_(nullptr),
      width_(static_config::kWindowWidth),
      height_(static_config::kWindowHeight),
      name_(static_config::kApplicationName),
      camera_(nullptr) {
  if (!glfwInit()) {
    spdlog::error("Failed to initialize GLFW");
    exit(EXIT_FAILURE);
  }

  if (!glfwVulkanSupported()) {
    spdlog::error("GLFW-Vulkan not supported");
    exit(EXIT_FAILURE);
  }

  glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
  glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);
  window_ = glfwCreateWindow(width_, height_, name_.c_str(), nullptr, nullptr);

  // enable callback functions as member functions
  glfwSetWindowUserPointer(window_, this);
  glfwSetKeyCallback(window_, [](GLFWwindow* w, int key, int, int action, int) {
    static_cast<Window*>(glfwGetWindowUserPointer(w))
        ->CameraMoveCallback(key, action);
  });
  glfwSetMouseButtonCallback(window_,
                             [](GLFWwindow* w, int button, int action, int) {
                               static_cast<Window*>(glfwGetWindowUserPointer(w))
                                   ->MousePressCallback(button, action);
                             });
  glfwSetCursorPosCallback(
      window_, [](GLFWwindow* w, double x_pos, double y_pos) {
        static_cast<Window*>(glfwGetWindowUserPointer(w))
            ->CameraRotateCallback(static_cast<float>(x_pos),
                                   static_cast<float>(y_pos));
      });

  if (!window_) {
    spdlog::error("Failed to initialize GLFW window");
    glfwTerminate();
    exit(EXIT_FAILURE);
  }

  spdlog::info("Initialized window with size {}x{}", height_, width_);
}

Window::~Window() {
  glfwDestroyWindow(window_);
  glfwTerminate();
}

}  // namespace volume_restir
