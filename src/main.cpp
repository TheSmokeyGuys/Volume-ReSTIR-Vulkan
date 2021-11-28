#include "Camera.hpp"
#include "Renderer.hpp"
#include "SingtonManager.hpp"
#include "VkBootstrap.h"
#include "config/static_config.hpp"
#include "spdlog/spdlog.h"

using namespace volume_restir;

int main() {
#if DEBUG
  spdlog::set_level(spdlog::level::debug);
#endif
  spdlog::info("Hello from Volumetric-ReSTIR project!");

  Renderer renderer;

  while (!SingletonManager::GetWindow().ShouldQuit()) {
    glfwPollEvents();
    renderer.Draw();
  }

  // vkDeviceWaitIdle(renderer.RenderContextPtr()->Device().device);

#ifdef _WIN32
  system("pause");
#endif

  return 0;
}
