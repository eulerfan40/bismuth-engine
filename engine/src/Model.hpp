#pragma once

#include "Device.hpp"

// libs
#define GLM_FORCE_RADIANS
// Expect depth buffer values to range from 0 to 1 as opposed to OpenGL standard which is -1 to 1
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>

// std
#include <memory>
#include <vector>

namespace engine {
  class Model {
  public:
    struct Vertex {
      glm::vec3 position{};
      glm::vec3 color{};
      glm::vec3 normal{};
      glm::vec2 uv{};

      static std::vector<VkVertexInputBindingDescription> getBindingDescriptions();

      static std::vector<VkVertexInputAttributeDescription> getAttributeDescriptions();

      bool operator==(const Vertex &other) const {
        return position == other.position && color == other.color && normal == other.normal && uv == other.uv;
      }
    };

    struct Data {
      std::vector<Vertex> vertices{};
      std::vector<uint32_t> indices{};

      void loadModel(const std::string &filePath);
    };

    Model(Device &device, const Data &data);

    ~Model();

    Model(const Model &) = delete;

    Model &operator=(const Model &) = delete;

    static std::unique_ptr<Model> createModelFromFile(Device &device, const std::string &filePath);

    void bind(VkCommandBuffer commandBuffer);

    void draw(VkCommandBuffer commandBuffer);

  private:
    void createVertexBuffers(const std::vector<Vertex> &vertices);

    void createIndexBuffer(const std::vector<uint32_t> &indices);

    Device &device;

    VkBuffer vertexBuffer;
    VkDeviceMemory vertexBufferMemory;
    uint32_t vertexCount;

    bool hasIndexBuffer = false;
    VkBuffer indexBuffer;
    VkDeviceMemory indexBufferMemory;
    uint32_t indexCount;
  };
}
