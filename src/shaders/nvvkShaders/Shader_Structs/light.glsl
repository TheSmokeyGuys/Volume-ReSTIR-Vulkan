struct pointLight {
	vec4 pos;
	vec4 emission_luminance; // w is luminance
};

struct triangleLight {
	vec4 p1;
	vec4 p2;
	vec4 p3;
	vec4 emission_luminance; // w is luminance
	vec4 normalArea;
};

#define LIGHT_KIND_POINT 0
#define LIGHT_KIND_TRIANGLE 1
#define LIGHT_KIND_ENVIRONMENT 2 //in future


struct aliasTableCell {
	int alias;
	float prob;
	float pdf;
	float aliasPdf;
};

