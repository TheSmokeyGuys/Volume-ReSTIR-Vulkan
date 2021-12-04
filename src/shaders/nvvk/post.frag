#version 460
#extension GL_EXT_ray_tracing : require
#extension GL_GOOGLE_include_directive : enable
#extension GL_EXT_scalar_block_layout : enable


#include "../structs/sceneStructs.glsl"
#include "../structs/light.glsl"
#include "../structs/restirStructs.glsl"

#include "../headers/binding.glsl"
#include "../headers/DebugConstants.glsl"

layout(set = 0, binding = B_SCENE) uniform Uniforms{
	SceneUniforms uniforms;
};


layout(set = 1, binding = B_POINT_LIGHTS, scalar) buffer PointLights {
	pointLight lights[];
} pointLights;
layout(set = 1, binding = B_TRIANGLE_LIGHTS, scalar) buffer TriangleLights {
	triangleLight lights[];
} triangleLights;
layout(set = 1, binding = B_ENVIRONMENTAL_MAP) uniform sampler2D environmentalTexture;


layout(set = 2, binding = B_FRAME_ALBEDO, rgba32f) uniform image2D frameAlbedo;
layout(set = 2, binding = B_FRAME_NORMAL, rgba32f) uniform image2D frameNormal;
layout(set = 2, binding = B_FRAME_MATERIAL_PROPS, rgba32f) uniform image2D frameRoughnessMetallic;
layout(set = 2, binding = B_FRAME_WORLD_POSITION, rgba32f) uniform image2D frameWorldPosition;


layout(set = 2, binding = B_RESERVIORS_INFO, rgba32f) uniform image2D reservoirInfoBuf;
layout(set = 2, binding = B_RESERVIORS_WEIGHT, rgba32f) uniform image2D reservoirWeightBuf;
layout(set = 2, binding = B_STORAGE_IMAGE, rgba32f) uniform image2D resultImage;



layout(location = 0) in vec2 inUv;

layout(location = 0) out vec3 outColor;

#include "../headers/random.glsl"
#include "../headers/restirUtils.glsl"
#include "../headers/reservoir.glsl"

#define PI 3.1415926


layout(push_constant)uniform Constants
{
  int frame;
  int initialize;
}
pushC;


void main() {
	ivec2 coordImage = ivec2(gl_FragCoord.xy);

	GeometryInfo gInfo;
	gInfo.albedo = imageLoad(frameAlbedo, coordImage);
	gInfo.normal = imageLoad(frameNormal, coordImage).xyz;
	gInfo.worldPos = imageLoad(frameWorldPosition, coordImage).xyz;
	vec2 roughnessMetallic = imageLoad(frameRoughnessMetallic, coordImage).xy;
	gInfo.roughness = roughnessMetallic.x;
	gInfo.metallic = roughnessMetallic.y;
	gInfo.camPos = uniforms.cameraPos.xyz;


	outColor = vec3(0.0f);

	if (uniforms.debugMode == DEBUG_NONE) {
		uvec2 pixelCoord = uvec2(gl_FragCoord.xy);

		vec4 resovirInfo = imageLoad(reservoirInfoBuf, coordImage);
		vec4 resovirWeight = imageLoad(reservoirWeightBuf, coordImage);
		Reservoir res = unpackResovirStruct(resovirInfo, resovirWeight);
		gInfo.sampleSeed = res.sampleSeed;

		uint lightIndex = res.lightIndex;
		int lightKind = res.lightKind;
		vec3 pHat = evaluatePHatFull(lightIndex, lightKind, gInfo);
		outColor += pHat * res.w;
		if (gInfo.albedo.w > 0.5f) {
			outColor = gInfo.albedo.xyz;
		}
	}
	else if (uniforms.debugMode == DEBUG_ALBEDO) {
		if (gInfo.albedo.a < 0.5f) {
			outColor = gInfo.albedo.rgb;
		}
		else {
			outColor = vec3(0.0f);
		}
	}
	else if (uniforms.debugMode == DEBUG_EMISSION) {
		if (gInfo.albedo.a > 0.5f) {
			outColor = gInfo.albedo.rgb;
		}
		else {
			outColor = vec3(0.0f);
		}
	}
	else if (uniforms.debugMode == DEBUG_NORMAL) {
		outColor = (vec3(gInfo.normal) + 1.0f) * 0.5f;
	}
	else if (uniforms.debugMode == DEBUG_ROUGHNESS) {
		outColor = vec3(roughnessMetallic.r);
	}
	else if (uniforms.debugMode == DEBUG_METALLIC) {
		outColor = vec3(roughnessMetallic.g);
	}
	else if (uniforms.debugMode == DEBUG_WORLD_POSITION) {
		outColor = gInfo.worldPos / 10.0f + 0.5f;
	}
	else if (uniforms.debugMode == DEBUG_NAIVE_POINT_LIGHT_NO_SHADOW) {
		float roughness = roughnessMetallic.r;
		float metallic = roughnessMetallic.g;

		outColor = vec3(0.0f);
		for (int i = 0; i < uniforms.pointLightCount; ++i) {
			outColor += evaluatePHatFull(uint(i), LIGHT_KIND_POINT, gInfo);
		}
	}

	{
		float lum = luminance(outColor);
		if (lum > uniforms.fireflyClampThreshold)
			outColor *= uniforms.fireflyClampThreshold / lum;
	}

	outColor = max(vec3(0.0), outColor);

	if (pushC.frame < 1 || pushC.initialize == 1)
	{
		imageStore(resultImage, ivec2(gl_FragCoord.xy), vec4(outColor, 1.f));
	}
	else
	{
		float a = 1.0f / float(pushC.frame);
		vec3 old_color = imageLoad(resultImage, ivec2(gl_FragCoord.xy)).xyz;
		imageStore(resultImage, ivec2(gl_FragCoord.xy), vec4(mix(old_color, outColor, a), 1.f));
		outColor = mix(old_color, outColor, a);
	}
	outColor = pow(outColor, vec3(1.0f / uniforms.gamma));

}
