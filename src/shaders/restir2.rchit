#version 460 core
#extension GL_GOOGLE_include_directive : enable
#extension GL_EXT_ray_tracing : enable
#extension GL_EXT_scalar_block_layout : enable
#extension GL_ARB_shader_clock : enable
#extension GL_EXT_nonuniform_qualifier : require
#extension GL_EXT_shader_explicit_arithmetic_types_int64 : require
#extension GL_EXT_buffer_reference2 : require

#include "host_device.h"
#include "raycommon.glsl"

// clang-format off

layout(location = 0) rayPayloadInEXT Payload prd;

layout(set = 0, binding = eTlas) uniform accelerationStructureEXT topLevelAS;
layout(set = 1, binding = eImplicit, scalar) buffer allSpheres_ {Sphere i[];} allSpheres;

// clang-format on

layout(set = 1, binding = eGlobals) uniform _GlobalUniforms {
  GlobalUniforms globalUniform;
};
layout(set = 1, binding = eGLTFVertices) readonly buffer _VertexBuf {
  float vertices[];
};
layout(set = 1, binding = eGLTFIndices) readonly buffer _Indices {
  uint indices[];
};
layout(set = 1, binding = eGLTFNormals) readonly buffer _NormalBuf {
  float normals[];
};
layout(set = 1, binding = eGLTFTexcoords) readonly buffer _TexCoordBuf {
  float texcoord0[];
};
layout(set = 1, binding = eGLTFTangents) readonly buffer _TangentBuf {
  float tangents[];
};
layout(set = 1, binding = eGLTFColors) readonly buffer _ColorBuf {
  float colors[];
};
layout(set = 1, binding = eGLTFMaterials) readonly buffer _MaterialBuffer {
  GltfMaterials materials[];
};
layout(set = 1, binding = eGLTFTextures) uniform sampler2D texturesMap[];
layout(set = 1, binding = eGLTFMatrices) buffer _Matrices {
  GLTFModelMatrices matrices[];
};

layout(set     = 1,
       binding = eSphereMaterial) readonly buffer _SphereMaterialBuffer {
  GltfMaterials sphereMaterials[];
};

layout(set = 1, binding = eGLTFPrimLookup) readonly buffer _InstanceInfo {
  RestirPrimitiveLookup primInfo[];
};

hitAttributeEXT vec2 bary;
#include "restircommon.glsl"

HitState GetState() {
  HitState state;
  state.InstanceID          = gl_InstanceID;
  state.PrimitiveID         = gl_PrimitiveID;
  state.InstanceCustomIndex = gl_InstanceCustomIndexEXT;
  state.ObjectToWorld       = gl_ObjectToWorldEXT;
  state.WorldToObject       = gl_WorldToObjectEXT;
  state.WorldRayOrigin      = gl_WorldRayOriginEXT;
  state.bary                = bary;
  return state;
}

void main() {
  vec3 worldPos   = gl_WorldRayOriginEXT + gl_WorldRayDirectionEXT * gl_HitTEXT;
  Sphere instance = allSpheres.i[gl_PrimitiveID];
  // Computing the normal at hit position
  vec3 worldNrm = normalize(worldPos - instance.center);

  HitState hstate   = GetState();
  ShadeState sstate = GetShadeState(hstate);

  // specific material for VDB spheres
  GltfMaterials material;
  material.shadingModel                 = 0;
  material.pbrBaseColorFactor           = vec4(0.5, 0.1, 0.2, 1);
  material.pbrBaseColorTexture          = -1;
  material.pbrMetallicFactor            = 1.0;
  material.pbrRoughnessFactor           = 1.0;
  material.pbrMetallicRoughnessTexture  = -1;
  material.khrDiffuseFactor             = vec4(1.0, 1.0, 1.0, 1.0);
  material.khrSpecularFactor            = vec3(1.0, 1.0, 1.0);
  material.khrDiffuseTexture            = -1;
  material.khrGlossinessFactor          = 1.0;
  material.khrSpecularGlossinessTexture = -1;
  material.emissiveTexture              = -1;
  material.emissiveFactor               = vec3(0.3, 0.3, 0.3);
  material.alphaMode                    = 0;
  material.alphaCutoff                  = 0.5;
  material.doubleSided                  = 0;
  material.normalTexture                = -1;
  material.normalTextureScale           = 1.0;
  material.uvTransform                  = mat4(1.0);

  GltfMaterials currSphereMaterials = sphereMaterials[gl_PrimitiveID];

  prd.worldPos.xyz = worldPos;
  prd.worldNormal  = worldNrm;
  prd.albedo       = currSphereMaterials.pbrBaseColorFactor;
  prd.worldPos.w   = 1.0;
  prd.roughness    = currSphereMaterials.pbrRoughnessFactor;
  prd.metallic     = currSphereMaterials.pbrMetallicFactor;
  prd.emissive     = currSphereMaterials.emissiveFactor;
  prd.exist        = true;

  // prd.worldPos.xyz = sstate.position;
  // prd.worldNormal  = sstate.normal;
  // prd.albedo.xyz   = bsdfMat.albedo.xyz;
  // prd.worldPos.w   = 1.0;
  // prd.roughness    = bsdfMat.roughness;
  // prd.metallic     = bsdfMat.metallic;
  // prd.emissive     = emissive;
  // prd.exist        = true;
}
