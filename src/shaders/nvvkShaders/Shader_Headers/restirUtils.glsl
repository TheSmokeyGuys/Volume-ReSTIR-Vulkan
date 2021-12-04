#include "disneyBRDF.glsl"

float luminance(vec3 v) {
	return dot(v, vec3(0.212671f, 0.715160f, 0.072169f));

}

vec3 getTrianglePoint(float r1, float r2, vec3 p1, vec3 p2, vec3 p3) {
	float sqrt_r1 = sqrt(r1);
	return (1.0 - sqrt_r1) * p1 + (sqrt_r1 * (1.0 - r2)) * p2 + (r2 * sqrt_r1) * p3;
}


vec4 EnvironmentSample(uint selected_idx, vec3 to_light) {
	uvec2 tsize = textureSize(environmentalTexture, 0);
	uint  width = tsize.x;
	uint  height = tsize.y;

	uint  px = selected_idx % width;
	uint  py = selected_idx / width;
	//py = height - py - 1;
	const float dw = 1.0f / float(width);
	const float dh = 1.0f / float(height);
	const float u = float(px) * dw;
	const float phi = (u + 0.5f * dw) * (2.0f * M_PI) - M_PI;
	const float v = float(py) * dh;
	const float theta = (v + 0.5f * dh) * M_PI;

	to_light = vec3(cos(phi) * sin(theta), cos(theta), sin(phi) * sin(theta));


	return texture(environmentalTexture, vec2(u, 1.0f - v));
}



float evaluatePHat(
	uint lightIdx, int lightKind, in GeometryInfo gInfo
) {
	vec3 wi;
	float emissionLum;
	float LdotN = 1.0;
	uint seed = gInfo.sampleSeed;
	if (lightKind == LIGHT_KIND_POINT) {
		pointLight light = pointLights.lights[lightIdx];
		wi = light.pos.xyz - gInfo.worldPos;
		emissionLum = light.emission_luminance.w;
	}
	else if (lightKind == LIGHT_KIND_TRIANGLE) {
		triangleLight light = triangleLights.lights[lightIdx];
		vec3 lightSamplePos = getTrianglePoint(rnd(seed), rnd(seed), light.p1.xyz, light.p2.xyz, light.p3.xyz);
		wi = lightSamplePos - gInfo.worldPos;
		emissionLum = light.emission_luminance.w;
		vec3 normal = light.normalArea.xyz;
		LdotN = dot(normal, wi);
	}
	else if (lightKind == LIGHT_KIND_ENVIRONMENT) {
		vec4 col = 1.0f / uniforms.environmentalPower * EnvironmentSample(lightIdx, wi);
		emissionLum = 1.0f / uniforms.environmentalPower * col.a;
	}
	if (dot(wi, gInfo.normal) < 0.0f) {
		return 0.0f;
	}

	float sqrDist = dot(wi, wi);
	wi /= sqrt(sqrDist);
	vec3 wo = normalize(vec3(gInfo.camPos) - gInfo.worldPos);

	float cosIn = dot(gInfo.normal, wi);
	float cosOut = dot(gInfo.normal, wo);
	vec3 halfVec = normalize(wi + wo);
	float cosHalf = dot(gInfo.normal, halfVec);
	float cosInHalf = dot(wi, halfVec);

	float geometry = LdotN * cosIn / sqrDist;


	return emissionLum * disneyBrdfLuminance(cosIn, cosOut, cosHalf, cosInHalf, gInfo.albedoLum, gInfo.roughness, gInfo.metallic) * geometry;
}

vec3 evaluatePHatFull(
	uint lightIdx, int lightKind, in GeometryInfo gInfo
) {
	vec3 wi;
	vec3 emission;
	float LdotN = 1.0;
	uint seed = gInfo.sampleSeed;
	if (lightKind == LIGHT_KIND_POINT) {
		pointLight light = pointLights.lights[lightIdx];
		wi = light.pos.xyz - gInfo.worldPos;
		emission = light.emission_luminance.xyz;
	}
	else if (lightKind == LIGHT_KIND_TRIANGLE) {
		triangleLight light = triangleLights.lights[lightIdx];
		vec3 lightSamplePos = getTrianglePoint(rnd(seed), rnd(seed), light.p1.xyz, light.p2.xyz, light.p3.xyz);
		wi = lightSamplePos - gInfo.worldPos;
		emission = light.emission_luminance.xyz;
		vec3 normal = light.normalArea.xyz;
		LdotN = dot(normal, wi);
	}
	else if (lightKind == LIGHT_KIND_ENVIRONMENT) {
		emission = 1.0f / uniforms.environmentalPower * EnvironmentSample(lightIdx, wi).xyz;

	}

	if (dot(wi, gInfo.normal) < 0.0f) {
		return vec3(0.0f);
	}

	float sqrDist = dot(wi, wi);
	wi /= sqrt(sqrDist);
	vec3 wo = normalize(vec3(gInfo.camPos) - gInfo.worldPos);

	float cosIn = dot(gInfo.normal, wi);
	float cosOut = dot(gInfo.normal, wo);
	vec3 halfVec = normalize(wi + wo);
	float cosHalf = dot(gInfo.normal, halfVec);
	float cosInHalf = dot(wi, halfVec);

	float geometry = LdotN * cosIn / sqrDist;


	return emission * disneyBrdfColor(cosIn, cosOut, cosHalf, cosInHalf, gInfo.albedo.xyz, gInfo.roughness, gInfo.metallic) * geometry;
}




vec3 OffsetRay(in vec3 p, in vec3 n)
{
	const float intScale = 256.0f;
	const float floatScale = 1.0f / 65536.0f;
	const float origin = 1.0f / 32.0f;

	ivec3 of_i = ivec3(intScale * n.x, intScale * n.y, intScale * n.z);

	vec3 p_i = vec3(intBitsToFloat(floatBitsToInt(p.x) + ((p.x < 0) ? -of_i.x : of_i.x)),
		intBitsToFloat(floatBitsToInt(p.y) + ((p.y < 0) ? -of_i.y : of_i.y)),
		intBitsToFloat(floatBitsToInt(p.z) + ((p.z < 0) ? -of_i.z : of_i.z)));

	return vec3(abs(p.x) < origin ? p.x + floatScale * n.x : p_i.x,  //
		abs(p.y) < origin ? p.y + floatScale * n.y : p_i.y,  //
		abs(p.z) < origin ? p.z + floatScale * n.z : p_i.z);
}
