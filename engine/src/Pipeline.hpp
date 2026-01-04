#pragma once

#include "Device.hpp"
#include <string>
#include <vector>

namespace engine {
  // This struct contains the data we will use to configure the graphics pipeline
  struct PipelineConfigInfo {
    PipelineConfigInfo() = default;
    PipelineConfigInfo(const PipelineConfigInfo&) = delete;
    PipelineConfigInfo& operator=(const PipelineConfigInfo&) = delete;

    VkPipelineViewportStateCreateInfo viewportInfo;
    VkPipelineInputAssemblyStateCreateInfo inputAssemblyInfo;
    VkPipelineRasterizationStateCreateInfo rasterizationInfo;
    VkPipelineMultisampleStateCreateInfo multisampleInfo;
    VkPipelineColorBlendAttachmentState colorBlendAttachment;
    VkPipelineColorBlendStateCreateInfo colorBlendInfo;
    VkPipelineDepthStencilStateCreateInfo depthStencilInfo;
    std::vector<VkDynamicState> dynamicStateEnables;
    VkPipelineDynamicStateCreateInfo dynamicStateInfo;
    VkPipelineLayout pipelineLayout = nullptr;
    VkRenderPass renderPass = nullptr;
    uint32_t subpass = 0;
  };

  class Pipeline {
  public:
    Pipeline(Device &device,
             const std::string &vertPath,
             const std::string &fragPath,
             const PipelineConfigInfo &configInfo);

    ~Pipeline();

    // Delete copy constructors to avoid duplicating pointers to our Vulkan objects
    Pipeline(const Pipeline &) = delete;

    Pipeline& operator=(const Pipeline &) = delete;

    void bind(VkCommandBuffer commandBuffer);

    static void defaultPipelineConfigInfo(PipelineConfigInfo& configInfo);

  private:
    static std::vector<char> readFile(const std::string &path);

    void createGraphicsPipeline(const std::string &vertPath,
                                const std::string &fragPath,
                                const PipelineConfigInfo &configInfo);

    void createShaderModule(const std::vector<char> &code, VkShaderModule *shaderModule);

    // Potentially memory unsafe however the pipeline fundamentally requires a device to exist; aggregation
    Device &device;
    VkPipeline graphicsPipeline;
    VkShaderModule vertShaderModule;
    VkShaderModule fragShaderModule;
  };
}
