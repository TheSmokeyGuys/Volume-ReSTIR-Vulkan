#version 460
#extension GL_GOOGLE_include_directive : enable
#extension GL_EXT_ray_tracing : enable
// Align structure layout to scalar
#extension GL_EXT_scalar_block_layout : enable
#include "../structs/sceneStructs.glsl"
#include "../headers/binding.glsl"
#include "../structs/restirStructs.glsl"

layout(location = 0) rayPayloadInEXT Payload prd;

layout(set = 0, binding = B_SCENE) uniform Restiruniforms{
	SceneUniforms uniforms;
};
layout(set = 2, binding = B_ENVIRONMENTAL_MAP) uniform sampler2D environmentalTexture;

vec2 GetSphericalUv(vec3 v)
{
	float gamma = asin(-v.y);
	float theta = atan(v.z, v.x);

	vec2 uv = vec2(theta * M_1_PI * 0.5, gamma * M_1_PI) + 0.5;
	return uv;
}

void main() {
	prd.worldPos.w = 0.0;
	prd.exist = false;
	if ((uniforms.flags & USE_ENVIRONMENT_FLAG) != 0) {
		vec2 uv = GetSphericalUv(gl_WorldRayDirectionEXT.xyz);
		prd.emissive = texture(environmentalTexture, uv).rgb;
		prd.albedo = vec4(1.0);
	}
}
