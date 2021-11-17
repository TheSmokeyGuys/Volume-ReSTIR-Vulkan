#include "Window.hpp"

#include <cstdlib>

#include "config/static_config.hpp"
#include "spdlog/spdlog.h"

namespace volume_restir {

Window::Window()
    : window_(nullptr),
      width_(static_config::kWindowWidth),
      height_(static_config::kWindowHeight),
      name_(static_config::kApplicationName) {
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
