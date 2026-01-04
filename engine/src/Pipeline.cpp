#include "Pipeline.hpp"
#include "Model.hpp"

//std
#include <fstream> //file input/output
#include <iostream>
#include <stdexcept>
#include <cassert> // Allows us to make sure values are explicitly set

namespace engine {
  Pipeline::Pipeline(Device &device,
                     const std::string &vertPath,
                     const std::string &fragPath,
                     const PipelineConfigInfo &configInfo) : device{device} {
    createGraphicsPipeline(vertPath, fragPath, configInfo);
  };

  Pipeline::~Pipeline() {
    // Shader modules are GPU objects created from SPIR-V bytecode
    vkDestroyShaderModule(device.device(), vertShaderModule, nullptr);
    vkDestroyShaderModule(device.device(), fragShaderModule, nullptr);
    // Destroy the graphics pipeline. This releases all GPU state associated with the pipeline
    vkDestroyPipeline(device.device(), graphicsPipeline, nullptr);
  }

  // Read an entire file into memory and return its contents as a vector of chars
  std::vector<char> Pipeline::readFile(const std::string &path) {
    // Open the file, seek to the end immediately, and read raw bytes to avoid text conversion
    std::ifstream file{path, std::ios::ate | std::ios::binary};

    if (!file.is_open()) {
      throw std::runtime_error{"Failed to open file \"" + path + "\"!"};
    }

    // tellg() returns current read position (which we said should be the last character)
    size_t fileSize = static_cast<size_t>(file.tellg());
    std::vector<char> buffer(fileSize); // Create a buffer big enough to hold the whole file

    file.seekg(0); // Return to the first position in the file
    // Read fileSize bytes into the buffer
    file.read(buffer.data(), fileSize); // buffer.data() returns a raw pointer to the vector's storage

    file.close();
    return buffer;
  }

