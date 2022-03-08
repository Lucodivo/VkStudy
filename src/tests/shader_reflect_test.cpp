#include "test.h"
#include "../vk_study.h"

class VulkanTest : public testing::Test {
protected:
  // run immediately before a test starts
  void SetUp() override {
    vkb::InstanceBuilder builder;

    vkb::Instance vkbInst = builder.set_app_name("Vulkan Tests")
            .request_validation_layers(true)
            .require_api_version(1, 1, 0)
            .use_default_debug_messenger()
            .build().value();

    instance = vkbInst.instance;
    debugMessenger = vkbInst.debug_messenger;

    vkb::PhysicalDeviceSelector selector{vkbInst};
    vkb::PhysicalDevice physicalDevice = selector.set_minimum_version(1, 1)
            .defer_surface_initialization()
            .select().value();

    vkb::DeviceBuilder deviceBuilder{physicalDevice};

    vkb::Device vkbDevice = deviceBuilder.build().value();

    device = vkbDevice.device;
  }

  // invoked immediately after a test finishes
  void TearDown() override {
    vkDestroyDevice(device, nullptr);
    vkb::destroy_debug_utils_messenger(instance, debugMessenger);
    vkDestroyInstance(instance, nullptr);
  }

  VkInstance instance;
  VkDevice device;
  VkDebugUtilsMessengerEXT debugMessenger;
};

class MaterialManagerTest : public VulkanTest {
protected:
  // run immediately before a test starts
  void SetUp() override {
    VulkanTest::SetUp();

    for(u32 i = 0; i < ArrayCount(preloadedMaterialData); i++) {
      PreloadedMaterialData& preloadMatData = preloadedMaterialData[i];
      matManager.loadShaderMetadata(device,
                                    preloadMatData.createInfo.vertFileName,
                                    preloadMatData.createInfo.fragFileName,
                                    preloadMatData.metadata);
    }
  }

  // invoked immediately after a test finishes
  void TearDown() override {
    // MaterialManagerTest tear down
    matManager.destroyAll(device);

    // super teardown
    VulkanTest::TearDown();
  }

  // NOTE: This preloaded data is to remain constant, changes will mess with tests
  struct PreloadedMaterialData {
    MaterialCreateInfo createInfo;
    ShaderMetadata metadata;
  } preloadedMaterialData[6] = {
          // three that use the same vertex shader
          {materialBlue,          {}},
          {materialRed,           {}},
          {materialGreen,         {}},
          // two that use the same fragment shader
          {materialNormalAsColor, {}},
          {materialVertexColor,   {}},
          // One unique
          {materialTextured,      {}},
  };
  MaterialManager matManager;
};

