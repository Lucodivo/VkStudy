#version 460

layout (location = 0) in vec2 texCoord;

layout (location = 0) out vec4 outFragColor;

layout(set = 2, binding = 0) uniform sampler2D tex1;

layout(set = 0, binding = 1) uniform  SceneData {
	vec4 fogColor; // w is for exponent
	vec4 fogDistances; //x for min, y for max, zw unused.
	vec4 ambientColor;
	vec4 sunlightDirection; //w for sun power
	vec4 sunlightColor;
} sceneData;


void main()
{
	vec4 color = texture(tex1, texCoord);
	if(color.a < 0.01) { discard; }
	outFragColor = color;
}
