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

struct DescriptorSetLayoutData {
	uint32_t setIndex;
	std::vector<VkDescriptorSetLayoutBinding> bindings;
	VkDescriptorSetLayoutCreateInfo descriptorSetLayoutCreateInfo();
};

struct ShaderInputMetadata {
	VertexInputDescription vertexInputDesc;
	VkPipelineVertexInputStateCreateInfo vertexInputStateCreateInfo();
};

struct VertexShaderMetadata {
  VkShaderModule module;
  std::vector<DescriptorSetLayoutData> descSetLayouts;
  ShaderInputMetadata input;
};

struct FragmentShaderMetadata {
  VkShaderModule module;
  std::vector<DescriptorSetLayoutData> descSetLayouts;
};

struct ShaderMetadata {
	VkShaderModule vertModule;
  VkShaderModule fragModule;
  std::vector<DescriptorSetLayoutData> descSetLayouts;
  ShaderInputMetadata input;
};

class MaterialManager {
public:
	std::unordered_map<std::string, VertexShaderMetadata> cachedVertShaders;
  std::unordered_map<std::string, FragmentShaderMetadata> cachedFragShaders;
  std::unordered_map<std::string, ShaderMetadata> cachedShaders;
	void destroyAll(VkDevice device);

	void loadShaderMetadata(VkDevice device, const char* vertFileName, const char* fragFileName, ShaderMetadata& out);

private:

  struct ReflectData {
    std::vector<SpvReflectDescriptorSet*> reflectDescSets;
    std::vector<SpvReflectInterfaceVariable*> inputVars;
    std::vector<SpvReflectInterfaceVariable*> outputVars;
  };

	void mergeDescSetReflectionData(const std::vector<DescriptorSetLayoutData>& dataA,
																	const std::vector<DescriptorSetLayoutData>& dataB,
																	std::vector<DescriptorSetLayoutData>& out);
	void getDescriptorSetLayoutData(const SpvReflectShaderModule& module, 
																	const ReflectData& data, 
																	std::vector<DescriptorSetLayoutData>& outData);
	void getReflectData(const SpvReflectShaderModule module, ReflectData& outData);

  void mergeReflectionData(const VertexShaderMetadata& vertShader, const FragmentShaderMetadata& fragShader, ShaderMetadata& outShader);
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