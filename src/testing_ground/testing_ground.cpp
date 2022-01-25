#include "../vk_study.h"

#include "testing_util.h"

#include <unordered_map>

struct DescriptorSetLayoutData {
  uint32_t setIndex;
  std::vector<VkDescriptorSetLayoutBinding> bindings;

  VkDescriptorSetLayoutCreateInfo descriptorSetLayoutCreateInfo() {
    VkDescriptorSetLayoutCreateInfo descriptorSetLayoutCreateInfo{};
    descriptorSetLayoutCreateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    descriptorSetLayoutCreateInfo.pNext = nullptr;
    descriptorSetLayoutCreateInfo.flags = 0;
    descriptorSetLayoutCreateInfo.bindingCount = static_cast<u32>(bindings.size());
    descriptorSetLayoutCreateInfo.pBindings = bindings.data();
    return descriptorSetLayoutCreateInfo;
  }
};
struct ShaderInputOutputMetaData {
  VertexInputDescription vertexInputDesc;

  VkPipelineVertexInputStateCreateInfo vertexInputStateCreateInfo() {
    VkPipelineVertexInputStateCreateInfo vertexInputStateCreateInfo{};
    vertexInputStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
    vertexInputStateCreateInfo.pNext = nullptr;
    vertexInputStateCreateInfo.flags = 0;
    vertexInputStateCreateInfo.vertexBindingDescriptionCount = 1;
    vertexInputStateCreateInfo.pVertexBindingDescriptions = &vertexInputDesc.bindingDesc;
    vertexInputStateCreateInfo.vertexAttributeDescriptionCount = static_cast<u32>(vertexInputDesc.attributes.size());
    vertexInputStateCreateInfo.pVertexAttributeDescriptions = vertexInputDesc.attributes.data();
    return vertexInputStateCreateInfo;
  }
};

struct ShaderMetaData {
  std::vector<DescriptorSetLayoutData> descSetLayouts;
  ShaderInputOutputMetaData inputOutputMetaData;
};

struct ReflectData {
  std::vector<SpvReflectDescriptorSet*> reflectDescSets;
  std::vector<SpvReflectInterfaceVariable*> inputVars;
  std::vector<SpvReflectInterfaceVariable*> outputVars;
};

void getDescriptorSetLayoutData(const SpvReflectShaderModule& module, const ReflectData& data, std::vector<DescriptorSetLayoutData>& outData) {
  u64 descSetCount = data.reflectDescSets.size();
  outData.resize(descSetCount, {});
  for (u64 setIndex = 0; setIndex < descSetCount; ++setIndex) {
    const SpvReflectDescriptorSet& reflectDescSet = *(data.reflectDescSets[setIndex]);
    DescriptorSetLayoutData& layout = outData[setIndex];

    layout.setIndex = reflectDescSet.set;
    layout.bindings.resize(reflectDescSet.binding_count);
    for (u32 bindingIndex = 0; bindingIndex < reflectDescSet.binding_count; ++bindingIndex) {
      const SpvReflectDescriptorBinding& reflectBinding = *(reflectDescSet.bindings[bindingIndex]);
      VkDescriptorSetLayoutBinding& layoutBinding = layout.bindings[bindingIndex];
      layoutBinding.binding = reflectBinding.binding;
      // SPIRV-Reflect will not know the difference between VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER
      // and VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC
      // TODO: Default to dynamic uniform buffer?
      layoutBinding.descriptorType = static_cast<VkDescriptorType>(reflectBinding.descriptor_type);
      layoutBinding.descriptorCount = 1;
      // TODO: Understand this???
      for (u32 i_dim = 0; i_dim < reflectBinding.array.dims_count; ++i_dim) {
        layoutBinding.descriptorCount *= reflectBinding.array.dims[i_dim];
      }
      layoutBinding.stageFlags = static_cast<VkShaderStageFlagBits>(module.shader_stage);
    }

    // sort each set by desc binding index
    std::sort(layout.bindings.begin(), layout.bindings.end(),
      [](const VkDescriptorSetLayoutBinding& a, const VkDescriptorSetLayoutBinding& b) {
        assert(a.binding != b.binding); // bindings should never be the same
        return a.binding < b.binding;
      });
  }

  // sort the sets by their set index
  std::sort(outData.begin(), outData.end(),
    [](const DescriptorSetLayoutData& a, const DescriptorSetLayoutData& b) {
      assert(a.setIndex != b.setIndex); // There should be no set number duplicates
      return a.setIndex < b.setIndex;
    });
}

