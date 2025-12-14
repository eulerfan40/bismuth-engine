#include "FirstApp.hpp"

namespace engine {
  void FirstApp::run() {
    while (!window.shouldClose()) {
      glfwPollEvents(); // Events such as mouse clicks, moving the window, exiting the window
    }
  }
}
