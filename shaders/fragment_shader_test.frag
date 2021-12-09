#version 450

layout (location = 0) out vec4 outFragColor;

layout( push_constant ) uniform constants
{
	float frameNumber;
	float resolutionX;
	float resolutionY;
} PushConstants;

void main()
{
  float frameOver60 = PushConstants.frameNumber / 60.0f;
	float red = (sin(frameOver60) + 1.0f) * 0.5f;
	outFragColor = vec4(red, 0.4f, 0.4f,1.0f);
}
