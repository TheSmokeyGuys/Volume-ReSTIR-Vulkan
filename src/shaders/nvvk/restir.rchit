#version 460 core
#extension GL_GOOGLE_include_directive : enable
#extension GL_EXT_ray_tracing : enable
#extension GL_EXT_scalar_block_layout : enable
#extension GL_ARB_shader_clock : enable
#extension GL_EXT_nonuniform_qualifier : require

#include "structs/sceneStructs.glsl"
#include "headers/binding.glsl"


layout(location = 0) rayPayloadInEXT Payload prd;


//layout(set = , binding = B_CAMERA) uniform _CameraMatrices { CameraMatrices uniforms; };
layout(set = 1, binding = B_VERTICES) readonly buffer _VertexBuf { float vertices[]; };
layout(set = 1, binding = B_INDICES) readonly buffer _Indices { uint indices[]; };
layout(set = 1, binding = B_NORMALS) readonly buffer _NormalBuf { float normals[]; };
layout(set = 1, binding = B_TEXCOORDS) readonly buffer _TexCoordBuf { float texcoord0[]; };
layout(set = 1, binding = B_TANGENTS) readonly buffer _TangentBuf { float tangents[]; };
layout(set = 1, binding = B_COLORS) readonly buffer _ColorBuf { float colors[]; };
layout(set = 1, binding = B_MATERIALS) readonly buffer _MaterialBuffer { GltfMaterials materials[]; };
layout(set = 1, binding = B_TEXTURES) uniform sampler2D texturesMap[]; // all textures
layout(set = 1, binding = B_MATRICES) buffer _Matrices { ModelMatrices matrices[]; };

layout(set = 1, binding = B_PLIM_LOOK_UP) readonly buffer _InstanceInfo { RtPrimitiveLookup primInfo[]; };

hitAttributeEXT vec2 bary;
#include "headers/raycommon.glsl"

HitState GetState()
{
	HitState state;
	state.InstanceID = gl_InstanceID;
	state.PrimitiveID = gl_PrimitiveID;
	state.InstanceCustomIndex = gl_InstanceCustomIndexEXT;
	state.ObjectToWorld = gl_ObjectToWorldEXT;
	state.WorldToObject = gl_WorldToObjectEXT;
	state.WorldRayOrigin = gl_WorldRayOriginEXT;
	state.bary = bary;
	return state;
}

void main() {
	HitState          hstate = GetState();
	ShadeState        sstate = GetShadeState(hstate);
	GltfMaterials material = materials[nonuniformEXT(sstate.matIndex)];
	sstate.text_coords = (vec4(sstate.text_coords.xy, 1, 1) * material.uvTransform).xy;
	if (material.normalTexture > -1)
	{
		mat3 TBN = mat3(sstate.tangent_u, sstate.tangent_v, sstate.normal);
		vec3 normalVector = texture(texturesMap[nonuniformEXT(material.normalTexture)], sstate.text_coords).xyz;
		normalVector = normalize(normalVector * 2.0 - 1.0);
		normalVector *= vec3(material.normalTextureScale, material.normalTextureScale, 1.0);
		sstate.normal = normalize(TBN * normalVector);
	}
	vec3 emissive = material.emissiveFactor;
	if (material.emissiveTexture > -1)
		emissive *= SRGBtoLINEAR(texture(texturesMap[nonuniformEXT(material.emissiveTexture)], sstate.text_coords)).rgb;

	Material bsdfMat;
	if (material.shadingModel == SHADING_MODEL_METALLIC_ROUGHNESS)
		bsdfMat = GetMetallicRoughness(material, sstate);
	else
		bsdfMat = GetSpecularGlossiness(material, sstate);

	prd.worldPos.xyz = sstate.position;
	prd.worldNormal = sstate.normal;
	prd.albedo.xyz = bsdfMat.albedo.xyz;
	prd.worldPos.w = 1.0;
	prd.roughness = bsdfMat.roughness;
	prd.metallic = bsdfMat.metallic;
	prd.emissive = emissive;
	prd.exist = true;

}
