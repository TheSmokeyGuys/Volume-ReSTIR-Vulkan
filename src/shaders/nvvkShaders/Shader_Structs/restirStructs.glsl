#define SPATIAL_REUSE_GROUP_SIZE_X 64
#define SPATIAL_REUSE_GROUP_SIZE_Y 1


struct GeometryInfo {
	vec3 camPos;
	vec3 worldPos;
	vec3 normal;
	vec4 albedo;
	vec3 emissive;
	float albedoLum;
	float roughness;
	float metallic;
	uint sampleSeed;
};

struct Reservoir {
	GeometryInfo info;
	vec3 lightPos;
	uint numStreamSamples;
	uint lightIndex;
	int lightKind;
	uint sampleSeed;
	float pHat;
	float sumWeights;
	float w;
};


#define RESTIR_VISIBILITY_REUSE_FLAG (1 << 0)
#define RESTIR_TEMPORAL_REUSE_FLAG (1 << 1)
#define RESTIR_SPATIAL_REUSE_FLAG (1 << 2)
#define USE_ENVIRONMENT_FLAG (1 << 3)



