#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(set = 0, binding = 0) uniform CameraBufferObject {
  mat4 view;
  mat4 viewInv;
  mat4 proj;
}
camera;

vec2 positions[3] = vec2[](vec2(0.0, -0.5), vec2(0.5, 0.5), vec2(-0.5, 0.5));
layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inColor; 

layout(location = 0) out vec3 fragColor;

void main() {
  gl_Position = vec4(inPosition, 1.0);
  fragColor   = inColor;
}
