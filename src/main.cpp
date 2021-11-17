#include "SingtonManager.hpp"
#include "VkBootstrap.h"
#include "spdlog/spdlog.h"

int main() {
  spdlog::info("Hello from Volumetric-ReSTIR project!");

  vkb::InstanceBuilder instance_builder;
  auto instance_build_success = instance_builder.set_app_name("Volume ReSTIR")
                                    .request_validation_layers()
                                    .use_default_debug_messenger()
                                    .build();
  if (!instance_build_success) {
    spdlog::error("Failed to create Vulkan instance: {}",
                  instance_build_success.error().message());
  }

  while (!volume_restir::SingletonManager::GetWindow().ShouldQuit()) {
    glfwPollEvents();
  }

#ifdef _WIN32
  system("pause");
#endif

  return 0;
}
