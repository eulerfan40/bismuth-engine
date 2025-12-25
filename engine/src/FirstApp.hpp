#pragma once
#include "Window.hpp"
#include "Pipeline.hpp"

namespace engine {
  class FirstApp {
  public:
    static constexpr int WIDTH = 800;
    static constexpr int HEIGHT = 600;

    void run();

  private:
    Window window{WIDTH, HEIGHT, "Vulkan Engine"};
    Pipeline pipeline{std::string(COMPILED_SHADERS_DIR) + "simple_shader.vert.spv",
                      std::string(COMPILED_SHADERS_DIR) + "simple_shader.frag.spv"};
  };
}
