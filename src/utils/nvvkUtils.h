#pragma once

#include <nvmath/nvmath.h>
#include <nvmath/nvmath_glsltypes.h>

#include <filesystem>
#include <iostream>
#include <optional>
#include <random>
#include <unordered_set>
#include <vector>
#include <vulkan/vulkan.hpp>

#ifndef NVVK_ALLOC_DEDICATED
#define NVVK_ALLOC_DEDICATED
#include "nvvk/allocator_vk.hpp"
#endif

#include "nvh/gltfscene.hpp"
#include "nvvk/appbase_vkpp.hpp"
#include "nvvk/descriptorsets_vk.hpp"
#include "utils/ShaderIncludes.h"

namespace volume_restir {

[[nodiscard]] std::vector<shader::pointLight> collectPointLights(
    const nvh::GltfScene&);
[[nodiscard]] std::vector<shader::pointLight> generatePointLights(
    nvmath::vec3 min, nvmath::vec3 max);

[[nodiscard]] std::vector<shader::triangleLight> collectTriangleLights(
    const nvh::GltfScene&);

[[nodiscard]] std::vector<shader::aliasTableCell> createAliasTable(
    std::vector<float>&);

}  // namespace volume_restir
