#include "Window.hpp"

#include <stdexcept>

namespace engine {
  Window::Window(int w, int h, std::string name) : width{w}, height{h}, windowName{name} {
    initWindow();
  }

  Window::~Window() {
    glfwDestroyWindow(window);
    glfwTerminate();
  }

  void Window::initWindow() {
    // Initialize volk first
    if (volkInitialize() != VK_SUCCESS) {
      throw std::runtime_error("Failed to initialize Vulkan!");
    }

    glfwInit();
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API); // Do not create an OpenGL context
    glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE); // Window resizing is handled later

    window = glfwCreateWindow(width, height, windowName.c_str(), nullptr, nullptr);
  }
}
