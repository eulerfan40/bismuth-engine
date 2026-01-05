#pragma once

#pragma once

#include "Pipeline.hpp"
#include "Device.hpp"
#include "GameObject.hpp"
#include "Camera.hpp"

//std
#include <memory>
#include <vector>

namespace engine {
  class SimpleRenderSystem {
  public:
    SimpleRenderSystem(Device &device, VkRenderPass renderPass);

    ~SimpleRenderSystem();

    SimpleRenderSystem(const SimpleRenderSystem &) = delete;

    SimpleRenderSystem &operator=(const SimpleRenderSystem &) = delete;

    void renderGameObjects(VkCommandBuffer commandBuffer, std::vector<GameObject> &gameObjects, const Camera& camera);

  private:
    void createPipelineLayout();

    void createPipeline(VkRenderPass renderPass);

    Device &device;
    std::unique_ptr<Pipeline> pipeline;
    VkPipelineLayout pipelineLayout;
  };
}
