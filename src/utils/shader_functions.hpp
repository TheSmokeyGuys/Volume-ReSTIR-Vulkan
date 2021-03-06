#pragma once
#include <nvmath/nvmath.h>
#include <nvmath/nvmath_glsltypes.h>

#define KIND_SPHERE 0
#define KIND_CUBE   1

namespace shader {

#define uint  ::std::uint32_t
#define vec2  ::nvmath::vec2
#define vec4  ::nvmath::vec4
#define ivec2 ::nvmath::ivec2
#define ivec4 ::nvmath::ivec4
#define uvec2 ::nvmath::uvec2
#define uvec4 ::nvmath::uvec4
#define mat4  ::nvmath::mat4

#define CPP_FUNCTION inline

#include "shaders/headers/common.glsl"

#undef uint
#undef vec2
#undef vec4
#undef ivec2
#undef ivec4
#undef uvec2
#undef uvec4
#undef mat4

#undef CPP_FUNCTION

}  // namespace shader
