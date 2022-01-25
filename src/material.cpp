VkDescriptorSetLayoutCreateInfo DescriptorSetLayoutData::descriptorSetLayoutCreateInfo() {
	VkDescriptorSetLayoutCreateInfo descriptorSetLayoutCreateInfo{};
	descriptorSetLayoutCreateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
	descriptorSetLayoutCreateInfo.pNext = nullptr;
	descriptorSetLayoutCreateInfo.flags = 0;
	descriptorSetLayoutCreateInfo.bindingCount = static_cast<u32>(bindings.size());
	descriptorSetLayoutCreateInfo.pBindings = bindings.data();
	return descriptorSetLayoutCreateInfo;
}

VkPipelineVertexInputStateCreateInfo ShaderInputMetadata::vertexInputStateCreateInfo() {
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

// Returns a vector of descriptor set layouts which group together bindings per set
// These sets will be returned in increasing order based on their set index
// the bindings within the set will be returned in increasing order based on their binding index
void MaterialManager::getDescriptorSetLayoutData(const SpvReflectShaderModule& module, 
                                                  const ReflectData& data, 
                                                  std::vector<DescriptorSetLayoutData>& outData) {
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

      // Descriptor count is affected if the descriptor is an array
      // for a multidimensional array, count is the product of all dimensions
      for (u32 iArrayDimension = 0; iArrayDimension < reflectBinding.array.dims_count; ++iArrayDimension) {
        layoutBinding.descriptorCount *= reflectBinding.array.dims[iArrayDimension];
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

void MaterialManager::getReflectData(const SpvReflectShaderModule module, ReflectData& outData) {
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
void MaterialManager::mergeDescSetReflectionData(const std::vector<DescriptorSetLayoutData>& dataA, const std::vector<DescriptorSetLayoutData>& dataB, std::vector<DescriptorSetLayoutData>& out) {

  u32 setsIndexA = 0;
  u32 setsIndexB = 0;
  u32 setsACount = static_cast<u32>(dataA.size());
  u32 setsBCount = static_cast<u32>(dataB.size());

  while (setsIndexA < setsACount && setsIndexB < setsBCount) {
    const DescriptorSetLayoutData& setLayoutA = dataA[setsIndexA];
    const DescriptorSetLayoutData& setLayoutB = dataB[setsIndexB];
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
      const std::vector<VkDescriptorSetLayoutBinding>& bindingsA = setLayoutA.bindings;
      const std::vector<VkDescriptorSetLayoutBinding>& bindingsB = setLayoutB.bindings;

      u32 bindingsIndexA = 0;
      u32 bindingsIndexB = 0;
      u32 bindingsACount = static_cast<u32>(bindingsA.size());
      u32 bindingsBCount = static_cast<u32>(bindingsB.size());

      while (bindingsIndexA < bindingsACount && bindingsIndexB < bindingsBCount) {
        const VkDescriptorSetLayoutBinding& bindingA = bindingsA[bindingsIndexA];
        const VkDescriptorSetLayoutBinding& bindingB = bindingsB[bindingsIndexB];
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
    out.insert(out.end(), dataA.begin() + setsIndexA, dataA.end());
  }

  if (setsIndexB < setsBCount) {
    // empty set B into output
    out.insert(out.end(), dataB.begin() + setsIndexB, dataB.end());
  }
}

void MaterialManager::destroyAll(VkDevice device) {
  for (std::pair<std::string, VertexShaderMetadata> pair : cachedVertShaders) {
    VkShaderModule shaderModule = pair.second.module;
    if (shaderModule != VK_NULL_HANDLE) { // null handles used for combination shader
      vkDestroyShaderModule(device, shaderModule, nullptr);
    }
  }
  for (std::pair<std::string, FragmentShaderMetadata> pair : cachedFragShaders) {
    VkShaderModule shaderModule = pair.second.module;
    if (shaderModule != VK_NULL_HANDLE) { // null handles used for combination shader
      vkDestroyShaderModule(device, shaderModule, nullptr);
    }
  }
  cachedVertShaders.clear();
  cachedFragShaders.clear();
  cachedShaders.clear();
}

void MaterialManager::loadShaderMetadata(VkDevice device, const char* vertFileName, const char* fragFileName, ShaderMetadata& out)
{
  VertexShaderMetadata vertShader;
  FragmentShaderMetadata fragShader;
  bool vertShaderCached, fragShaderCached, combinationShaderCached = false;
  vertShaderCached = cachedVertShaders.find(vertFileName) != cachedVertShaders.end();
  fragShaderCached = cachedFragShaders.find(fragFileName) != cachedFragShaders.end();

  if (vertShaderCached) {
    vertShader = cachedVertShaders[vertFileName];
  } else {
    std::vector<char> vertShaderBytes;
    vkutil::loadShaderBuffer(vertFileName, vertShaderBytes);

    vertShader.module = vkutil::loadShaderModule(device, vertShaderBytes);
    if(vertShader.module == VK_NULL_HANDLE) { return; }

    SpvReflectShaderModule vertReflModule{};
    SpvReflectResult result = spvReflectCreateShaderModule(vertShaderBytes.size(), vertShaderBytes.data(), &vertReflModule);
    Assert(result == SPV_REFLECT_RESULT_SUCCESS);

    ReflectData vertReflectData;
    getReflectData(vertReflModule, vertReflectData);

    Assert(vertReflModule.shader_stage == SPV_REFLECT_SHADER_STAGE_VERTEX_BIT);

    getDescriptorSetLayoutData(vertReflModule, vertReflectData, vertShader.descSetLayouts);

    // TODO: a real application would be merged with similar structures from other shader stages and/or pipelines
    // to create a VkPipelineLayout.

    // Vertex Attribute Input Variables

    // populate VkPipelineVertexInputStateCreateInfo structure, given the module's
    // expected input variables.
    //
    // Simplifying assumptions:
    // - The format of each attribute matches its usage in the shader;
    //   float4 -> VK_FORMAT_R32G32B32A32_FLOAT, etc. No attribute compression is applied.
    ShaderInputMetadata& inputOutputMetadata = vertShader.input;

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
      u32 formatSize = vkutil::formatSize(attribute.format);
      attribute.offset = vertexInputBindingDescription.stride;
      vertexInputBindingDescription.stride += formatSize;
    }
    // Nothing further is done with attribute_descriptions or binding_description
    // in this sample. A real application would probably derive this information from its
    // mesh format(s); a similar mechanism could be used to ensure mesh/shader compatibility.

    // cache vert shader metadata
    cachedVertShaders[vertFileName] = vertShader;

    spvReflectDestroyShaderModule(&vertReflModule);
  }

  if (fragShaderCached) {
    fragShader = cachedFragShaders[fragFileName];
  } else {
    std::vector<char> fragShaderBytes;
    vkutil::loadShaderBuffer(fragFileName, fragShaderBytes);

    fragShader.module = vkutil::loadShaderModule(device, fragShaderBytes);
    if(fragShader.module == VK_NULL_HANDLE) { return; }

    SpvReflectShaderModule fragReflModule{};
    SpvReflectResult result = spvReflectCreateShaderModule(fragShaderBytes.size(), fragShaderBytes.data(), &fragReflModule);
    assert(result == SPV_REFLECT_RESULT_SUCCESS);
    ReflectData fragReflectData;
    getReflectData(fragReflModule, fragReflectData);

    Assert(fragReflModule.shader_stage == SPV_REFLECT_SHADER_STAGE_FRAGMENT_BIT);

    // Descriptor Set Layout Data
    getDescriptorSetLayoutData(fragReflModule, fragReflectData, fragShader.descSetLayouts);

    // cache frag shader metadata
    cachedFragShaders[fragFileName] = fragShader;

    spvReflectDestroyShaderModule(&fragReflModule);
  }

  const std::string vertFragShaderKey = std::string(vertFileName) + fragFileName;
  if (vertShaderCached && fragShaderCached) {
    combinationShaderCached = cachedShaders.find(vertFragShaderKey) != cachedShaders.end();

    if (combinationShaderCached) {
      out = cachedShaders[vertFragShaderKey];
    }
  }
  else {
    mergeReflectionData(vertShader, fragShader, out);

    // cache frag shader metadata
    cachedShaders[vertFragShaderKey] = out;
  }
}

void MaterialManager::mergeReflectionData(const VertexShaderMetadata& vertShader, const FragmentShaderMetadata& fragShader, ShaderMetadata& outShader)
{
  outShader.vertModule = vertShader.module;
  outShader.fragModule = fragShader.module;
  outShader.input = vertShader.input;
  mergeDescSetReflectionData(vertShader.descSetLayouts, fragShader.descSetLayouts, outShader.descSetLayouts);
}
