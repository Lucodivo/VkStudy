#pragma once

#include <vk_types.h>
#include "types.h"

#define SHADER_DIR "../shaders/"

struct MaterialInfo {
	const char* name;
	const char* vertFileName;
	const char* fragFileName;
	VkPushConstantRange pushConstantRange;
	VkPolygonMode polygonMode;
};

struct FragmentShaderPushConstants {
	f32 time;
	f32 resolutionX;
	f32 resolutionY;
};

const VkPushConstantRange fragmentShaderPushConstantsRange{
	VK_SHADER_STAGE_FRAGMENT_BIT,
	0,
	sizeof(FragmentShaderPushConstants)
};

struct Mat4x4PushConstants {
	glm::mat4 renderMatrix;
};

const VkPushConstantRange mat4x4PushConstantsRange {
	VK_SHADER_STAGE_VERTEX_BIT,
	0,
	sizeof(Mat4x4PushConstants),
};

static MaterialInfo materialNormalAsColor {
	"normalAsRGB",
	"pn_normal_out_0.vert",
	"in_color.frag",
	mat4x4PushConstantsRange,
	VK_POLYGON_MODE_FILL
};

static MaterialInfo materialVertexColor{
	"vertexColor",
	"pnc_color_out_0.vert",
	"in_color.frag",
	mat4x4PushConstantsRange,
	VK_POLYGON_MODE_FILL
};

static MaterialInfo materialRedOutline {
	"redOutline",
	"position.vert",
	"red.frag",
	mat4x4PushConstantsRange,
	VK_POLYGON_MODE_LINE
};

static MaterialInfo materialRed {
	"red",
	"position.vert",
	"red.frag",
	mat4x4PushConstantsRange,
	VK_POLYGON_MODE_FILL
};

static MaterialInfo materialGreen {
	"green",
	"position.vert",
	"green.frag",
	mat4x4PushConstantsRange,
	VK_POLYGON_MODE_FILL
};

static MaterialInfo materialBlue {
	"blue",
	"position.vert",
	"blue.frag",
	mat4x4PushConstantsRange,
	VK_POLYGON_MODE_FILL
};