// TEST_F is for using "fixtures"
TEST_F(MaterialManagerTest, CacheTest) {
  // assert cache sizes are as expected
  ASSERT_EQ(matManager.cachedVertShaders.size(), 4);
  ASSERT_EQ(matManager.cachedFragShaders.size(), 5);
  ASSERT_EQ(matManager.cachedShaders.size(), 6);

  // assert same modules are used as expected
  // blue, red, green shared vert shader
  ASSERT_EQ(preloadedMaterialData[0].metadata.vertModule, preloadedMaterialData[1].metadata.vertModule);
  ASSERT_EQ(preloadedMaterialData[0].metadata.vertModule, preloadedMaterialData[2].metadata.vertModule);
  // normal/vertex color shared frag shader
  ASSERT_EQ(preloadedMaterialData[3].metadata.fragModule, preloadedMaterialData[4].metadata.fragModule);

  ShaderMetadata repeatShaderMetadata;
  const MaterialCreateInfo& repeatCreateInfo = preloadedMaterialData[5].createInfo;
  const ShaderMetadata& expectedShaderMetadata = preloadedMaterialData[5].metadata;
  matManager.loadShaderMetadata(device,
                                repeatCreateInfo.vertFileName,
                                repeatCreateInfo.fragFileName,
                                repeatShaderMetadata);
  ASSERT_EQ(repeatShaderMetadata.vertModule, expectedShaderMetadata.vertModule);
  ASSERT_EQ(repeatShaderMetadata.fragModule, expectedShaderMetadata.fragModule);

  // assert that description set layouts are exact
  ASSERT_EQ(repeatShaderMetadata.descSetLayouts.size(), expectedShaderMetadata.descSetLayouts.size());
  u32 descSetLayoutSize = static_cast<u32>(repeatShaderMetadata.descSetLayouts.size());
  for(u32 i = 0; i < descSetLayoutSize; i++) {
    const DescriptorSetLayoutData& repeatDescSetLayoutData = repeatShaderMetadata.descSetLayouts[i];
    const DescriptorSetLayoutData& expectedDescSetLayoutData = expectedShaderMetadata.descSetLayouts[i];
    ASSERT_EQ(repeatDescSetLayoutData.setIndex, expectedDescSetLayoutData.setIndex);

    // assert each binding is exact
    ASSERT_EQ(repeatDescSetLayoutData.bindings.size(), expectedDescSetLayoutData.bindings.size());
    u32 bindingCount = static_cast<u32>(repeatDescSetLayoutData.bindings.size());
    for(u32 j = 0; j < bindingCount; j++) {
      const VkDescriptorSetLayoutBinding& repeatDescSetLayoutBinding = repeatDescSetLayoutData.bindings[j];
      const VkDescriptorSetLayoutBinding& expectedDescSetLayoutBinding = expectedDescSetLayoutData.bindings[j];
      ASSERT_EQ(repeatDescSetLayoutBinding.binding, expectedDescSetLayoutBinding.binding);
      ASSERT_EQ(repeatDescSetLayoutBinding.descriptorType, expectedDescSetLayoutBinding.descriptorType);
      ASSERT_EQ(repeatDescSetLayoutBinding.descriptorCount, expectedDescSetLayoutBinding.descriptorCount);
      ASSERT_EQ(repeatDescSetLayoutBinding.stageFlags, expectedDescSetLayoutBinding.stageFlags);
      ASSERT_EQ(repeatDescSetLayoutBinding.pImmutableSamplers, expectedDescSetLayoutBinding.pImmutableSamplers);
    }
  }

  // assert that push constants are exact
  ASSERT_EQ(repeatShaderMetadata.pushConstantRanges.size(), expectedShaderMetadata.pushConstantRanges.size());
  u32 pushConstantSize = static_cast<u32>(repeatShaderMetadata.pushConstantRanges.size());
  for(u32 i = 0; i < pushConstantSize; i++) {
    const VkPushConstantRange& repeatPushConstantRange = repeatShaderMetadata.pushConstantRanges[i];
    const VkPushConstantRange& expectedPushConstantRange = expectedShaderMetadata.pushConstantRanges[i];
    ASSERT_EQ(repeatPushConstantRange.stageFlags, expectedPushConstantRange.stageFlags);
    ASSERT_EQ(repeatPushConstantRange.offset, expectedPushConstantRange.offset);
    ASSERT_EQ(repeatPushConstantRange.size, expectedPushConstantRange.size);
  }

  // assert that ShaderInputMetadata are exact
  {
    const VkVertexInputBindingDescription& repeatVertexInputVertexDesc = repeatShaderMetadata.input.vertexInputDesc.bindingDesc;
    const VkVertexInputBindingDescription& expectedVertexBindingDesc = expectedShaderMetadata.input.vertexInputDesc.bindingDesc;
    ASSERT_EQ(repeatVertexInputVertexDesc.binding, expectedVertexBindingDesc.binding);
    ASSERT_EQ(repeatVertexInputVertexDesc.stride, expectedVertexBindingDesc.stride);
    ASSERT_EQ(repeatVertexInputVertexDesc.inputRate, expectedVertexBindingDesc.inputRate);


    const std::vector<VkVertexInputAttributeDescription>& repeatVertexAttDescs = repeatShaderMetadata.input.vertexInputDesc.attributes;
    const std::vector<VkVertexInputAttributeDescription>& expectedVertexAttDescs = expectedShaderMetadata.input.vertexInputDesc.attributes;
    ASSERT_EQ(repeatVertexAttDescs.size(), expectedVertexAttDescs.size());
    u32 vertAttDescCount = static_cast<u32>(repeatVertexAttDescs.size());
    for(u32 i = 0; i < vertAttDescCount; i++) {
      const VkVertexInputAttributeDescription& repeatVertexAttDesc = repeatVertexAttDescs[i];
      const VkVertexInputAttributeDescription& expectedVertexAttDesc = expectedVertexAttDescs[i];
      ASSERT_EQ(repeatVertexAttDesc.location, expectedVertexAttDesc.location);
      ASSERT_EQ(repeatVertexAttDesc.binding, expectedVertexAttDesc.binding);
      ASSERT_EQ(repeatVertexAttDesc.format, expectedVertexAttDesc.format);
      ASSERT_EQ(repeatVertexAttDesc.offset, expectedVertexAttDesc.offset);
    }
  }

  // assert that no new shaders have been introduced
  ASSERT_EQ(matManager.cachedVertShaders.size(), 4);
  ASSERT_EQ(matManager.cachedFragShaders.size(), 5);
  ASSERT_EQ(matManager.cachedShaders.size(), 6);
}