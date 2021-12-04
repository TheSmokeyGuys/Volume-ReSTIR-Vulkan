#define M_PI 3.1415926535897932384626433832795
#define M_1_PI  0.318309886183790671538


struct ModelMatrices {
	mat4 transform;
	mat4 transformInverseTransposed;
};


#define SHADING_MODEL_METALLIC_ROUGHNESS 0
#define SHADING_MODEL_SPECULAR_GLOSSINESS 1

#define ALPHA_MODE_OPAQUE 0
#define ALPHA_MODE_MASK 1
#define ALPHA_MODE_BLEND 2



struct Payload {
	vec4 worldPos;
	vec3 worldNormal;
	vec4 albedo;
	vec3 emissive;
	float roughness;
	float metallic;
	bool exist;
};




struct RtPrimitiveLookup
{
	uint indexOffset;
	uint vertexOffset;
	int  materialIndex;
};


struct GltfMaterials
{
	int shadingModel;
	vec4 pbrBaseColorFactor;

	int   pbrBaseColorTexture;
	float pbrMetallicFactor;
	float pbrRoughnessFactor;
	int   pbrMetallicRoughnessTexture;

	// KHR_materials_pbrSpecularGlossiness
	vec4 khrDiffuseFactor;
	vec3 khrSpecularFactor;
	int  khrDiffuseTexture;
	float khrGlossinessFactor;
	int khrSpecularGlossinessTexture;

	int   emissiveTexture;
	vec3 emissiveFactor;
	int  alphaMode;

	float alphaCutoff;
	int   doubleSided;
	int   normalTexture;
	float normalTextureScale;
	mat4 uvTransform;

};


struct SceneUniforms {
	mat4 view;
	mat4 proj;
	mat4 viewInverse;
	mat4 projInverse;
	mat4 projectionViewMatrix;
	mat4 prevFrameProjectionViewMatrix;
	vec4 cameraPos;
	vec4 prevCamPos;
	uvec2 screenSize;

	uint initialLightSampleCount;
	int temporalSampleCountMultiplier;

	uint spatialNeighbors;
	float spatialRadius;

	int flags;
	int debugMode;
	float gamma;

	int pointLightCount;
	int triangleLightCount;
	int aliasTableCount;

	float environmentalPower;
	float fireflyClampThreshold;
};

