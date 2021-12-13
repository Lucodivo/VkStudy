#version 450

layout (location = 0) in vec3 vPosition;

//push constants block
layout( push_constant ) uniform constants
{
	mat4 renderMatrix;
} pc;

void main()
{
	gl_Position = pc.renderMatrix * vec4(vPosition, 1.0f);
}