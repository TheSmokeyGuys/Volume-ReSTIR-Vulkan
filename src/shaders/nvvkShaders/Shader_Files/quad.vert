#version 450
#extension GL_KHR_vulkan_glsl : enable

vec2 positions[4] = vec2[](
	vec2(-1.0f, -1.0f),
	vec2(-1.0f, 1.0f),
	vec2(1.0f, -1.0f),
	vec2(1.0f, 1.0f)
	);

layout(location = 0) out vec2 outUV;

void main() {
	outUV = vec2((gl_VertexIndex << 1) & 2, gl_VertexIndex & 2);
	gl_Position = vec4(outUV * 2.0f - 1.0f, 1.0f, 1.0f);
}