  void Pipeline::createGraphicsPipeline(const std::string &vertPath,
                                        const std::string &fragPath,
                                        const PipelineConfigInfo &configInfo) {
    // Ensures a valid pipeline layout was provided, which defines descriptor sets and push constants
    assert(
      configInfo.pipelineLayout != VK_NULL_HANDLE &&
      "Cannot create graphics pipeline: No pipelineLayout provided in configInfo!");
    // Ensures a valid render pass was provided, which defines the framebuffer attachments this pipeline can render into
    assert(
      configInfo.renderPass != VK_NULL_HANDLE &&
      "Cannot create graphics pipeline: No renderPass provided in configInfo!");

    auto vertCode = readFile(vertPath);
    auto fragCode = readFile(fragPath);

    createShaderModule(vertCode, &vertShaderModule);
    createShaderModule(fragCode, &fragShaderModule);

    // -------------------- SHADER STAGES --------------------
    // Each shader stage describes a programmable stage of the pipeline
    VkPipelineShaderStageCreateInfo shaderStages[2];

    // Vertex shader stage
    shaderStages[0].sType =
        VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    shaderStages[0].stage = VK_SHADER_STAGE_VERTEX_BIT;
    shaderStages[0].module = vertShaderModule;
    // Entry point function name in the shader. This must match the function name in the SPIR-V
    shaderStages[0].pName = "main";
    // No special flags or specialization constants
    shaderStages[0].flags = 0;
    shaderStages[0].pNext = nullptr;
    shaderStages[0].pSpecializationInfo = nullptr;

    // Fragment shader stage
    shaderStages[1].sType =
        VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    shaderStages[1].stage = VK_SHADER_STAGE_FRAGMENT_BIT;
    shaderStages[1].module = fragShaderModule;
    shaderStages[1].pName = "main";
    shaderStages[1].flags = 0;
    shaderStages[1].pNext = nullptr;
    shaderStages[1].pSpecializationInfo = nullptr;

    // -------------------- VERTEX INPUT STATE --------------------
    auto bindingDescriptions = Model::Vertex::getBindingDescriptions();
    auto attributeDescriptions = Model::Vertex::getAttributeDescriptions();
    // Describes how vertex data is read from vertex buffers
    VkPipelineVertexInputStateCreateInfo vertexInputInfo{};
    vertexInputInfo.sType =
        VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
    vertexInputInfo.vertexBindingDescriptionCount = static_cast<uint32_t>(bindingDescriptions.size());
    vertexInputInfo.vertexAttributeDescriptionCount = static_cast<uint32_t>(attributeDescriptions.size());
    vertexInputInfo.pVertexAttributeDescriptions = attributeDescriptions.data();
    vertexInputInfo.pVertexBindingDescriptions = bindingDescriptions.data();

    // -------------------- VIEWPORT STATE --------------------
    // This struct connects the viewport and scissor to the pipeline
    VkPipelineViewportStateCreateInfo viewportInfo{};
    viewportInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
    // Number of viewports used by the pipeline
    viewportInfo.viewportCount = 1;
    // Pointer to the viewport description
    viewportInfo.pViewports = &configInfo.viewport;
    // Number of scissor rectangles used by the pipeline
    viewportInfo.scissorCount = 1;
    // Pointer to the scissor rectangle description
    viewportInfo.pScissors = &configInfo.scissor;

    // -------------------- GRAPHICS PIPELINE CREATE INFO --------------------
    // This struct ties together all pipeline stages into a single object
    VkGraphicsPipelineCreateInfo pipelineInfo{};
    pipelineInfo.sType =
        VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
    // Number of shader stages (vertex + fragment)
    pipelineInfo.stageCount = 2;
    pipelineInfo.pStages = shaderStages;
    // Fixed-function pipeline stages
    pipelineInfo.pVertexInputState = &vertexInputInfo;
    pipelineInfo.pInputAssemblyState = &configInfo.inputAssemblyInfo;
    pipelineInfo.pViewportState = &viewportInfo;
    pipelineInfo.pRasterizationState = &configInfo.rasterizationInfo;
    pipelineInfo.pMultisampleState = &configInfo.multisampleInfo;
    pipelineInfo.pColorBlendState = &configInfo.colorBlendInfo;
    pipelineInfo.pDepthStencilState = &configInfo.depthStencilInfo;
    // No dynamic state is used (all state is baked into the pipeline)
    pipelineInfo.pDynamicState = nullptr;

    // Pipeline layout defines descriptor sets and push constants
    pipelineInfo.layout = configInfo.pipelineLayout;
    // Render pass this pipeline is compatible with
    pipelineInfo.renderPass = configInfo.renderPass;
    // Subpass index within the render pass
    pipelineInfo.subpass = configInfo.subpass;

    // Pipeline derivatives allow sharing compilation work between pipelines. NOT USED HERE
    pipelineInfo.basePipelineIndex = -1;
    pipelineInfo.basePipelineHandle = VK_NULL_HANDLE;

    if (vkCreateGraphicsPipelines(device.device(), VK_NULL_HANDLE, 1, &pipelineInfo,
                                  nullptr, &graphicsPipeline) != VK_SUCCESS) {
      throw std::runtime_error("Failed to create graphics pipeline!");
    }
  }

  void Pipeline::createShaderModule(const std::vector<char> &code, VkShaderModule *shaderModule) {
    VkShaderModuleCreateInfo createInfo{}; // Create a struct with all values initialized to zero
    // Vulkan requires explicit struct type specification
    createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    createInfo.codeSize = code.size();
    // SPIR-V shaders are interpreted in 32 bit words instead of bytes
    createInfo.pCode = reinterpret_cast<const uint32_t *>(code.data());

    if (vkCreateShaderModule(device.device(), &createInfo, nullptr, shaderModule) != VK_SUCCESS) {
      throw std::runtime_error("Failed to create shader module!");
    }
  }

