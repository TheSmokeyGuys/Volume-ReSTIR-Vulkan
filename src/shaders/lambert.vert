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

vec4 lightPos = vec4(0.0, 10.0, 0.0, 1.0);

float scaleFactor = 0.01;
vec4 xaxis= vec4(1.0f, 0.0f, 0.0f, 0.0f);
vec4 yaxis= vec4(0.0f, 1.0f ,0.0f, 0.0f);
vec4 zaxis= vec4(0.0f, 0.0f, 1.0f, 0.0f);
mat4 modelMat = mat4 (xaxis * scaleFactor, yaxis * scaleFactor, zaxis * scaleFactor, vec4(0.0f, 0.0f, 0.0f, 1.0f));

layout(location = 0) out vec3 fragColor;

layout(location = 1) out vec4 fs_Nor;            // The array of normals that has been transformed by u_ModelInvTr. This is implicitly passed to the fragment shader.
layout(location = 2) out vec4 fs_LightVec;       // The direction in which our virtual light lies, relative to each vertex. This is implicitly passed to the fragment shader.

void main() {
  gl_PointSize  = 10;

 vec4 modelposition = modelMat * vec4(inPosition, 1.0); 
  gl_Position  = proj * view * modelposition;

  fs_Nor = vec4( inNormal, 1.0);

  lightPos = viewInv * lightPos;
  fs_LightVec = lightPos - modelposition; 

  fragColor    = inColor;
}
