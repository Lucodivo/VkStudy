#pragma once

#define SHADER_DIR "../shaders/spv/"

#define SPV_CHECK(x)                                                 \
  do                                                              \
  {                                                               \
    SpvReflectResult err = x;                                           \
    if (err != SPV_REFLECT_RESULT_SUCCESS)                                                    \
    {                                                           \
      std::cout <<"Detected SPIR-V Reflect error: " << err << std::endl; \
      abort();                                                \
    }                                                           \
  } while (0)

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
  u32 setIndex;
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
  std::vector<VkPushConstantRange> pushConstantRanges;
  ShaderInputMetadata input;
};

struct FragmentShaderMetadata {
  VkShaderModule module;
  std::vector<DescriptorSetLayoutData> descSetLayouts;
  std::vector<VkPushConstantRange> pushConstantRanges;
};

struct ShaderMetadata {
  VkShaderModule vertModule;
  VkShaderModule fragModule;
  std::vector<DescriptorSetLayoutData> descSetLayouts;
  std::vector<VkPushConstantRange> pushConstantRanges;
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
    VkShaderStageFlagBits shaderStage;
    std::vector<SpvReflectDescriptorSet*> reflectDescSets;
    std::vector<SpvReflectBlockVariable*> pushConstantBlockVars;
    std::vector<SpvReflectInterfaceVariable*> inputVars;
    std::vector<SpvReflectInterfaceVariable*> outputVars;
  };

  void mergeDescSetReflectionData(const std::vector<DescriptorSetLayoutData>& dataA,
                                  const std::vector<DescriptorSetLayoutData>& dataB,
                                  std::vector<DescriptorSetLayoutData>& dataOut);
  void getDescriptorSetLayoutData(const ReflectData& data,
                                  std::vector<DescriptorSetLayoutData>& outData);
  void getReflectData(const SpvReflectShaderModule& module, ReflectData& outData);
  void mergeReflectionData(const VertexShaderMetadata& vertShader, const FragmentShaderMetadata& fragShader, ShaderMetadata& outShader);
  void mergePushConstantRangeData(const std::vector<VkPushConstantRange>& rangesA,
                                  const std::vector<VkPushConstantRange>& rangesB,
                                  std::vector<VkPushConstantRange>& rangesOut);
  void getPushConstantRangeData(const ReflectData& data, std::vector<VkPushConstantRange>& outData);
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
	SHADER_DIR"pnct_tex_out_0.vert.spv",
	SHADER_DIR"textured_lit.frag.spv"
};