  void Pipeline::bind(VkCommandBuffer commandBuffer) {
    vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, graphicsPipeline);
  }

  PipelineConfigInfo Pipeline::defaultPipelineConfigInfo(uint32_t width, uint32_t height) {
    PipelineConfigInfo configInfo{};

    // -------------------- INPUT ASSEMBLY STATE --------------------
    // This struct describes how Vulkan should assemble vertices into primitives
    configInfo.inputAssemblyInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    // Specifies the type of geometric primitive to create from the vertices
    // VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST means:
    // - Every group of 3 vertices forms an independent triangle
    configInfo.inputAssemblyInfo.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
    // Disables primitive restart
    // Primitive restart allows you to use a special index value to restart a strip
    // This is mainly useful for triangle strips or line strips
    // Since we are using a triangle list, this is not needed
    configInfo.inputAssemblyInfo.primitiveRestartEnable = VK_FALSE;

    // -------------------- VIEWPORT --------------------
    // The viewport defines the area of the framebuffer that the final image will be rendered to
    // X and Y specify the top-left corner of the viewport in screen space
    configInfo.viewport.x = 0.0f;
    configInfo.viewport.y = 0.0f;
    // Width and height define the size of the viewport
    // Typically matches the swapchain extent (window size)
    configInfo.viewport.width = static_cast<float>(width);
    configInfo.viewport.height = static_cast<float>(height);
    // minDepth and maxDepth define the depth range used after projection
    // Depth values from the vertex shader are mapped into this range
    // 0.0 = near plane, 1.0 = far plane
    configInfo.viewport.minDepth = 0.0f;
    configInfo.viewport.maxDepth = 1.0f;

    // -------------------- SCISSOR RECTANGLE --------------------
    // Any pixels outside of the scissor rectangle will be cut off. screenspace 0-width/height, top right to bottom left
    // Offset defines the top-left corner of the scissor rectangle
    configInfo.scissor.offset = {0, 0};
    // Extent defines the width and height of the scissor rectangle
    configInfo.scissor.extent = {width, height};

    // -------------------- RASTERIZATION STATE --------------------
    // The rasterization stage takes the primitives (triangles) produced by the input assembly stage and converts them
    // into fragments (potential pixels)
    configInfo.rasterizationInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
    // When enabled, fragments outside the near/far depth range are clamped instead of discarded. This requires a
    // special GPU feature and is usually disabled in standard rendering pipelines
    configInfo.rasterizationInfo.depthClampEnable = VK_FALSE;
    // If enabled, geometry is completely discarded before rasterization. This is useful for transform feedback or
    // compute-only pipelines but not for standard rendering
    configInfo.rasterizationInfo.rasterizerDiscardEnable = VK_FALSE;
    // Specifies how polygons are rendered:
    // - VK_POLYGON_MODE_FILL  : filled triangles (normal rendering)
    // - VK_POLYGON_MODE_LINE  : wireframe rendering
    // - VK_POLYGON_MODE_POINT : points only
    configInfo.rasterizationInfo.polygonMode = VK_POLYGON_MODE_FILL;
    // Width of lines when using VK_POLYGON_MODE_LINE. Must be 1.0f unless wide lines are explicitly enabled as a GPU
    // feature
    configInfo.rasterizationInfo.lineWidth = 1.0f;
    // Controls which faces of a triangle are discarded (back-face culling). VK_CULL_MODE_NONE disables face culling
    // entirely
    configInfo.rasterizationInfo.cullMode = VK_CULL_MODE_NONE;
    // Defines which vertex winding order is considered the "front" face. VK_FRONT_FACE_CLOCKWISE means clockwise-wound
    // triangles face the camera (This depends on your coordinate system and projection matrix)
    configInfo.rasterizationInfo.frontFace = VK_FRONT_FACE_CLOCKWISE;
    // Depth bias adds an offset to depth values. Commonly used for shadow mapping to reduce shadow acne
    configInfo.rasterizationInfo.depthBiasEnable = VK_FALSE;
    configInfo.rasterizationInfo.depthBiasConstantFactor = 0.0f;
    configInfo.rasterizationInfo.depthBiasClamp = 0.0f;
    configInfo.rasterizationInfo.depthBiasSlopeFactor = 0.0f;

    // -------------------- MULTISAMPLE STATE --------------------
    // Controls multisampling behavior (MSAA). Multisampling is a technique used to reduce aliasing (jagged edges) by
    // sampling each pixel multiple times at different locations
    configInfo.multisampleInfo.sType =
        VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
    // Enables per-sample shading. When disabled, the fragment shader runs once per pixel. When enabled, it can run once
    // per sample (higher quality, lower performance)
    configInfo.multisampleInfo.sampleShadingEnable = VK_FALSE;
    // Specifies the number of samples per pixel used for rasterization. VK_SAMPLE_COUNT_1_BIT means no multisampling
    // (1 sample per pixel). Common values when MSAA is enabled are 2, 4, or 8 samples
    configInfo.multisampleInfo.rasterizationSamples =
        VK_SAMPLE_COUNT_1_BIT;
    // Minimum fraction of samples that must be shaded. Only relevant when sampleShadingEnable is VK_TRUE. Set to 1.0f
    // to shade all samples (maximum quality)
    configInfo.multisampleInfo.minSampleShading = 1.0f;
    // Optional bitmask that controls which samples are active. nullptr means all samples are enabled
    configInfo.multisampleInfo.pSampleMask = nullptr;
    // Alpha-to-coverage converts the fragment's alpha value into a coverage mask. Commonly used for smoother edges on
    // alpha-tested geometry (e.g. foliage)
    configInfo.multisampleInfo.alphaToCoverageEnable = VK_FALSE;
    // Forces the alpha value of the fragment shader output to 1.0. Rarely used and typically left disabled
    configInfo.multisampleInfo.alphaToOneEnable = VK_FALSE;

    // -------------------- COLOR BLEND ATTACHMENT --------------------
    // Describes how a single framebuffer color attachment is written to after fragment shading
    // Specifies which color channels can be written to the framebuffer. Here we enable writing to RGBA (all components)
    configInfo.colorBlendAttachment.colorWriteMask =
        VK_COLOR_COMPONENT_R_BIT |
        VK_COLOR_COMPONENT_G_BIT |
        VK_COLOR_COMPONENT_B_BIT |
        VK_COLOR_COMPONENT_A_BIT;
    // Disables color blending. The fragment shader output color will overwrite the framebuffer color
    configInfo.colorBlendAttachment.blendEnable = VK_FALSE;
    // These blend factors and operations are ignored when blending is disabled. They are still set to sensible defaults
    // for future extensibility
    configInfo.colorBlendAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_ONE;
    configInfo.colorBlendAttachment.dstColorBlendFactor = VK_BLEND_FACTOR_ZERO;
    configInfo.colorBlendAttachment.colorBlendOp = VK_BLEND_OP_ADD;
    configInfo.colorBlendAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
    configInfo.colorBlendAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
    configInfo.colorBlendAttachment.alphaBlendOp = VK_BLEND_OP_ADD;

    // -------------------- COLOR BLEND STATE --------------------
    // Controls global color blending behavior across all color attachments
    configInfo.colorBlendInfo.sType =
        VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
    // Logic operations apply bitwise operations between fragment output and framebuffer values. Rarely used in modern
    // rendering.
    configInfo.colorBlendInfo.logicOpEnable = VK_FALSE;
    configInfo.colorBlendInfo.logicOp = VK_LOGIC_OP_COPY;
    // Number of color attachments being used by the pipeline
    configInfo.colorBlendInfo.attachmentCount = 1;
    // Pointer to the color blend attachment configuration
    configInfo.colorBlendInfo.pAttachments = &configInfo.colorBlendAttachment;
    // Constant blend color values (used by certain blend factors). Unused here but required to be initialized
    configInfo.colorBlendInfo.blendConstants[0] = 0.0f;
    configInfo.colorBlendInfo.blendConstants[1] = 0.0f;
    configInfo.colorBlendInfo.blendConstants[2] = 0.0f;
    configInfo.colorBlendInfo.blendConstants[3] = 0.0f;

    // -------------------- DEPTH & STENCIL STATE --------------------
    // Controls depth testing and stencil testing. This stage determines whether a fragment should be discarded based on
    // depth and stencil buffer contents
    configInfo.depthStencilInfo.sType =
        VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
    // Enables depth testing. Fragments are compared against the depth buffer
    configInfo.depthStencilInfo.depthTestEnable = VK_TRUE;
    // Enables writing to the depth buffer. Typically enabled for opaque geometry
    configInfo.depthStencilInfo.depthWriteEnable = VK_TRUE;
    // Specifies the depth comparison operation. VK_COMPARE_OP_LESS means closer fragments pass the test
    configInfo.depthStencilInfo.depthCompareOp = VK_COMPARE_OP_LESS;
    // Depth bounds testing allows fragments to pass only if their depth lies within a specified range. Rarely used.
    configInfo.depthStencilInfo.depthBoundsTestEnable = VK_FALSE;
    configInfo.depthStencilInfo.minDepthBounds = 0.0f;
    configInfo.depthStencilInfo.maxDepthBounds = 1.0f;
    // Disables stencil testing. Stencil buffers are useful for outlines, mirrors, and masking
    configInfo.depthStencilInfo.stencilTestEnable = VK_FALSE;
    // Front and back face stencil operations (unused here)
    configInfo.depthStencilInfo.front = {};
    configInfo.depthStencilInfo.back = {};

    return configInfo;
  }
}
