#include "SimpleRenderSystem.hpp"

// libs
#define GLM_FORCE_RADIANS
// Expect depth buffer values to range from 0 to 1 as opposed to OpenGL standard which is -1 to 1
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>
#include <glm/gtc/constants.hpp>

#include <stdexcept>
#include <array>
#include <iostream>

namespace engine {
  struct SimplePushConstantData {
    glm::mat4 transform{1.f};
    alignas(16) glm::vec3 color;
  };

  SimpleRenderSystem::SimpleRenderSystem(Device &device, VkRenderPass renderPass) : device{device} {
    createPipelineLayout();
    createPipeline(renderPass);
  }

  SimpleRenderSystem::~SimpleRenderSystem() {
    vkDestroyPipelineLayout(device.device(), pipelineLayout, nullptr);
  }

  void SimpleRenderSystem::createPipelineLayout() {
    VkPushConstantRange pushConstantRange{};
    pushConstantRange.stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;
    pushConstantRange.offset = 0;
    pushConstantRange.size = sizeof(SimplePushConstantData);

    VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
    pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    pipelineLayoutInfo.setLayoutCount = 0;
    pipelineLayoutInfo.pSetLayouts = nullptr;
    pipelineLayoutInfo.pushConstantRangeCount = 1;
    pipelineLayoutInfo.pPushConstantRanges = &pushConstantRange;

    if (vkCreatePipelineLayout(device.device(), &pipelineLayoutInfo, nullptr, &pipelineLayout) != VK_SUCCESS) {
      throw std::runtime_error("Failed to create pipeline layout!");
    }
  }

  void SimpleRenderSystem::createPipeline(VkRenderPass renderPass) {
    assert(pipelineLayout != nullptr && "Cannot create pipeline before pipeline layout!");

    PipelineConfigInfo pipelineConfig{};
    Pipeline::defaultPipelineConfigInfo(pipelineConfig);
    pipelineConfig.renderPass = renderPass;
    pipelineConfig.pipelineLayout = pipelineLayout;
    pipeline = std::make_unique<Pipeline>(
      device,
      std::string(COMPILED_SHADERS_DIR) + "simple_shader.vert.spv",
      std::string(COMPILED_SHADERS_DIR) + "simple_shader.frag.spv",
      pipelineConfig);
  }

  void SimpleRenderSystem::renderGameObjects(VkCommandBuffer commandBuffer, std::vector<GameObject> &gameObjects) {
    pipeline->bind(commandBuffer);

    for (auto &obj: gameObjects) {
      // Continuously rotate the cube along the y axis and x axis.
      obj.transform.rotation.y = glm::mod(obj.transform.rotation.y + 0.01f, glm::two_pi<float>());
      obj.transform.rotation.x = glm::mod(obj.transform.rotation.x + 0.005f, glm::two_pi<float>());

      SimplePushConstantData push{};
      push.color = obj.color;
      push.transform = obj.transform.mat4();

      vkCmdPushConstants(
        commandBuffer,
        pipelineLayout,
        VK_SHADER_STAGE_FRAGMENT_BIT | VK_SHADER_STAGE_VERTEX_BIT,
        0,
        sizeof(SimplePushConstantData),
        &push);

      obj.model->bind(commandBuffer);
      obj.model->draw(commandBuffer);
    }
  }
}
