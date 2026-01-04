#pragma once

#include "Device.hpp"

// libs
#define GLM_FORCE_RADIANS
// Expect depth buffer values to range from 0 to 1 as opposed to OpenGL standard which is -1 to 1
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>

// std
#include <vector>

namespace engine {
  class Model {
  public:
    struct Vertex {
      glm::vec2 position;

      glm::vec3 color;

      static std::vector<VkVertexInputBindingDescription> getBindingDescriptions();

      static std::vector<VkVertexInputAttributeDescription> getAttributeDescriptions();
    };

    Model(Device& device, const std::vector<Vertex>& vertices);

    ~Model();

    Model(const Model &) = delete;

    Model &operator=(const Model &) = delete;

    void bind(VkCommandBuffer commandBuffer);

    void draw(VkCommandBuffer commandBuffer);

  private:
    void createVertexBuffers(const std::vector<Vertex>& vertices);

    Device &device;
    VkBuffer vertexBuffer;
    VkDeviceMemory vertexBufferMemory;
    uint32_t vertexCount;
  };
}
