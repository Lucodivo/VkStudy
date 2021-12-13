#version 450

layout (location = 0) in vec3 vPosition;
layout (location = 1) in vec3 vNormal;

layout (location = 0) out vec3 outNormal;

//push constants block
layout( push_constant ) uniform constants
{
	mat4 renderMatrix;
} pc;

void main()
{
	gl_Position = pc.renderMatrix * vec4(vPosition, 1.0f);
	outNormal = vNormal;
}
