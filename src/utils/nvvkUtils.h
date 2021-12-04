#pragma once
#pragma once
#define NVVK_ALLOC_DEDICATED

#include <vulkan/vulkan.hpp>
#include <nvmath/nvmath.h>
#include <nvmath/nvmath_glsltypes.h>
#include "nvvk/allocator_vk.hpp"
#include "nvvk/appbase_vkpp.hpp"
#include "nvh/gltfscene.hpp"
#include "nvvk/descriptorsets_vk.hpp"

#include "shaderIncludes.h"
#include "nvh/gltfscene.hpp"
#include <unordered_set>
#include <vector>
#include <filesystem>
#include <iostream>
#include <optional>
#include <random>


[[nodiscard]] std::vector<shader::pointLight> collectPointLights(const nvh::GltfScene&);
[[nodiscard]] std::vector<shader::pointLight> generatePointLights(
	nvmath::vec3 min, nvmath::vec3 max
);

[[nodiscard]] std::vector<shader::triangleLight> collectTriangleLights(const nvh::GltfScene&);

[[nodiscard]] std::vector<shader::aliasTableCell> createAliasTable(std::vector<float>&);

