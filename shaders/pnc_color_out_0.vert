#version 460

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

struct ObjectData {
	mat4 model;
};
layout(std140, set = 1, binding = 0) readonly buffer ObjectBuffer {
	ObjectData objects[];
} objectBuffer;

void main()
{
  mat4 transformMatrix = (cameraData.viewproj * objectBuffer.objects[gl_BaseInstance].model);
	gl_Position = transformMatrix * vec4(vPosition, 1.0f);
	outCOlor = vColor;
}
