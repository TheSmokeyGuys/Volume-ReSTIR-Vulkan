#pragma once

#include <nvmath/nvmath.h>
#include <nvmath/nvmath_glsltypes.h>

#include <filesystem>
#include <iostream>
#include <optional>
#include <random>
#include <unordered_set>
#include <vector>

#include "nvh/gltfscene.hpp"
#include "nvvk/appbase_vk.hpp"
#include "nvvk/descriptorsets_vk.hpp"
#include "nvvk/memallocator_dma_vk.hpp"
#include "nvvk/resourceallocator_vk.hpp"
#include "shaders/host_device.h"

[[nodiscard]] std::vector<PointLight> collectPointLights(const nvh::GltfScene&);
[[nodiscard]] std::vector<PointLight> generatePointLights(
    nvmath::vec3 min, nvmath::vec3 max, bool isGenerateWhiteLight = true,
    uint32_t numPointLightGenerates = 100);

[[nodiscard]] std::vector<TriangleLight> collectTriangleLights(
    const nvh::GltfScene&);

[[nodiscard]] std::vector<AliasTableCell> createAliasTable(
    const std::vector<float>& pdf);
