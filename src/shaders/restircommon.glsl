/* Copyright (c) 2014-2018, NVIDIA CORPORATION. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *  * Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *  * Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *  * Neither the name of NVIDIA CORPORATION nor the names of its
 *    contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE COPYRIGHT OWNER OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
 * OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

struct HitState {
  uint InstanceID;
  uint PrimitiveID;
  vec2 bary;
  int InstanceCustomIndex;
  vec3 WorldRayOrigin;
  mat4x3 ObjectToWorld;
  mat4x3 WorldToObject;
};

struct ShadeState {
  vec3 normal;
  vec3 geom_normal;
  vec3 position;
  vec2 text_coords;
  vec3 tangent_u;
  vec3 tangent_v;
  vec3 color;
  uint matIndex;
};

// Return the vertex position
vec3 getVertex(uint index) {
  uint i = 3 * index;
  return vec3(vertices[nonuniformEXT(i + 0)], vertices[nonuniformEXT(i + 1)],
              vertices[nonuniformEXT(i + 2)]);
}

vec3 getNormal(uint index) {
  uint i = 3 * index;
  return vec3(normals[nonuniformEXT(i + 0)], normals[nonuniformEXT(i + 1)],
              normals[nonuniformEXT(i + 2)]);
}

vec2 getTexCoord(uint index) {
  uint i = 2 * index;
  return vec2(texcoord0[nonuniformEXT(i + 0)], texcoord0[nonuniformEXT(i + 1)]);
}

vec4 getTangent(uint index) {
  uint i = 4 * index;
  return vec4(tangents[nonuniformEXT(i + 0)], tangents[nonuniformEXT(i + 1)],
              tangents[nonuniformEXT(i + 2)], tangents[nonuniformEXT(i + 3)]);
}

vec4 getColor(uint index) {
  uint i = 4 * index;
  return vec4(colors[nonuniformEXT(i + 0)], colors[nonuniformEXT(i + 1)],
              colors[nonuniformEXT(i + 2)], colors[nonuniformEXT(i + 3)]);
}

ShadeState GetShadeState(in HitState hstate) {
  ShadeState sstate;

  // Retrieve the Primitive mesh buffer information
  RestirPrimitiveLookup pinfo = primInfo[hstate.InstanceCustomIndex];

  // Getting the 'first index' for this mesh (offset of the mesh + offset of the
  // triangle)
  const uint indexOffset = pinfo.indexOffset + (3 * hstate.PrimitiveID);
  const uint vertexOffset =
      pinfo.vertexOffset;  // Vertex offset as defined in glTF
  const uint matIndex =
      max(0, pinfo.materialIndex);  // material of primitive mesh

  // Getting the 3 indices of the triangle (local)
  ivec3 triangleIndex = ivec3(indices[nonuniformEXT(indexOffset + 0)],  //
                              indices[nonuniformEXT(indexOffset + 1)],  //
                              indices[nonuniformEXT(indexOffset + 2)]);
  triangleIndex += ivec3(vertexOffset);  // (global)

  const vec3 barycentrics =
      vec3(1.0 - hstate.bary.x - hstate.bary.y, hstate.bary.x, hstate.bary.y);

  // Vertex of the triangle
  const vec3 pos0 = getVertex(triangleIndex.x);
  const vec3 pos1 = getVertex(triangleIndex.y);
  const vec3 pos2 = getVertex(triangleIndex.z);
  const vec3 position =
      pos0 * barycentrics.x + pos1 * barycentrics.y + pos2 * barycentrics.z;
  const vec3 world_position = vec3(hstate.ObjectToWorld * vec4(position, 1.0));

  // Normal
  const vec3 nrm0   = getNormal(triangleIndex.x);
  const vec3 nrm1   = getNormal(triangleIndex.y);
  const vec3 nrm2   = getNormal(triangleIndex.z);
  const vec3 normal = normalize(nrm0 * barycentrics.x + nrm1 * barycentrics.y +
                                nrm2 * barycentrics.z);
  const vec3 world_normal = normalize(vec3(normal * hstate.WorldToObject));
  const vec3 geom_normal  = normalize(cross(pos1 - pos0, pos2 - pos0));
  const vec3 wgeom_normal = normalize(vec3(geom_normal * hstate.WorldToObject));

  // Tangent and Binormal
  const vec4 tng0 = getTangent(triangleIndex.x);
  const vec4 tng1 = getTangent(triangleIndex.y);
  const vec4 tng2 = getTangent(triangleIndex.z);
  vec3 tangent    = (tng0.xyz * barycentrics.x + tng1.xyz * barycentrics.y +
                  tng2.xyz * barycentrics.z);
  tangent.xyz     = normalize(tangent.xyz);
  vec3 world_tangent =
      normalize(vec3(mat4(hstate.ObjectToWorld) * vec4(tangent.xyz, 0)));
  world_tangent       = normalize(world_tangent -
                            dot(world_tangent, world_normal) * world_normal);
  vec3 world_binormal = cross(world_normal, world_tangent) * tng0.w;

  // TexCoord
  const vec2 uv0 = getTexCoord(triangleIndex.x);
  const vec2 uv1 = getTexCoord(triangleIndex.y);
  const vec2 uv2 = getTexCoord(triangleIndex.z);
  const vec2 texcoord0 =
      uv0 * barycentrics.x + uv1 * barycentrics.y + uv2 * barycentrics.z;

  // Colors
  const vec4 col0 = getColor(triangleIndex.x);
  const vec4 col1 = getColor(triangleIndex.y);
  const vec4 col2 = getColor(triangleIndex.z);
  const vec4 color =
      col0 * barycentrics.x + col1 * barycentrics.y + col2 * barycentrics.z;

  sstate.normal      = world_normal;
  sstate.geom_normal = wgeom_normal;
  sstate.position    = world_position;
  sstate.text_coords = texcoord0;
  sstate.tangent_u   = world_tangent;
  sstate.tangent_v   = world_binormal;
  sstate.color       = color.rgb;
  sstate.matIndex    = matIndex;

  // Move normal to same side as geometric normal
  if (dot(sstate.normal, sstate.geom_normal) <= 0) {
    sstate.normal *= -1.0f;
  }

  return sstate;
}

struct Material {
  vec4 albedo;
  float metallic;
  float roughness;
};

vec4 SRGBtoLINEAR(vec4 srgbIn) {
#ifdef SRGB_FAST_APPROXIMATION
  vec3 linOut = pow(srgbIn.xyz, vec3(2.2));
#else   // SRGB_FAST_APPROXIMATION
  vec3 bLess = step(vec3(0.04045), srgbIn.xyz);
  vec3 linOut =
      mix(srgbIn.xyz / vec3(12.92),
          pow((srgbIn.xyz + vec3(0.055)) / vec3(1.055), vec3(2.4)), bLess);
#endif  // SRGB_FAST_APPROXIMATION
  return vec4(linOut, srgbIn.w);
}

Material GetMetallicRoughness(GltfMaterials material, ShadeState sstate) {
  Material bsdfMat;
  float perceptualRoughness = 0.0;
  float metallic            = 0.0;
  vec4 baseColor            = vec4(0.0, 0.0, 0.0, 1.0);

  // Metallic and Roughness material properties are packed together
  // In glTF, these factors can be specified by fixed scalar values
  // or from a metallic-roughness map
  perceptualRoughness = material.pbrRoughnessFactor;
  metallic            = material.pbrMetallicFactor;
  if (material.pbrMetallicRoughnessTexture > -1) {
    // Roughness is stored in the 'g' channel, metallic is stored in the 'b'
    // channel. This layout intentionally reserves the 'r' channel for
    // (optional) occlusion map data
    vec4 mrSample = texture(
        texturesMap[nonuniformEXT(material.pbrMetallicRoughnessTexture)],
        sstate.text_coords);
    perceptualRoughness = mrSample.g * perceptualRoughness;
    metallic            = mrSample.b * metallic;
  }

  // The albedo may be defined from a base texture or a flat color
  baseColor = material.pbrBaseColorFactor;
  if (material.pbrBaseColorTexture > -1) {
    baseColor *= SRGBtoLINEAR(
        texture(texturesMap[nonuniformEXT(material.pbrBaseColorTexture)],
                sstate.text_coords));
  }

  bsdfMat.albedo    = baseColor;
  bsdfMat.metallic  = metallic;
  bsdfMat.roughness = perceptualRoughness;

  return bsdfMat;
}
const float c_MinReflectance = 0.04;

float getPerceivedBrightness(vec3 vector) {
  return sqrt(0.299 * vector.r * vector.r + 0.587 * vector.g * vector.g +
              0.114 * vector.b * vector.b);
}

float solveMetallic(vec3 diffuse, vec3 specular,
                    float oneMinusSpecularStrength) {
  float specularBrightness = getPerceivedBrightness(specular);

  if (specularBrightness < c_MinReflectance) {
    return 0.0;
  }

  float diffuseBrightness = getPerceivedBrightness(diffuse);

  float a = c_MinReflectance;
  float b =
      diffuseBrightness * oneMinusSpecularStrength / (1.0 - c_MinReflectance) +
      specularBrightness - 2.0 * c_MinReflectance;
  float c = c_MinReflectance - specularBrightness;
  float D = max(b * b - 4.0 * a * c, 0);

  return clamp((-b + sqrt(D)) / (2.0 * a), 0.0, 1.0);
}

// Specular-Glossiness which will be converted to metallic-roughness
Material GetSpecularGlossiness(GltfMaterials material, ShadeState sstate) {
  Material bsdfMat;
  float perceptualRoughness = 0.0;
  float metallic            = 0.0;
  vec4 baseColor            = vec4(0.0, 0.0, 0.0, 1.0);

  vec3 f0             = material.khrSpecularFactor;
  perceptualRoughness = 1.0 - material.khrGlossinessFactor;

  if (material.khrSpecularGlossinessTexture > -1) {
    vec4 sgSample       = SRGBtoLINEAR(texture(
        texturesMap[nonuniformEXT(material.khrSpecularGlossinessTexture)],
        sstate.text_coords));
    perceptualRoughness = 1 - material.khrGlossinessFactor *
                                  sgSample.a;  // glossiness to roughness
    f0 *= sgSample.rgb;                        // specular
  }

  vec3 specularColor             = f0;  // f0 = specular
  float oneMinusSpecularStrength = 1.0 - max(max(f0.r, f0.g), f0.b);

  vec4 diffuseColor = material.khrDiffuseFactor;
  if (material.khrDiffuseTexture > -1)
    diffuseColor *= SRGBtoLINEAR(
        texture(texturesMap[nonuniformEXT(material.khrDiffuseTexture)],
                sstate.text_coords));

  baseColor.rgb = diffuseColor.rgb * oneMinusSpecularStrength;
  metallic =
      solveMetallic(diffuseColor.rgb, specularColor, oneMinusSpecularStrength);

  bsdfMat.albedo    = baseColor;
  bsdfMat.albedo.a  = diffuseColor.a;
  bsdfMat.metallic  = metallic;
  bsdfMat.roughness = perceptualRoughness;

  return bsdfMat;
}