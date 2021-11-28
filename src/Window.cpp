#include "Window.hpp"

#include <cstdlib>

#include "Renderer.hpp"
#include "config/static_config.hpp"
#include "spdlog/spdlog.h"

namespace volume_restir {

/*********************************************************
 * STEP01: Pg. 42, integrating GLFW
 **********************************************************/

static void FrameBufferResizeCallback(GLFWwindow* window, int, int) {
  auto app = reinterpret_cast<Renderer*>(glfwGetWindowUserPointer(window));
  app->SetFrameBufferResized(true);
}

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
  glfwSetWindowUserPointer(window_, this);
  glfwSetFramebufferSizeCallback(window_, FrameBufferResizeCallback);

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
