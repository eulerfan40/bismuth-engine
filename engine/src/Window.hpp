#pragma once

#include <volk.h> // Must come first

#define GLFW_INCLUDE_NONE // Don't let GLFW include Vulkan headers because volk takes care of that already
#include "GLFW/glfw3.h"

#include <string>

namespace engine {
  class Window {
  public:
    Window(int w, int h, std::string name);

    ~Window();

    // Prevent two pointers from potentially owning the same window
    Window(const Window &) = delete; // Prevents copy constructor
    Window &operator=(const Window &) = delete; // Prevents copy assignment

    // When the close button is pressed
    bool shouldClose() { return glfwWindowShouldClose(window); }

    VkExtent2D getExtent() { return {static_cast<uint32_t>(width), static_cast<uint32_t>(height)}; }

    void createWindowSurface(VkInstance instance, VkSurfaceKHR* surface);

  private:
    void initWindow();

    const int width;
    const int height;

    std::string windowName;
    GLFWwindow *window; // Should always be a unique pointer
  };
}
