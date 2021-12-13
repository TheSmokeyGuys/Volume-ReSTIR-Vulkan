#version 460
#extension GL_GOOGLE_include_directive : enable
#extension GL_EXT_ray_tracing : enable
// Align structure layout to scalar
#extension GL_EXT_scalar_block_layout : enable
#extension GL_EXT_shader_explicit_arithmetic_types_int64 : require

#include "host_device.h"
#include "raycommon.glsl"
#include "headers/math.glsl"

layout(location = 0) rayPayloadInEXT Payload prd;

layout(set = 2, binding = eUniform) uniform _RestirUniforms {
  RestirUniforms uniforms;
};
// layout(set = 2, binding = B_ENVIRONMENTAL_MAP) uniform sampler2D
// environmentalTexture;

layout(push_constant) uniform _PushConstantRestir {
  int frame;
  int initialize;
  vec4 clearColor;
}
pushC;

vec2 GetSphericalUv(vec3 v) {
  float gamma = asin(-v.y);
  float theta = atan(v.z, v.x);

  vec2 uv = vec2(theta * M_1_PI * 0.5, gamma * M_1_PI) + 0.5;
  return uv;
}

void main() {
  prd.worldPos.w = 0.0;
  prd.exist      = false;
  prd.albedo.xyz = pushC.clearColor.xyz * 0.8;
  // if ((uniforms.flags & USE_ENVIRONMENT_FLAG) != 0) {
  //   vec2 uv      = GetSphericalUv(gl_WorldRayDirectionEXT.xyz);
  //   prd.emissive = texture(environmentalTexture, uv).rgb;
  //   prd.albedo   = vec4(1.0);
  // }
}
