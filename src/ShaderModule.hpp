#ifndef __VOLUME_RESTIR_SHADER_MODULE_HPP__
#define __VOLUME_RESTIR_SHADER_MODULE_HPP__

#include <vulkan/vulkan.h>

#include <string>
#include <vector>

namespace volume_restir {

class ShaderModule {
public:
  static VkShaderModule Create(const std::vector<char>& code,
                               VkDevice logicalDevice);
  static VkShaderModule Create(const std::string& filename,
                               VkDevice logicalDevice);
};

}  // namespace volume_restir

#endif /* __VOLUME_RESTIR_SHADER_MODULE_HPP__ */
