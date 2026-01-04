#pragma once

#include "Window.hpp"
#include "Device.hpp"
#include "Renderer.hpp"
#include "GameObject.hpp"

//std
#include <memory>
#include <vector>

namespace engine {
  class FirstApp {
  public:
    static constexpr int WIDTH = 800;
    static constexpr int HEIGHT = 600;

    FirstApp();

    ~FirstApp();

    FirstApp(const FirstApp &) = delete;

    FirstApp &operator=(const FirstApp &) = delete;

    void run();

  private:
    void loadGameObjects();

    Window window{WIDTH, HEIGHT, "Bismuth Engine"};
    Device device{window};
    Renderer renderer{window, device};
    std::vector<GameObject> gameObjects;
  };
}