void getReflectData(const SpvReflectShaderModule module, ReflectData& outData) {
  SpvReflectResult result;
  u32 count = 0;
  result = spvReflectEnumerateDescriptorSets(&module, &count, NULL);
  assert(result == SPV_REFLECT_RESULT_SUCCESS);

  outData.reflectDescSets.resize(count);
  result = spvReflectEnumerateDescriptorSets(&module, &count, outData.reflectDescSets.data());
  assert(result == SPV_REFLECT_RESULT_SUCCESS);

  result = spvReflectEnumerateInputVariables(&module, &count, NULL);
  assert(result == SPV_REFLECT_RESULT_SUCCESS);

  outData.inputVars.resize(count);
  result = spvReflectEnumerateInputVariables(&module, &count, outData.inputVars.data());
  assert(result == SPV_REFLECT_RESULT_SUCCESS);

  result = spvReflectEnumerateOutputVariables(&module, &count, NULL);
  assert(result == SPV_REFLECT_RESULT_SUCCESS);

  outData.outputVars.resize(count);
  result = spvReflectEnumerateOutputVariables(&module, &count, outData.outputVars.data());
  assert(result == SPV_REFLECT_RESULT_SUCCESS);
}

// NOTE: This function expects that the DescriptorSetLayoutData is sorted increasingly by the set index
// NOTE: It also assumes that the bindings within the sets are sorted increasingle by the binding index
void mergeDescSetReflectionData(std::vector<DescriptorSetLayoutData>* dataA, std::vector<DescriptorSetLayoutData>* dataB, std::vector<DescriptorSetLayoutData>& out) {

  u32 setsIndexA = 0;
  u32 setsIndexB = 0;
  u32 setsACount = dataA->size();
  u32 setsBCount = dataB->size();
  
  while (setsIndexA < setsACount && setsIndexB < setsBCount) {
    DescriptorSetLayoutData& setLayoutA = dataA->at(setsIndexA);
    DescriptorSetLayoutData& setLayoutB = dataB->at(setsIndexB);
    if (setLayoutA.setIndex < setLayoutB.setIndex) {
      // empty set A into output
      out.push_back(setLayoutA);
      ++setsIndexA;
    }
    else if (setLayoutB.setIndex < setLayoutA.setIndex) {
      // empty set B into output
      out.push_back(setLayoutB);
      ++setsIndexB;
    }
    else {
      // resolve any set/binding descrepancies
      out.emplace_back();
      DescriptorSetLayoutData& setLayout = out.back();
      setLayout.setIndex = setLayoutA.setIndex;

      std::vector<VkDescriptorSetLayoutBinding>& bindings = setLayout.bindings;
      std::vector<VkDescriptorSetLayoutBinding>& bindingsA = setLayoutA.bindings;
      std::vector<VkDescriptorSetLayoutBinding>& bindingsB = setLayoutB.bindings;

      u32 bindingsIndexA = 0;
      u32 bindingsIndexB = 0;
      u32 bindingsACount = bindingsA.size();
      u32 bindingsBCount = bindingsB.size();

      while (bindingsIndexA < bindingsACount && bindingsIndexB < bindingsBCount) {
        VkDescriptorSetLayoutBinding& bindingA = bindingsA[bindingsIndexA];
        VkDescriptorSetLayoutBinding& bindingB = bindingsB[bindingsIndexB];
        if (bindingA.binding < bindingB.binding) {
          // empty set A into output
          bindings.push_back(bindingA);
          ++bindingsIndexA;
        }
        else if (bindingB.binding < bindingA.binding) {
          // empty set B into output
          bindings.push_back(bindingB);
          ++bindingsIndexB;
        }
        else {
          // resolve any set/binding descrepancies
          VkDescriptorSetLayoutBinding binding = bindingA;
          assert(bindingA.descriptorType == bindingB.descriptorType); // ensure there is no conflicted opinions of binding descriptor type
          binding.stageFlags |= bindingB.stageFlags;
          bindings.push_back(binding);

          ++bindingsIndexA;
          ++bindingsIndexB;
        }
      }

      if (bindingsIndexA < bindingsACount) {
        // empty set A into output
        bindings.insert(bindings.end(), bindingsA.begin() + bindingsIndexA, bindingsA.end());
      }

      if (bindingsIndexB < bindingsBCount) {
        // empty set B into output
        bindings.insert(bindings.end(), bindingsB.begin() + bindingsIndexB, bindingsB.end());
      }

      ++setsIndexA;
      ++setsIndexB;
    }
  }

  if (setsIndexA < setsACount) {
    // empty set A into output
    out.insert(out.end(), dataA->begin() + setsIndexA, dataA->end());
  }

  if (setsIndexB < setsBCount) {
    // empty set B into output
    out.insert(out.end(), dataB->begin() + setsIndexB, dataB->end());
  }
}

