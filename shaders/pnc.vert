#version 450

layout (location = 0) in vec3 vPosition;
layout (location = 1) in vec3 vNormal;
layout (location = 2) in vec3 vColor;

//push constants block
layout( push_constant ) uniform constants
{
	mat4 renderMatrix;
} PushConstants;

void main()
{
	gl_Position = PushConstants.renderMatrix * vec4(vPosition, 1.0f);
}