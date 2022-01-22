#version 460

layout (location = 0) in vec3 vPosition;

// TODO: Remove the following lines. Currently just to stop validation layer from complaining
layout (location = 1) in vec3 butts;
layout (location = 2) in vec3 butts2;
layout (location = 3) in vec3 butts3;

layout (location = 0) out vec3 outColor;

// NOTE: grabbed from descriptor set bound to slot 0, with a binding of 0 within that descriptor set
layout(set = 0, binding = 0) uniform CameraBuffer{
	mat4 view;
	mat4 proj;
	mat4 viewproj;
} cameraData;

struct ObjectData {
	mat4 model;
	vec4 defaultColor;
};
layout(std140, set = 1, binding = 0) readonly buffer ObjectBuffer {
	ObjectData objects[];
} objectBuffer;

void main()
{
	ObjectData object = objectBuffer.objects[gl_InstanceIndex];
	mat4 transformMatrix = (cameraData.viewproj * object.model);
	gl_Position = transformMatrix * vec4(vPosition, 1.0f);
	outColor = object.defaultColor.rgb;
}