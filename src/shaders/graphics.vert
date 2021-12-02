#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(set = 0, binding = 0) uniform CameraBufferObject {
  mat4 view;
  mat4 viewInv;
  mat4 proj;
};

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inColor;
layout(location = 2) in vec3 inNormal;
layout(location = 3) in vec2 inUV;

float scaleFactor = 0.1;
vec4 xaxis= vec4(1.0f, 0.0f, 0.0f, 0.0f);
vec4 yaxis= vec4(0.0f, 1.0f ,0.0f, 0.0f);
vec4 zaxis= vec4(0.0f, 0.0f, 1.0f, 0.0f);
mat4 modelMat = mat4 (xaxis * scaleFactor, yaxis * scaleFactor, zaxis * scaleFactor, vec4(0.0f, 0.0f, 0.0f, 1.0f));

layout(location = 0) out vec3 fragColor;

void main() {
 gl_PointSize  = 0.1;
  gl_Position  = proj * view * modelMat * vec4(inPosition, 1.0);
  fragColor    = inColor;
}
