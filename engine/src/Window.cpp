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
    glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE);

    window = glfwCreateWindow(width, height, windowName.c_str(), nullptr, nullptr);
    glfwSetWindowUserPointer(window, this);
    glfwSetWindowSizeCallback(window, frameBufferResizeCallback);
  }

  void Window::createWindowSurface(VkInstance instance, VkSurfaceKHR *surface) {
    if (glfwCreateWindowSurface(instance, window, nullptr, surface) != VK_SUCCESS) {
      throw std::runtime_error("Failed to create window surface!");
    }
  }

  void Window::frameBufferResizeCallback(GLFWwindow* window, int width, int height) {
    auto pWindow = reinterpret_cast<Window*>(glfwGetWindowUserPointer(window));
    pWindow->framebufferResized = true;
    pWindow->width = width;
    pWindow->height = height;
  }

}
