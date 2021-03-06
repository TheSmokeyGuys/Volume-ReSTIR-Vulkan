#version 460
#extension GL_EXT_ray_tracing : require
#extension GL_GOOGLE_include_directive : enable
#extension GL_EXT_scalar_block_layout : enable
#extension GL_EXT_shader_explicit_arithmetic_types_int64 : require

#include "structs/light.glsl"
#include "host_device.h"
#include "headers/debugConstants.glsl"

layout(set = 0, binding = eUniform) uniform Uniforms {
  RestirUniforms uniforms;
};

layout(set = 1, binding = ePointLights, scalar) buffer PointLights {
  PointLight lights[];
}
pointLights;
layout(set = 1, binding = eTriangleLights, scalar) buffer TriangleLights {
  TriangleLight lights[];
}
triangleLights;
// layout(set = 1, binding = B_ENVIRONMENTAL_MAP) uniform sampler2D
// environmentalTexture;

layout(set = 2, binding = eFrameAlbedo, rgba32f) uniform image2D frameAlbedo;
layout(set = 2, binding = eFrameNormal, rgba32f) uniform image2D frameNormal;
layout(set = 2, binding = eFrameMaterialProps,
       rgba32f) uniform image2D frameRoughnessMetallic;
layout(set = 2, binding = eFrameWorldPosition,
       rgba32f) uniform image2D frameWorldPosition;

layout(set = 2, binding = eReservoirsInfo,
       rgba32f) uniform image2D reservoirInfoBuf;
layout(set = 2, binding = eReservoirWeights,
       rgba32f) uniform image2D reservoirWeightBuf;
layout(set = 3, binding = 0, rgba32f) uniform image2D resultImage;

layout(location = 0) in vec2 inUv;

layout(location = 0) out vec3 outColor;

#include "headers/random.glsl"
#include "headers/reservoir.glsl"

#define PI 3.1415926

layout(push_constant) uniform Constants {
  float clearColorRed;
  float clearColorGreen;
  float clearColorBlue;
  int frame;
  int initialize;
}
pushC;

void main() {
  ivec2 coordImage = ivec2(gl_FragCoord.xy);

  GeometryInfo gInfo;
  gInfo.albedo           = imageLoad(frameAlbedo, coordImage);
  gInfo.normal           = imageLoad(frameNormal, coordImage).xyz;
  gInfo.worldPos         = imageLoad(frameWorldPosition, coordImage).xyz;
  vec2 roughnessMetallic = imageLoad(frameRoughnessMetallic, coordImage).xy;
  gInfo.roughness        = roughnessMetallic.x;
  gInfo.metallic         = roughnessMetallic.y;
  gInfo.camPos           = uniforms.currCamPos.xyz;

  outColor = vec3(0.0f);

  uvec2 pixelCoord = uvec2(gl_FragCoord.xy);

  vec4 reservoirInfo   = imageLoad(reservoirInfoBuf, coordImage);
  vec4 reservoirWeight = imageLoad(reservoirWeightBuf, coordImage);
  Reservoir res        = unpackReservoirStruct(reservoirInfo, reservoirWeight);
  gInfo.sampleSeed     = res.sampleSeed;

  uint lightIndex = res.lightIndex;
  int lightKind   = res.lightKind;
  vec3 pHat       = evaluatePHatFull(lightIndex, lightKind, gInfo);
  outColor += pHat * res.w;
  if (gInfo.albedo.w > 0.5f) {
    outColor = gInfo.albedo.xyz;
  }

  {
    float lum = luminance(outColor);
    if (lum > uniforms.fireflyClampThreshold)
      outColor *= uniforms.fireflyClampThreshold / lum;
  }

  outColor = max(vec3(0.0), outColor);

  if (pushC.frame < 1 || pushC.initialize == 1) {
    imageStore(resultImage, ivec2(gl_FragCoord.xy), vec4(outColor, 1.f));
  } else {
    float a        = 1.0f / float(pushC.frame);
    vec3 old_color = imageLoad(resultImage, ivec2(gl_FragCoord.xy)).xyz;
    imageStore(resultImage, ivec2(gl_FragCoord.xy),
               vec4(mix(old_color, outColor, a), 1.f));
    outColor = mix(old_color, outColor, a);
  }
  // outColor = pow(outColor, vec3(1.0f / uniforms.gamma));
  outColor = pow(outColor, vec3(1.0f / 0.8f));
}
