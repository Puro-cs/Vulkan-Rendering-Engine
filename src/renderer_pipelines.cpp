/* Copyright (c) 2025 Holochip Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Licensed under the Apache License, Version 2.0 the "License";
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
#include "mesh_component.h"
#include "renderer.h"
#include <array>
#include <fstream>
#include <iostream>

// This file contains pipeline-related methods from the Renderer class

// Create a descriptor set layout
bool Renderer::createDescriptorSetLayout() {
  try {
    // Create binding for a uniform buffer
    vk::DescriptorSetLayoutBinding uboLayoutBinding{
      .binding = 0,
      .descriptorType = vk::DescriptorType::eUniformBuffer,
      .descriptorCount = 1,
      .stageFlags = vk::ShaderStageFlagBits::eVertex | vk::ShaderStageFlagBits::eFragment,
      .pImmutableSamplers = nullptr
    };

    // Create binding for texture sampler
    vk::DescriptorSetLayoutBinding samplerLayoutBinding{
      .binding = 1,
      .descriptorType = vk::DescriptorType::eCombinedImageSampler,
      .descriptorCount = 1,
      .stageFlags = vk::ShaderStageFlagBits::eFragment,
      .pImmutableSamplers = nullptr
    };

    // Create a descriptor set layout
    std::array<vk::DescriptorSetLayoutBinding, 2> bindings = {uboLayoutBinding, samplerLayoutBinding};

    // Descriptor indexing: set per-binding flags for UPDATE_AFTER_BIND if enabled
    vk::DescriptorSetLayoutBindingFlagsCreateInfo bindingFlagsInfo{};
    std::array<vk::DescriptorBindingFlags, 2> bindingFlags{};
    if (descriptorIndexingEnabled) {
      if (descriptorBindingUniformBufferUpdateAfterBindEnabled) {
        bindingFlags[0] = vk::DescriptorBindingFlagBits::eUpdateAfterBind | vk::DescriptorBindingFlagBits::eUpdateUnusedWhilePending;
      }
      if (descriptorBindingSampledImageUpdateAfterBindEnabled) {
        bindingFlags[1] = vk::DescriptorBindingFlagBits::eUpdateAfterBind | vk::DescriptorBindingFlagBits::eUpdateUnusedWhilePending;
      }
      bindingFlagsInfo.bindingCount = static_cast<uint32_t>(bindingFlags.size());
      bindingFlagsInfo.pBindingFlags = bindingFlags.data();
    }

    vk::DescriptorSetLayoutCreateInfo layoutInfo{};
    layoutInfo.bindingCount = static_cast<uint32_t>(bindings.size());
    layoutInfo.pBindings = bindings.data();
    if (descriptorIndexingEnabled) {
      layoutInfo.flags |= vk::DescriptorSetLayoutCreateFlagBits::eUpdateAfterBindPool;
      layoutInfo.pNext = &bindingFlagsInfo;
    }

    descriptorSetLayout = vk::raii::DescriptorSetLayout(device, layoutInfo);
    return true;
  } catch (const std::exception& e) {
    std::cerr << "Failed to create descriptor set layout: " << e.what() << std::endl;
    return false;
  }
}

// Create PBR descriptor set layout
bool Renderer::createPBRDescriptorSetLayout() {
  try {
    // Create descriptor set layout bindings for PBR shader
    std::array baseBindings = {
      // Binding 0: Uniform buffer (UBO)
      vk::DescriptorSetLayoutBinding{
        .binding = 0,
        .descriptorType = vk::DescriptorType::eUniformBuffer,
        .descriptorCount = 1,
        .stageFlags = vk::ShaderStageFlagBits::eVertex | vk::ShaderStageFlagBits::eFragment,
        .pImmutableSamplers = nullptr
      },
      // Binding 1: Base color map and sampler
      vk::DescriptorSetLayoutBinding{
        .binding = 1,
        .descriptorType = vk::DescriptorType::eCombinedImageSampler,
        .descriptorCount = 1,
        .stageFlags = vk::ShaderStageFlagBits::eFragment,
        .pImmutableSamplers = nullptr
      },
      // Binding 2: Metallic roughness map and sampler
      vk::DescriptorSetLayoutBinding{
        .binding = 2,
        .descriptorType = vk::DescriptorType::eCombinedImageSampler,
        .descriptorCount = 1,
        .stageFlags = vk::ShaderStageFlagBits::eFragment,
        .pImmutableSamplers = nullptr
      },
      // Binding 3: Normal map and sampler
      vk::DescriptorSetLayoutBinding{
        .binding = 3,
        .descriptorType = vk::DescriptorType::eCombinedImageSampler,
        .descriptorCount = 1,
        .stageFlags = vk::ShaderStageFlagBits::eFragment,
        .pImmutableSamplers = nullptr
      },
      // Binding 4: Occlusion map and sampler
      vk::DescriptorSetLayoutBinding{
        .binding = 4,
        .descriptorType = vk::DescriptorType::eCombinedImageSampler,
        .descriptorCount = 1,
        .stageFlags = vk::ShaderStageFlagBits::eFragment,
        .pImmutableSamplers = nullptr
      },
      // Binding 5: Emissive map and sampler
      vk::DescriptorSetLayoutBinding{
        .binding = 5,
        .descriptorType = vk::DescriptorType::eCombinedImageSampler,
        .descriptorCount = 1,
        .stageFlags = vk::ShaderStageFlagBits::eFragment,
        .pImmutableSamplers = nullptr
      }
    };

    std::vector<vk::DescriptorSetLayoutBinding> bindings(baseBindings.begin(), baseBindings.end());

    // Binding 6: lights SSBO
    bindings.push_back(vk::DescriptorSetLayoutBinding{
      .binding = 6,
        .descriptorType = vk::DescriptorType::eStorageBuffer,
        .descriptorCount = 1,
        .stageFlags = vk::ShaderStageFlagBits::eFragment,
        .pImmutableSamplers = nullptr
    });

    // Create a descriptor set layout
    // Descriptor indexing: set per-binding flags for UPDATE_AFTER_BIND on UBO (0) and sampled images (1..5)
    vk::DescriptorSetLayoutBindingFlagsCreateInfo bindingFlagsInfo{};
    std::vector<vk::DescriptorBindingFlags> bindingFlags(bindings.size(), vk::DescriptorBindingFlags{});
    if (descriptorIndexingEnabled) {
      for (size_t i = 0; i < bindings.size(); ++i) {
        if (bindings[i].descriptorType == vk::DescriptorType::eUniformBuffer && descriptorBindingUniformBufferUpdateAfterBindEnabled) {
          bindingFlags[i] = vk::DescriptorBindingFlagBits::eUpdateAfterBind | vk::DescriptorBindingFlagBits::eUpdateUnusedWhilePending;
        } else if (bindings[i].descriptorType == vk::DescriptorType::eCombinedImageSampler && descriptorBindingSampledImageUpdateAfterBindEnabled) {
          bindingFlags[i] = vk::DescriptorBindingFlagBits::eUpdateAfterBind | vk::DescriptorBindingFlagBits::eUpdateUnusedWhilePending;
        }
      }
      bindingFlagsInfo.bindingCount = static_cast<uint32_t>(bindingFlags.size());
      bindingFlagsInfo.pBindingFlags = bindingFlags.data();
    }

    vk::DescriptorSetLayoutCreateInfo layoutInfo{};
    layoutInfo.bindingCount = static_cast<uint32_t>(bindings.size());
    layoutInfo.pBindings = bindings.data();
    if (descriptorIndexingEnabled) {
      layoutInfo.flags |= vk::DescriptorSetLayoutCreateFlagBits::eUpdateAfterBindPool;
      layoutInfo.pNext = &bindingFlagsInfo;
    }

    pbrDescriptorSetLayout = vk::raii::DescriptorSetLayout(device, layoutInfo);

    // Binding 7: transparent passes input
    // Layout for Set 1: Just the scene color texture
    vk::DescriptorSetLayoutBinding sceneColorBinding{
      .binding = 0, .descriptorType = vk::DescriptorType::eCombinedImageSampler, .descriptorCount = 1, .stageFlags = vk::ShaderStageFlagBits::eFragment
    };
    vk::DescriptorSetLayoutCreateInfo transparentLayoutInfo{.bindingCount = 1, .pBindings = &sceneColorBinding};
    if (descriptorIndexingEnabled && descriptorBindingSampledImageUpdateAfterBindEnabled) {
      // Make this sampler binding update-after-bind safe as well (optional)
      vk::DescriptorSetLayoutBindingFlagsCreateInfo transBindingFlagsInfo{};
      vk::DescriptorBindingFlags transFlags = vk::DescriptorBindingFlagBits::eUpdateAfterBind | vk::DescriptorBindingFlagBits::eUpdateUnusedWhilePending;
      transBindingFlagsInfo.bindingCount = 1;
      transBindingFlagsInfo.pBindingFlags = &transFlags;
      transparentLayoutInfo.flags |= vk::DescriptorSetLayoutCreateFlagBits::eUpdateAfterBindPool;
      transparentLayoutInfo.pNext = &transBindingFlagsInfo;

      // Create the layout while the pNext chain is still valid (avoid dangling pointer)
      transparentDescriptorSetLayout = vk::raii::DescriptorSetLayout(device, transparentLayoutInfo);
    } else {
      // Create without extra binding flags
      transparentDescriptorSetLayout = vk::raii::DescriptorSetLayout(device, transparentLayoutInfo);
    }

    return true;
  } catch (const std::exception& e) {
    std::cerr << "Failed to create PBR descriptor set layout: " << e.what() << std::endl;
    return false;
  }
}

// Create a graphics pipeline
bool Renderer::createGraphicsPipeline() {
  try {
    // Read shader code (Reverted to use the Slang-compiled texturedMesh shader)
    auto shaderCode = readFile("shaders/texturedMesh.spv");

    // Create shader module
    vk::raii::ShaderModule shaderModule = createShaderModule(shaderCode);

    // Create shader stage info
    vk::PipelineShaderStageCreateInfo vertShaderStageInfo{};
    vertShaderStageInfo.sType = vk::StructureType::ePipelineShaderStageCreateInfo;
    vertShaderStageInfo.stage = vk::ShaderStageFlagBits::eVertex;
    vertShaderStageInfo.module = *shaderModule;
    vertShaderStageInfo.pName = "VSMain";

    vk::PipelineShaderStageCreateInfo fragShaderStageInfo{};
    fragShaderStageInfo.sType = vk::StructureType::ePipelineShaderStageCreateInfo;
    fragShaderStageInfo.stage = vk::ShaderStageFlagBits::eFragment;
    fragShaderStageInfo.module = *shaderModule;
    fragShaderStageInfo.pName = "PSMain";

    vk::PipelineShaderStageCreateInfo shaderStages[] = {vertShaderStageInfo, fragShaderStageInfo};

    // Create vertex input info with instancing support
    auto vertexBindingDescription = Vertex::getBindingDescription();
    auto instanceBindingDescription = InstanceData::getBindingDescription();
    std::array<vk::VertexInputBindingDescription, 2> bindingDescriptions = {
      vertexBindingDescription,
      instanceBindingDescription
    };

    auto vertexAttributeDescriptions = Vertex::getAttributeDescriptions();
    auto instanceAttributeDescriptions = InstanceData::getAttributeDescriptions();

    // Combine all attribute descriptions (no duplicates)
    std::vector<vk::VertexInputAttributeDescription> allAttributeDescriptions;
    allAttributeDescriptions.insert(allAttributeDescriptions.end(), vertexAttributeDescriptions.begin(), vertexAttributeDescriptions.end());
    allAttributeDescriptions.insert(allAttributeDescriptions.end(), instanceAttributeDescriptions.begin(), instanceAttributeDescriptions.end());

    // Note: materialIndex attribute (Location 11) is not used by current shaders

    vk::PipelineVertexInputStateCreateInfo vertexInputInfo{
      .vertexBindingDescriptionCount = static_cast<uint32_t>(bindingDescriptions.size()),
      .pVertexBindingDescriptions = bindingDescriptions.data(),
      .vertexAttributeDescriptionCount = static_cast<uint32_t>(allAttributeDescriptions.size()),
      .pVertexAttributeDescriptions = allAttributeDescriptions.data()
    };

    // Create input assembly info
    vk::PipelineInputAssemblyStateCreateInfo inputAssembly{
      .topology = vk::PrimitiveTopology::eTriangleList,
      .primitiveRestartEnable = VK_FALSE
    };

    // Create viewport state info
    vk::PipelineViewportStateCreateInfo viewportState{
      .viewportCount = 1,
      .scissorCount = 1
    };

    // Create rasterization state info
    vk::PipelineRasterizationStateCreateInfo rasterizer{
      .depthClampEnable = VK_FALSE,
      .rasterizerDiscardEnable = VK_FALSE,
      .polygonMode = vk::PolygonMode::eFill,
      .cullMode = vk::CullModeFlagBits::eNone,
      .frontFace = vk::FrontFace::eCounterClockwise,
      .depthBiasEnable = VK_FALSE,
      .lineWidth = 1.0f
    };

    // Create multisample state info
    vk::PipelineMultisampleStateCreateInfo multisampling{
      .rasterizationSamples = vk::SampleCountFlagBits::e1,
      .sampleShadingEnable = VK_FALSE
    };

    // Create depth stencil state info
    vk::PipelineDepthStencilStateCreateInfo depthStencil{
      .depthTestEnable = VK_TRUE,
      .depthWriteEnable = VK_TRUE,
      .depthCompareOp = vk::CompareOp::eLessOrEqual,
      .depthBoundsTestEnable = VK_FALSE,
      .stencilTestEnable = VK_FALSE
    };

    // Create a color blend attachment state
    vk::PipelineColorBlendAttachmentState colorBlendAttachment{
      .blendEnable = VK_FALSE,
      .colorWriteMask = vk::ColorComponentFlagBits::eR | vk::ColorComponentFlagBits::eG | vk::ColorComponentFlagBits::eB | vk::ColorComponentFlagBits::eA
    };

    // Create color blend state info
    vk::PipelineColorBlendStateCreateInfo colorBlending{
      .logicOpEnable = VK_FALSE,
      .logicOp = vk::LogicOp::eCopy,
      .attachmentCount = 1,
      .pAttachments = &colorBlendAttachment
    };

    // Create dynamic state info
    std::vector dynamicStates = {
      vk::DynamicState::eViewport,
      vk::DynamicState::eScissor
    };

    vk::PipelineDynamicStateCreateInfo dynamicState{
      .dynamicStateCount = static_cast<uint32_t>(dynamicStates.size()),
      .pDynamicStates = dynamicStates.data()
    };

    // Create pipeline layout
    vk::PipelineLayoutCreateInfo pipelineLayoutInfo{
      .setLayoutCount = 1,
      .pSetLayouts = &*descriptorSetLayout,
      .pushConstantRangeCount = 0,
      .pPushConstantRanges = nullptr
    };

    pipelineLayout = vk::raii::PipelineLayout(device, pipelineLayoutInfo);

    // Create pipeline rendering info
    vk::Format depthFormat = findDepthFormat();
    std::cout << "Creating main graphics pipeline with depth format: " << static_cast<int>(depthFormat) << std::endl;

    vk::PipelineRenderingCreateInfo pipelineRenderingInfo{};
    pipelineRenderingInfo.sType = vk::StructureType::ePipelineRenderingCreateInfo;
    pipelineRenderingInfo.colorAttachmentCount = 1;
    pipelineRenderingInfo.pColorAttachmentFormats = &swapChainImageFormat;
    pipelineRenderingInfo.depthAttachmentFormat = depthFormat;
    pipelineRenderingInfo.stencilAttachmentFormat = vk::Format::eUndefined;

    // Create the graphics pipeline
    vk::PipelineRasterizationStateCreateInfo rasterizerBack = rasterizer;
    // Disable back-face culling for opaque PBR to avoid disappearing geometry when
    // instance/model transforms flip winding (ensures PASS 1 actually shades pixels)
    rasterizerBack.cullMode = vk::CullModeFlagBits::eNone;

    vk::GraphicsPipelineCreateInfo pipelineInfo{};
    pipelineInfo.sType = vk::StructureType::eGraphicsPipelineCreateInfo;
    pipelineInfo.pNext = &pipelineRenderingInfo;
    pipelineInfo.stageCount = 2;
    pipelineInfo.pStages = shaderStages;
    pipelineInfo.pVertexInputState = &vertexInputInfo;
    pipelineInfo.pInputAssemblyState = &inputAssembly;
    pipelineInfo.pViewportState = &viewportState;
    pipelineInfo.pRasterizationState = &rasterizerBack;
    pipelineInfo.pMultisampleState = &multisampling;
    pipelineInfo.pDepthStencilState = &depthStencil;
    pipelineInfo.pColorBlendState = &colorBlending;
    pipelineInfo.pDynamicState = &dynamicState;
    pipelineInfo.layout = *pipelineLayout;

    graphicsPipeline = vk::raii::Pipeline(device, nullptr, pipelineInfo);
    return true;
  } catch (const std::exception& e) {
    std::cerr << "Failed to create graphics pipeline: " << e.what() << std::endl;
    return false;
  }
}

// Create PBR pipeline
bool Renderer::createPBRPipeline() {
  LOGI("Entering createPBRPipeline");
  try {
    // Create PBR descriptor set layout
    LOGI("Creating PBR descriptor set layout...");
    if (!createPBRDescriptorSetLayout()) {
      return false;
    }
    LOGI("PBR descriptor set layout created");

    // Read shader code
    LOGI("Reading PBR shader...");
    std::string shaderPath = "shaders/pbr.spv";
    auto shaderCode = readFile(shaderPath);
    LOGI("PBR shader read successfully (%s), size: %zu", shaderPath.c_str(), shaderCode.size());

    // Create shader modules
    vk::raii::ShaderModule shaderModule = createShaderModule(shaderCode);
    LOGI("PBR shader module created");

    // Create shader stage info
    LOGI("Creating shader stage info...");
    vk::PipelineShaderStageCreateInfo vertShaderStageInfo{};
    vertShaderStageInfo.sType = vk::StructureType::ePipelineShaderStageCreateInfo;
    vertShaderStageInfo.stage = vk::ShaderStageFlagBits::eVertex;
    vertShaderStageInfo.module = *shaderModule;
    vertShaderStageInfo.pName = "VSMain";

    vk::PipelineShaderStageCreateInfo fragShaderStageInfo{};
    fragShaderStageInfo.sType = vk::StructureType::ePipelineShaderStageCreateInfo;
    fragShaderStageInfo.stage = vk::ShaderStageFlagBits::eFragment;
    fragShaderStageInfo.module = *shaderModule;
    fragShaderStageInfo.pName = "PSMain";

    // Fragment entry point specialized for architectural glass
    vk::PipelineShaderStageCreateInfo fragGlassStageInfo{};
    fragGlassStageInfo.sType = vk::StructureType::ePipelineShaderStageCreateInfo;
    fragGlassStageInfo.stage = vk::ShaderStageFlagBits::eFragment;
    fragGlassStageInfo.module = *shaderModule;
    fragGlassStageInfo.pName = "GlassPSMain";
    LOGI("Shader stage info created");

    vk::PipelineShaderStageCreateInfo shaderStages[] = {vertShaderStageInfo, fragShaderStageInfo};

    // Define vertex and instance binding descriptions
    LOGI("Defining vertex input descriptions...");
    auto vertexBindingDescription = Vertex::getBindingDescription();
    auto instanceBindingDescription = InstanceData::getBindingDescription();
    std::array<vk::VertexInputBindingDescription, 2> bindingDescriptions = {
      vertexBindingDescription,
      instanceBindingDescription
    };

    // Define vertex and instance attribute descriptions
    auto vertexAttributeDescriptions = Vertex::getAttributeDescriptions();
    auto instanceModelMatrixAttributes = InstanceData::getModelMatrixAttributeDescriptions();
    auto instanceNormalMatrixAttributes = InstanceData::getNormalMatrixAttributeDescriptions();

    // Combine all attribute descriptions
    std::vector<vk::VertexInputAttributeDescription> allAttributeDescriptions;
    allAttributeDescriptions.insert(allAttributeDescriptions.end(), vertexAttributeDescriptions.begin(), vertexAttributeDescriptions.end());
    allAttributeDescriptions.insert(allAttributeDescriptions.end(), instanceModelMatrixAttributes.begin(), instanceModelMatrixAttributes.end());
    allAttributeDescriptions.insert(allAttributeDescriptions.end(), instanceNormalMatrixAttributes.begin(), instanceNormalMatrixAttributes.end());

    vk::PipelineVertexInputStateCreateInfo vertexInputInfo{};
    vertexInputInfo.sType = vk::StructureType::ePipelineVertexInputStateCreateInfo;
    vertexInputInfo.vertexBindingDescriptionCount = static_cast<uint32_t>(bindingDescriptions.size());
    vertexInputInfo.pVertexBindingDescriptions = bindingDescriptions.data();
    vertexInputInfo.vertexAttributeDescriptionCount = static_cast<uint32_t>(allAttributeDescriptions.size());
    vertexInputInfo.pVertexAttributeDescriptions = allAttributeDescriptions.data();

    // Create input assembly info
    vk::PipelineInputAssemblyStateCreateInfo inputAssembly{};
    inputAssembly.sType = vk::StructureType::ePipelineInputAssemblyStateCreateInfo;
    inputAssembly.topology = vk::PrimitiveTopology::eTriangleList;
    inputAssembly.primitiveRestartEnable = vk::False;

    // Create viewport state info
    vk::PipelineViewportStateCreateInfo viewportState{};
    viewportState.sType = vk::StructureType::ePipelineViewportStateCreateInfo;
    viewportState.viewportCount = 1;
    viewportState.scissorCount = 1;

    // Create rasterization state info
    vk::PipelineRasterizationStateCreateInfo rasterizer{};
    rasterizer.sType = vk::StructureType::ePipelineRasterizationStateCreateInfo;
    rasterizer.depthClampEnable = vk::False;
    rasterizer.rasterizerDiscardEnable = vk::False;
    rasterizer.polygonMode = vk::PolygonMode::eFill;
    rasterizer.cullMode = vk::CullModeFlagBits::eNone;
    rasterizer.frontFace = vk::FrontFace::eCounterClockwise;
    rasterizer.depthBiasEnable = vk::False;
    rasterizer.lineWidth = 1.0f;

    // Create multisample state info
    vk::PipelineMultisampleStateCreateInfo multisampling{};
    multisampling.sType = vk::StructureType::ePipelineMultisampleStateCreateInfo;
    multisampling.rasterizationSamples = vk::SampleCountFlagBits::e1;
    multisampling.sampleShadingEnable = vk::False;

    // Create depth stencil state info
    vk::PipelineDepthStencilStateCreateInfo depthStencil{};
    depthStencil.sType = vk::StructureType::ePipelineDepthStencilStateCreateInfo;
    depthStencil.depthTestEnable = vk::True;
    depthStencil.depthWriteEnable = vk::True;
    depthStencil.depthCompareOp = vk::CompareOp::eLess;
    depthStencil.depthBoundsTestEnable = vk::False;
    depthStencil.stencilTestEnable = vk::False;

    // Create a color blend attachment state
    vk::PipelineColorBlendAttachmentState colorBlendAttachment{};
    colorBlendAttachment.blendEnable = vk::False;
    colorBlendAttachment.colorWriteMask = vk::ColorComponentFlagBits::eR | vk::ColorComponentFlagBits::eG | vk::ColorComponentFlagBits::eB | vk::ColorComponentFlagBits::eA;

    // Create color blend state info
    vk::PipelineColorBlendStateCreateInfo colorBlending{};
    colorBlending.sType = vk::StructureType::ePipelineColorBlendStateCreateInfo;
    colorBlending.logicOpEnable = vk::False;
    colorBlending.logicOp = vk::LogicOp::eCopy;
    colorBlending.attachmentCount = 1;
    colorBlending.pAttachments = &colorBlendAttachment;

    // Create dynamic state info
    std::vector dynamicStates = {
      vk::DynamicState::eViewport,
      vk::DynamicState::eScissor
    };

    vk::PipelineDynamicStateCreateInfo dynamicState{};
    dynamicState.sType = vk::StructureType::ePipelineDynamicStateCreateInfo;
    dynamicState.dynamicStateCount = static_cast<uint32_t>(dynamicStates.size());
    dynamicState.pDynamicStates = dynamicStates.data();

    // Create push constant range for material properties
    vk::PushConstantRange pushConstantRange{};
    pushConstantRange.stageFlags = vk::ShaderStageFlagBits::eFragment;
    pushConstantRange.offset = 0;
    pushConstantRange.size = sizeof(MaterialProperties);

    std::array<vk::DescriptorSetLayout, 2> transparentSetLayouts = {*pbrDescriptorSetLayout, *transparentDescriptorSetLayout};
    // Create a pipeline layout for opaque PBR with only the PBR descriptor set (set 0)
    std::array<vk::DescriptorSetLayout, 1> pbrOnlySetLayouts = {*pbrDescriptorSetLayout};
    // Create BOTH pipeline layouts with two descriptor sets (PBR set 0 + scene color set 1)
    vk::PipelineLayoutCreateInfo pipelineLayoutInfo{};
    pipelineLayoutInfo.sType = vk::StructureType::ePipelineLayoutCreateInfo;
    pipelineLayoutInfo.setLayoutCount = static_cast<uint32_t>(transparentSetLayouts.size());
    pipelineLayoutInfo.pSetLayouts = transparentSetLayouts.data();
    pipelineLayoutInfo.pushConstantRangeCount = 1;
    pipelineLayoutInfo.pPushConstantRanges = &pushConstantRange;

    LOGI("Creating pipeline layout...");
    pbrPipelineLayout = vk::raii::PipelineLayout(device, pipelineLayoutInfo);
    LOGI("Pipeline layout created");

    // Transparent PBR layout uses the same two-set layout
    vk::PipelineLayoutCreateInfo transparentPipelineLayoutInfo{};
    transparentPipelineLayoutInfo.sType = vk::StructureType::ePipelineLayoutCreateInfo;
    transparentPipelineLayoutInfo.setLayoutCount = static_cast<uint32_t>(transparentSetLayouts.size());
    transparentPipelineLayoutInfo.pSetLayouts = transparentSetLayouts.data();
    transparentPipelineLayoutInfo.pushConstantRangeCount = 1;
    transparentPipelineLayoutInfo.pPushConstantRanges = &pushConstantRange;
    LOGI("Creating transparent pipeline layout...");
    pbrTransparentPipelineLayout = vk::raii::PipelineLayout(device, transparentPipelineLayoutInfo);
    LOGI("Transparent pipeline layout created");

    // Create pipeline rendering info
    vk::Format depthFormat = findDepthFormat();
    LOGI("Creating opaque PBR pipeline...");
    LOGI("Device: %p, Dispatcher: %p", (void *) (VkDevice) * device, (void *) device.getDispatcher());

    vk::PipelineRenderingCreateInfo pipelineRenderingInfo{};
    pipelineRenderingInfo.sType = vk::StructureType::ePipelineRenderingCreateInfo;
    pipelineRenderingInfo.colorAttachmentCount = 1;
    pipelineRenderingInfo.pColorAttachmentFormats = &swapChainImageFormat;
    pipelineRenderingInfo.depthAttachmentFormat = depthFormat;
    pipelineRenderingInfo.stencilAttachmentFormat = vk::Format::eUndefined;

    // 1) Opaque PBR pipeline (no blending, depth writes enabled)
    vk::PipelineColorBlendAttachmentState opaqueBlendAttachment = colorBlendAttachment;
    opaqueBlendAttachment.blendEnable = VK_FALSE;

    vk::PipelineColorBlendStateCreateInfo colorBlendingOpaque{};
    colorBlendingOpaque.sType = vk::StructureType::ePipelineColorBlendStateCreateInfo;
    colorBlendingOpaque.logicOpEnable = VK_FALSE;
    colorBlendingOpaque.logicOp = vk::LogicOp::eCopy;
    colorBlendingOpaque.attachmentCount = 1;
    colorBlendingOpaque.pAttachments = &opaqueBlendAttachment;

    vk::PipelineDepthStencilStateCreateInfo depthStencilOpaque = depthStencil;
    depthStencilOpaque.depthWriteEnable = VK_TRUE;

    vk::PipelineRasterizationStateCreateInfo rasterizerBack = rasterizer;
    rasterizerBack.cullMode = vk::CullModeFlagBits::eBack;

    // For architectural glass we often want to see both the inner and outer
    // walls of thin shells (e.g., bar glasses viewed from above). Use
    // no culling for the glass pipeline to render both sides, while
    // keeping back-face culling for the generic PBR pipelines.
    vk::PipelineRasterizationStateCreateInfo rasterizerGlass = rasterizer;
    rasterizerGlass.cullMode = vk::CullModeFlagBits::eNone;

    vk::GraphicsPipelineCreateInfo opaquePipelineInfo{};
    opaquePipelineInfo.sType = vk::StructureType::eGraphicsPipelineCreateInfo;
    opaquePipelineInfo.pNext = &pipelineRenderingInfo;
    opaquePipelineInfo.stageCount = 2;
    opaquePipelineInfo.pStages = shaderStages;
    opaquePipelineInfo.pVertexInputState = &vertexInputInfo;
    opaquePipelineInfo.pInputAssemblyState = &inputAssembly;
    opaquePipelineInfo.pViewportState = &viewportState;
    opaquePipelineInfo.pRasterizationState = &rasterizerBack;
    opaquePipelineInfo.pMultisampleState = &multisampling;
    opaquePipelineInfo.pDepthStencilState = &depthStencilOpaque;
    opaquePipelineInfo.pColorBlendState = &colorBlendingOpaque;
    opaquePipelineInfo.pDynamicState = &dynamicState;
    opaquePipelineInfo.layout = *pbrPipelineLayout;

    LOGI("Calling vkCreateGraphicsPipelines for opaque PBR...");
    pbrGraphicsPipeline = vk::raii::Pipeline(device, nullptr, opaquePipelineInfo);
    LOGI("Opaque PBR pipeline created");

    // 2) Blended PBR pipeline (straight alpha blending, depth writes disabled for translucency)
    vk::PipelineColorBlendAttachmentState blendedAttachment = colorBlendAttachment;
    blendedAttachment.blendEnable = VK_TRUE;
    // Straight alpha blending: out.rgb = src.rgb*src.a + dst.rgb*(1-src.a)
    blendedAttachment.srcColorBlendFactor = vk::BlendFactor::eSrcAlpha;
    blendedAttachment.dstColorBlendFactor = vk::BlendFactor::eOneMinusSrcAlpha;
    // Alpha channel keeps destination scaled by inverse src alpha
    blendedAttachment.srcAlphaBlendFactor = vk::BlendFactor::eOne;
    blendedAttachment.dstAlphaBlendFactor = vk::BlendFactor::eOneMinusSrcAlpha;
    vk::PipelineColorBlendStateCreateInfo colorBlendingBlended{.attachmentCount = 1, .pAttachments = &blendedAttachment};
    vk::PipelineDepthStencilStateCreateInfo depthStencilBlended = depthStencil;
    depthStencilBlended.depthWriteEnable = VK_FALSE;
    depthStencilBlended.depthCompareOp = vk::CompareOp::eLessOrEqual;

    vk::GraphicsPipelineCreateInfo blendedPipelineInfo{};
    blendedPipelineInfo.sType = vk::StructureType::eGraphicsPipelineCreateInfo;
    blendedPipelineInfo.pNext = &pipelineRenderingInfo;
    blendedPipelineInfo.stageCount = 2;
    blendedPipelineInfo.pStages = shaderStages;
    blendedPipelineInfo.pVertexInputState = &vertexInputInfo;
    blendedPipelineInfo.pInputAssemblyState = &inputAssembly;
    blendedPipelineInfo.pViewportState = &viewportState;
    blendedPipelineInfo.pRasterizationState = &rasterizerBack;
    blendedPipelineInfo.pMultisampleState = &multisampling;
    blendedPipelineInfo.pDepthStencilState = &depthStencilBlended;
    blendedPipelineInfo.pColorBlendState = &colorBlendingBlended;
    blendedPipelineInfo.pDynamicState = &dynamicState;
    blendedPipelineInfo.layout = *pbrTransparentPipelineLayout;

    pbrBlendGraphicsPipeline = vk::raii::Pipeline(device, nullptr, blendedPipelineInfo);

    // 3) Glass pipeline (architectural glass) - uses the same vertex input and
    // descriptor layouts, but a dedicated fragment shader entry point
    // (GlassPSMain) for more stable glass shading.
    vk::PipelineShaderStageCreateInfo glassStages[] = {vertShaderStageInfo, fragGlassStageInfo};

    vk::GraphicsPipelineCreateInfo glassPipelineInfo{};
    glassPipelineInfo.sType = vk::StructureType::eGraphicsPipelineCreateInfo;
    glassPipelineInfo.pNext = &pipelineRenderingInfo;
    glassPipelineInfo.stageCount = 2;
    glassPipelineInfo.pStages = glassStages;
    glassPipelineInfo.pVertexInputState = &vertexInputInfo;
    glassPipelineInfo.pInputAssemblyState = &inputAssembly;
    glassPipelineInfo.pViewportState = &viewportState;
    glassPipelineInfo.pRasterizationState = &rasterizerGlass;
    glassPipelineInfo.pMultisampleState = &multisampling;
    glassPipelineInfo.pDepthStencilState = &depthStencilBlended;
    glassPipelineInfo.pColorBlendState = &colorBlendingBlended;
    glassPipelineInfo.pDynamicState = &dynamicState;
    glassPipelineInfo.layout = *pbrTransparentPipelineLayout;

    glassGraphicsPipeline = vk::raii::Pipeline(device, nullptr, glassPipelineInfo);

    return true;
  } catch (const std::exception& e) {
    std::cerr << "Failed to create PBR pipeline: " << e.what() << std::endl;
    return false;
  }
}

// Create fullscreen composite pipeline (samples off-screen color and writes to swapchain)
bool Renderer::createCompositePipeline() {
  try {
    // Reuse the transparent descriptor set layout (binding 0 = combined image sampler)
    if (*transparentDescriptorSetLayout == nullptr) {
      // Ensure PBR pipeline path created it
      if (!createPBRPipeline()) {
        return false;
      }
    }

    // Read composite shader code
    auto shaderCode = readFile("shaders/composite.spv");
    vk::raii::ShaderModule shaderModule = createShaderModule(shaderCode);

    // Shader stages
    vk::PipelineShaderStageCreateInfo vert{};
    vert.sType = vk::StructureType::ePipelineShaderStageCreateInfo;
    vert.stage = vk::ShaderStageFlagBits::eVertex;
    vert.module = *shaderModule;
    vert.pName = "VSMain";

    vk::PipelineShaderStageCreateInfo frag{};
    frag.sType = vk::StructureType::ePipelineShaderStageCreateInfo;
    frag.stage = vk::ShaderStageFlagBits::eFragment;
    frag.module = *shaderModule;
    frag.pName = "PSMain";

    vk::PipelineShaderStageCreateInfo stages[] = {vert, frag};

    // No vertex inputs (fullscreen triangle via SV_VertexID)
    vk::PipelineVertexInputStateCreateInfo vertexInput{};
    vk::PipelineInputAssemblyStateCreateInfo inputAssembly{.topology = vk::PrimitiveTopology::eTriangleList};
    vk::PipelineViewportStateCreateInfo viewportState{.viewportCount = 1, .scissorCount = 1};
    vk::PipelineRasterizationStateCreateInfo rasterizer{.polygonMode = vk::PolygonMode::eFill, .cullMode = vk::CullModeFlagBits::eNone, .frontFace = vk::FrontFace::eCounterClockwise, .lineWidth = 1.0f};
    vk::PipelineMultisampleStateCreateInfo multisampling{.rasterizationSamples = vk::SampleCountFlagBits::e1};
    // No depth
    vk::PipelineDepthStencilStateCreateInfo depthStencil{.depthTestEnable = VK_FALSE, .depthWriteEnable = VK_FALSE};
    // No blending (we clear swapchain before this and blend transparents later)
    vk::PipelineColorBlendAttachmentState attachment{
      .blendEnable = VK_FALSE,
      .colorWriteMask = vk::ColorComponentFlagBits::eR | vk::ColorComponentFlagBits::eG |
      vk::ColorComponentFlagBits::eB | vk::ColorComponentFlagBits::eA
    };
    vk::PipelineColorBlendStateCreateInfo colorBlending{.attachmentCount = 1, .pAttachments = &attachment};
    std::array dynStates = {vk::DynamicState::eViewport, vk::DynamicState::eScissor};
    vk::PipelineDynamicStateCreateInfo dynamicState{.dynamicStateCount = static_cast<uint32_t>(dynStates.size()), .pDynamicStates = dynStates.data()};

    // Pipeline layout: single set (combined image sampler) + push constants for exposure/gamma/srgb flag
    vk::DescriptorSetLayout setLayouts[] = {*transparentDescriptorSetLayout};
    vk::PushConstantRange pushRange{.stageFlags = vk::ShaderStageFlagBits::eFragment, .offset = 0, .size = 16}; // matches struct Push in composite.slang
    vk::PipelineLayoutCreateInfo plInfo{.setLayoutCount = 1, .pSetLayouts = setLayouts, .pushConstantRangeCount = 1, .pPushConstantRanges = &pushRange};
    compositePipelineLayout = vk::raii::PipelineLayout(device, plInfo);

    // Dynamic rendering info
    vk::PipelineRenderingCreateInfo pipelineRenderingInfo{};
    pipelineRenderingInfo.sType = vk::StructureType::ePipelineRenderingCreateInfo;
    pipelineRenderingInfo.colorAttachmentCount = 1;
    pipelineRenderingInfo.pColorAttachmentFormats = &swapChainImageFormat;
    pipelineRenderingInfo.depthAttachmentFormat = vk::Format::eUndefined;
    pipelineRenderingInfo.stencilAttachmentFormat = vk::Format::eUndefined;

    vk::GraphicsPipelineCreateInfo pipeInfo{};
    pipeInfo.sType = vk::StructureType::eGraphicsPipelineCreateInfo;
    pipeInfo.pNext = &pipelineRenderingInfo;
    pipeInfo.stageCount = 2;
    pipeInfo.pStages = stages;
    pipeInfo.pVertexInputState = &vertexInput;
    pipeInfo.pInputAssemblyState = &inputAssembly;
    pipeInfo.pViewportState = &viewportState;
    pipeInfo.pRasterizationState = &rasterizer;
    pipeInfo.pMultisampleState = &multisampling;
    pipeInfo.pDepthStencilState = &depthStencil;
    pipeInfo.pColorBlendState = &colorBlending;
    pipeInfo.pDynamicState = &dynamicState;
    pipeInfo.layout = *compositePipelineLayout;

    compositePipeline = vk::raii::Pipeline(device, nullptr, pipeInfo);
    return true;
  } catch (const std::exception& e) {
    std::cerr << "Failed to create composite pipeline: " << e.what() << std::endl;
    return false;
  }
}

// Push material properties to the pipeline
void Renderer::pushMaterialProperties(vk::CommandBuffer commandBuffer, const MaterialProperties& material) const {
  commandBuffer.pushConstants(*pbrPipelineLayout, vk::ShaderStageFlagBits::eFragment, 0, sizeof(MaterialProperties), &material);
}
