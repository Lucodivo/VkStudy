#pragma once

#define SHADER_DIR "../shaders/"

struct MaterialInfo {
	const char* name;
	const char* vertFileName;
	const char* fragFileName;
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

static MaterialInfo materialNormalAsColor {
	"normalAsRGB",
	"pn_normal_out_0.vert",
	"in_color.frag",
	VK_POLYGON_MODE_FILL
};

static MaterialInfo materialVertexColor{
	"vertexColor",
	"pnc_color_out_0.vert",
	"in_color.frag",
	VK_POLYGON_MODE_FILL
};

static MaterialInfo materialDefaultLit{
	"defaultLit",
	"pnc_color_out_0.vert",
	"default_lit.frag",
	VK_POLYGON_MODE_FILL
};

static MaterialInfo materialRedOutline {
	"redOutline",
	"position.vert",
	"red.frag",
	VK_POLYGON_MODE_LINE
};

static MaterialInfo materialRed {
	"red",
	"position.vert",
	"red.frag",
	VK_POLYGON_MODE_FILL
};

static MaterialInfo materialGreen {
	"green",
	"position.vert",
	"green.frag",
	VK_POLYGON_MODE_FILL
};

static MaterialInfo materialBlue {
	"blue",
	"position.vert",
	"blue.frag",
	VK_POLYGON_MODE_FILL
};

static MaterialInfo materialDefaulColor{
	"defaultColor",
	"position_defaultCol.vert",
	"in_color.frag",
	VK_POLYGON_MODE_FILL
};