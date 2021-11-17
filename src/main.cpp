#include "Renderer.hpp"
#include "SingtonManager.hpp"
#include "VkBootstrap.h"
#include "spdlog/spdlog.h"

int main() {
#if DEBUG
  spdlog::set_level(spdlog::level::debug);
#endif
  spdlog::info("Hello from Volumetric-ReSTIR project!");

  volume_restir::Renderer renderer;

  while (!volume_restir::SingletonManager::GetWindow().ShouldQuit()) {
    glfwPollEvents();
    renderer.Draw();
  }

#ifdef _WIN32
  system("pause");
#endif

  return 0;
}
