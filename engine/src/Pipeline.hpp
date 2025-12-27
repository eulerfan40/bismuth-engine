#pragma once

#include "Device.hpp"
#include <string>
#include <vector>

namespace engine {
  // This struct contains the data we will use to configure the graphics pipeline
  struct PipelineConfigInfo {
    VkViewport viewport;
    VkRect2D scissor;

    VkPipelineInputAssemblyStateCreateInfo inputAssemblyInfo;
    VkPipelineRasterizationStateCreateInfo rasterizationInfo;
    VkPipelineMultisampleStateCreateInfo multisampleInfo;
    VkPipelineColorBlendAttachmentState colorBlendAttachment;
    VkPipelineColorBlendStateCreateInfo colorBlendInfo;
    VkPipelineDepthStencilStateCreateInfo depthStencilInfo;
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

    void operator=(const Pipeline &) = delete;

    void bind(VkCommandBuffer commandBuffer);

    static PipelineConfigInfo defaultPipelineConfigInfo(uint32_t width, uint32_t height);

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
