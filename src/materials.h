#pragma once

#define SHADER_DIR "../shaders/spv/"

struct MaterialCreateInfo {
	const char* name;
	const char* vertFileName;
	const char* fragFileName;
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

static MaterialCreateInfo materialNormalAsColor {
	"normalAsRGB",
	SHADER_DIR"pn_normal_out_0.vert.spv",
	SHADER_DIR"in_color.frag.spv"
};

static MaterialCreateInfo materialVertexColor{
	"vertexColor",
	SHADER_DIR"pnc_color_out_0.vert.spv",
	SHADER_DIR"in_color.frag.spv"
};

static MaterialCreateInfo materialDefaultLit{
	"defaultLit",
	SHADER_DIR"pnc_color_out_0.vert.spv",
	SHADER_DIR"default_lit.frag.spv"
};

static MaterialCreateInfo materialRed {
	"red",
	SHADER_DIR"position.vert.spv",
	SHADER_DIR"red.frag.spv"
};

static MaterialCreateInfo materialGreen {
	"green",
	SHADER_DIR"position.vert.spv",
	SHADER_DIR"green.frag.spv"
};

static MaterialCreateInfo materialBlue {
	"blue",
	SHADER_DIR"position.vert.spv",
	SHADER_DIR"blue.frag.spv"
};

static MaterialCreateInfo materialDefaulColor{
	"defaultColor",
	SHADER_DIR"position_defaultCol.vert.spv",
	SHADER_DIR"in_color.frag.spv"
};

static MaterialCreateInfo materialTextured{
	"textured",
	SHADER_DIR"pnct_color_out_0_tex_out_1.vert.spv",
	SHADER_DIR"textured_lit.frag.spv"
};