/*
 * Copyright (c) 2019-2021, NVIDIA CORPORATION.  All rights reserved.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 * SPDX-FileCopyrightText: Copyright (c) 2019-2021 NVIDIA CORPORATION
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef COMMON_HOST_DEVICE
#define COMMON_HOST_DEVICE

#ifdef __cplusplus
#include "nvmath/nvmath.h"
// GLSL Type
using vec2  = nvmath::vec2f;
using vec3  = nvmath::vec3f;
using vec4  = nvmath::vec4f;
using mat4  = nvmath::mat4f;
using uvec2 = nvmath::vec2ui;
using uint  = unsigned int;
#endif

// clang-format off
#ifdef __cplusplus // Descriptor binding helper for C++ and GLSL
 #define START_BINDING(a) enum a {
 #define END_BINDING() }
#else
 #define START_BINDING(a)  const uint
 #define END_BINDING() 
#endif

START_BINDING(SceneBindings)
 eGlobals  = 0,  // Global uniform containing camera matrices
 eObjDescs = 1,  // Access to the object descriptions
 eTextures = 2,  // Access to textures
 eImplicit = 3,  // All implicit objects
 eGLTFVertices    = 4,
 eGLTFIndices     = 5,
 eGLTFNormals     = 6,
 eGLTFTexcoords   = 7,
 eGLTFTangents    = 8,
 eGLTFColors      = 9,
 eGLTFMaterials   = 10,
 eGLTFTextures    = 11,
 eGLTFMatrices    = 12,
 eGLTFPrimLookup  = 13
END_BINDING();

START_BINDING(RtxBindings)
 eTlas     = 0,  // Top-level acceleration structure
 eOutImage = 1   // Ray tracer output image
END_BINDING();

START_BINDING(LightBindings) // m_lightDescSetLayoutBind
 ePointLights    = 0,
 eTriangleLights = 1,
 eAliasTable     = 2
END_BINDING();

START_BINDING(RestirUniformBindings)
 eUniform  = 0  // Global uniform containing camera matrices
END_BINDING();

START_BINDING(RestirBindings)
 eFrameWorldPosition      = 0,
 eFrameAlbedo             = 1,
 eFrameNormal             = 2,
 eFrameMaterialProps      = 3,
 ePrevFrameWorldPosition  = 4,
 ePrevFrameAlbedo         = 5,
 ePrevFrameNormal         = 6,
 ePrevFrameMaterialProps  = 7,
 eReservoirsInfo          = 8,
 eReservoirWeights        = 9,
 ePrevReservoirInfo       = 10,
 ePrevReservoirWeight     = 11,
 eTmpReservoirInfo        = 12,
 eTmpReservoirWeight      = 13,
 eStorageImage            = 14,
 eOutImageGBuffer         = 15,
 eOutImagePrevGBuffer     = 16
END_BINDING();
// clang-format on

#define SHADING_MODEL_METALLIC_ROUGHNESS  0
#define SHADING_MODEL_SPECULAR_GLOSSINESS 1

#define ALPHA_MODE_OPAQUE 0
#define ALPHA_MODE_MASK   1
#define ALPHA_MODE_BLEND  2

// Information of a obj model when referenced in a shader
struct ObjDesc {
  int txtOffset;             // Texture index offset in the array of textures
  uint64_t vertexAddress;    // Address of the Vertex buffer
  uint64_t indexAddress;     // Address of the index buffer
  uint64_t materialAddress;  // Address of the material buffer
  uint64_t
      materialIndexAddress;  // Address of the triangle material index buffer
};

// Uniform buffer set at each frame
struct GlobalUniforms {
  mat4 viewProj;     // Camera view * projection
  mat4 viewInverse;  // Camera inverse view matrix
  mat4 projInverse;  // Camera inverse projection matrix
};

// Push constant structure for the raster
struct PushConstantRaster {
  mat4 modelMatrix;  // matrix of the instance
  vec3 lightPosition;
  uint objIndex;
  float lightIntensity;
  int lightType;
};

// Push constant structure for the ray tracer
struct PushConstantRay {
  vec4 clearColor;
  vec3 lightPosition;
  float lightIntensity;
  int lightType;
};

struct PushConstantRestir {
  int frame;
  int initialize;
  vec4 clearColor;
};

struct Vertex  // See ObjLoader, copy of VertexObj, could be compressed for
               // device
{
  vec3 pos;
  vec3 nrm;
  vec3 color;
  vec2 texCoord;
};

struct WaveFrontMaterial  // See ObjLoader, copy of MaterialObj, could be
                          // compressed for device
{
  vec3 ambient;
  vec3 diffuse;
  vec3 specular;
  vec3 transmittance;
  vec3 emission;
  float shininess;
  float ior;       // index of refraction
  float dissolve;  // 1 == opaque; 0 == fully transparent
  int illum;       // illumination model (see
                   // http://www.fileformat.info/format/material/)
  int textureId;
};

struct GltfMaterials {
  int shadingModel;
  vec4 pbrBaseColorFactor;

  int pbrBaseColorTexture;
  float pbrMetallicFactor;
  float pbrRoughnessFactor;
  int pbrMetallicRoughnessTexture;

  // KHR_materials_pbrSpecularGlossiness
  vec4 khrDiffuseFactor;
  vec3 khrSpecularFactor;
  int khrDiffuseTexture;
  float khrGlossinessFactor;
  int khrSpecularGlossinessTexture;

  int emissiveTexture;
  vec3 emissiveFactor;
  int alphaMode;

  float alphaCutoff;
  int doubleSided;
  int normalTexture;
  float normalTextureScale;
  mat4 uvTransform;
};

struct GLTFModelMatrices {
  mat4 transform;
  mat4 transformInverseTransposed;
};

struct RestirPrimitiveLookup {
  uint indexOffset;
  uint vertexOffset;
  int materialIndex;
};

struct Sphere {
  vec3 center;
  float radius;
};

struct Aabb {
  vec3 minimum;
  vec3 maximum;
};

// ReSTIR params
struct PointLight {  // m_lightDescSetLayoutBind
  vec4 pos;
  vec4 emission_luminance;  // w is luminance
};

struct TriangleLight {  // m_lightDescSetLayoutBind
  vec4 p1;
  vec4 p2;
  vec4 p3;
  vec4 emission_luminance;  // w is luminance
  vec4 normalArea;
};

struct AliasTableCell {  // m_lightDescSetLayoutBind
  int alias;
  float prob;
  float pdf;
  float aliasPdf;
};

struct RestirUniforms {  // m_restirUniformDescSetLayoutBind
  int pointLightCount;
  int triangleLightCount;
  int aliasTableCount;

  float environmentalPower;
  float fireflyClampThreshold;

  uint spatialNeighbors;
  float spatialRadius;

  uint initialLightSampleCount;
  int temporalSampleCountMultiplier;

  uvec2 screenSize;
  vec4 currCamPos;
  mat4 currFrameProjectionViewMatrix;
  vec4 prevCamPos;
  mat4 prevFrameProjectionViewMatrix;

  int flags;
  int debugMode;
  float gamma;
};

#define KIND_SPHERE 0
#define KIND_CUBE   1

#define RESTIR_VISIBILITY_REUSE_FLAG (1 << 0)
#define RESTIR_TEMPORAL_REUSE_FLAG   (1 << 1)
#define RESTIR_SPATIAL_REUSE_FLAG    (1 << 2)
#define USE_ENVIRONMENT_FLAG         (1 << 3)

#endif