int main(int argn, char** argv)
{
  ShaderMetaData shaderMetaData;

  std::vector<char> vertShaderBytes, fragShaderBytes;
  readFile(materialTextured.vertFileName, vertShaderBytes);
  readFile(materialTextured.fragFileName, fragShaderBytes);

  SpvReflectShaderModule vertModule = {}, fragModule = {};
  SpvReflectResult result = spvReflectCreateShaderModule(vertShaderBytes.size(), vertShaderBytes.data(), &vertModule);
  assert(result == SPV_REFLECT_RESULT_SUCCESS);
  result = spvReflectCreateShaderModule(fragShaderBytes.size(), fragShaderBytes.data(), &fragModule);
  assert(result == SPV_REFLECT_RESULT_SUCCESS);

  ReflectData vertReflectData, fragReflectData;
  getReflectData(vertModule, vertReflectData);
  getReflectData(fragModule, fragReflectData);

  // Descriptor Set Layout Data
  std::vector<DescriptorSetLayoutData> vertDescSets, fragDescSets;
  getDescriptorSetLayoutData(vertModule, vertReflectData, vertDescSets);
  getDescriptorSetLayoutData(fragModule, fragReflectData, fragDescSets);

  mergeDescSetReflectionData(&vertDescSets, &fragDescSets, shaderMetaData.descSetLayouts);

  // TODO: a real application would be merged with similar structures from other shader stages and/or pipelines
  // to create a VkPipelineLayout.

  // Vertex Attribute Input Variables
  assert(vertModule.shader_stage == SPV_REFLECT_SHADER_STAGE_VERTEX_BIT);

  // populate VkPipelineVertexInputStateCreateInfo structure, given the module's
  // expected input variables.
  //
  // Simplifying assumptions:
  // - The format of each attribute matches its usage in the shader;
  //   float4 -> VK_FORMAT_R32G32B32A32_FLOAT, etc. No attribute compression is applied.
  ShaderInputOutputMetaData& inputOutputMetadata = shaderMetaData.inputOutputMetaData;

  VkVertexInputBindingDescription& vertexInputBindingDescription = inputOutputMetadata.vertexInputDesc.bindingDesc;
  vertexInputBindingDescription = {};
  vertexInputBindingDescription.binding = 0; // All vertex input attributes bound to slot 0
  vertexInputBindingDescription.stride = 0;  // computed below
  vertexInputBindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;  // All attributes are provided per-vertex, not per-instance.

  u64 inputAttributeCount = vertReflectData.inputVars.size();
  std::vector<VkVertexInputAttributeDescription>& inputAttrDescriptions = inputOutputMetadata.vertexInputDesc.attributes;
  inputAttrDescriptions.reserve(inputAttributeCount);
  for (u64 varIndex = 0; varIndex < inputAttributeCount; ++varIndex) {
    const SpvReflectInterfaceVariable& reflInterfaceVar = *(vertReflectData.inputVars[varIndex]);
    if (reflInterfaceVar.decoration_flags & SPV_REFLECT_DECORATION_BUILT_IN) { 
      continue; // ignore built in variables
    } 
    VkVertexInputAttributeDescription attrDescription;
    attrDescription.location = reflInterfaceVar.location;
    attrDescription.binding = vertexInputBindingDescription.binding;
    attrDescription.format = static_cast<VkFormat>(reflInterfaceVar.format);
    attrDescription.offset = 0;  // final offset computed below after sorting.
    inputAttrDescriptions.push_back(attrDescription);
  }

  // Each vertex's attribute are laid out in ascending order by location
  std::sort(inputAttrDescriptions.begin(), inputAttrDescriptions.end(),
    [](const VkVertexInputAttributeDescription& a, const VkVertexInputAttributeDescription& b) {
      return a.location < b.location; 
    });

  // Compute final offsets of each attribute, and total vertex stride.
  for (VkVertexInputAttributeDescription& attribute : inputAttrDescriptions) {
    u32 formatSize = FormatSize(attribute.format);
    attribute.offset = vertexInputBindingDescription.stride;
    vertexInputBindingDescription.stride += formatSize;
  }
  // Nothing further is done with attribute_descriptions or binding_description
  // in this sample. A real application would probably derive this information from its
  // mesh format(s); a similar mechanism could be used to ensure mesh/shader compatibility.

  // Fragment Input Variables
  Assert(fragModule.shader_stage == SPV_REFLECT_SHADER_STAGE_FRAGMENT_BIT);

  spvReflectDestroyShaderModule(&vertModule);
  spvReflectDestroyShaderModule(&fragModule);

  return 0;
}