#include "../vk_study.h"

#include "testing_util.h"

#include <unordered_map>

struct DescriptorSetLayoutData {
  uint32_t setNumber;
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

void getDescriptorSetLayoutData(const SpvReflectShaderModule& module, const ReflectData& data, std::vector <DescriptorSetLayoutData>& outData) {
  u64 descSetCount = data.reflectDescSets.size();
  outData.resize(descSetCount, DescriptorSetLayoutData{});
  for (u64 setIndex = 0; setIndex < descSetCount; ++setIndex) {
    const SpvReflectDescriptorSet& reflectDescSet = *(data.reflectDescSets[setIndex]);
    DescriptorSetLayoutData& layout = outData[setIndex];

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
    layout.setNumber = reflectDescSet.set;
  }
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

// TODO: THIS IS THE WORST FUNCTION I HAVE EVER WRITTEN. 
// CONSIDER REFACTORING IMMEDIATELY
void mergeDescSetReflectionData(std::vector<DescriptorSetLayoutData>& setsA, std::vector<DescriptorSetLayoutData>& setsB, std::vector<DescriptorSetLayoutData>& out) {
  // concatenate both sets to iterate through single list
  out.reserve(setsA.size() + setsB.size());
  out.insert(out.end(), setsA.begin(), setsA.end());
  out.insert(out.end(), setsB.begin(), setsB.end());

  // A hash function used to hash a pair of any kind
  struct hash_pair {
    size_t operator()(const std::pair<u32, u32>& pair) const
    {
      if (pair.first != pair.first) {
        return pair.first < pair.first;
      }
      else {
        return pair.second < pair.second;
      }
    }
  };

  // Merge all bindings with unique <set index, binding index> pair
  std::unordered_map<std::pair<u32, u32>, VkDescriptorSetLayoutBinding, hash_pair> setBindings;
  
  for (const DescriptorSetLayoutData& layoutData : out) {
    for (const VkDescriptorSetLayoutBinding& binding : layoutData.bindings) {
      std::pair<u32, u32> setBindingKey(layoutData.setNumber, binding.binding);
      if (setBindings.find(setBindingKey) == setBindings.end()) { // not found
        VkDescriptorSetLayoutBinding newBinding = vkinit::descriptorSetLayoutBinding(
          binding.descriptorType,
          binding.stageFlags,
          binding.binding);
        setBindings[setBindingKey] = newBinding;
      }
      else { // found
        VkDescriptorSetLayoutBinding& updateBinding = setBindings[setBindingKey];
        assert(updateBinding.descriptorType == binding.descriptorType); // ensure there is no conflicted opinions of binding descriptor type
        updateBinding.stageFlags |= binding.stageFlags;
      }
    }
  }
  
  // Group all bindings under the same set index
  std::unordered_map<u32, DescriptorSetLayoutData> sets;
  for (std::pair<std::pair<u32, u32>, VkDescriptorSetLayoutBinding> setBinding : setBindings) {
    u32 setIndex = setBinding.first.first;
    VkDescriptorSetLayoutBinding& descSetLayoutBinding = setBinding.second;
    if (sets.find(setIndex) == sets.end()) { // not found
      DescriptorSetLayoutData newSetLayoutData{};
      newSetLayoutData.setNumber = setIndex;
      newSetLayoutData.bindings.push_back(descSetLayoutBinding);
      sets[setIndex] = newSetLayoutData;
    }
    else { // found
      DescriptorSetLayoutData& updateSetLayoutData = sets[setIndex];
      updateSetLayoutData.bindings.push_back(descSetLayoutBinding);
    }
  }

  out.clear();
  for (std::pair<u32, DescriptorSetLayoutData> set : sets) {
    DescriptorSetLayoutData descSetLayoutData = set.second;
    // sort by binding index
    std::sort(descSetLayoutData.bindings.begin(), descSetLayoutData.bindings.end(),
      [](const VkDescriptorSetLayoutBinding& a, const VkDescriptorSetLayoutBinding& b) {
        assert(a.binding != b.binding); // bindings should never be the same
        return a.binding < b.binding;
      });
    out.push_back(descSetLayoutData);
  }

  // sort by set index
  std::sort(out.begin(), out.end(),
    [](const DescriptorSetLayoutData& a, const DescriptorSetLayoutData& b) {
      assert(a.setNumber != b.setNumber); // bindings should never be the same
      return a.setNumber < b.setNumber;
    });
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
  // TODO: merge descriptor set layouts
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
  
  // TODO: fuse descriptor set reflection data together
  mergeDescSetReflectionData(vertDescSets, fragDescSets, shaderMetaData.descSetLayouts);

  spvReflectDestroyShaderModule(&vertModule);
  spvReflectDestroyShaderModule(&fragModule);

  return 0;
}