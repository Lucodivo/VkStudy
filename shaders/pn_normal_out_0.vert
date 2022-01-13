#version 460

layout (location = 0) in vec3 vPosition;
layout (location = 1) in vec3 vNormal;

layout (location = 0) out vec3 outNormal;


struct ObjectData {
	mat4 model;
};
layout(std140, set = 1, binding = 0) readonly buffer ObjectBuffer {
	ObjectData objects[];
} objectBuffer;

void main()
{
	gl_Position = objectBuffer.objects[gl_BaseInstance].model * vec4(vPosition, 1.0f);
	outNormal = vNormal;
}
