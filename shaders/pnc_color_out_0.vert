#version 450

layout (location = 0) in vec3 vPosition;
layout (location = 1) in vec3 vNormal;
layout (location = 2) in vec3 vColor;

layout (location = 0) out vec3 outCOlor;

// NOTE: grabbed from descriptor set bound to slot 0, with a binding of 0 within that descriptor set
layout(set = 0, binding = 0) uniform CameraBuffer{
	mat4 view;
	mat4 proj;
	mat4 viewproj;
} cameraData;

//push constants block
layout( push_constant ) uniform constants
{
	mat4 renderMatrix;
} pc;

void main()
{
  mat4 transformMatrix = (cameraData.viewproj * pc.renderMatrix);
	gl_Position = transformMatrix * vec4(vPosition, 1.0f);
	outCOlor = vColor;
}
