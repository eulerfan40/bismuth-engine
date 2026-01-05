#pragma once

#include "GameObject.hpp"
#include "Window.hpp"

namespace engine {
  class KeyboardMovementController {
  public:
    const float DEFAULT_MOVE_SPEED = 3.0f;
    const float SLOW_MOVE_SPEED = 1.0f;

    struct KeyMappings {
        int moveLeft = GLFW_KEY_A;
        int moveRight = GLFW_KEY_D;
        int moveForward = GLFW_KEY_W;
        int moveBackward = GLFW_KEY_S;
        int moveUp = GLFW_KEY_SPACE;
        int moveDown = GLFW_KEY_LEFT_CONTROL;
        int lookLeft = GLFW_KEY_LEFT;
        int lookRight = GLFW_KEY_RIGHT;
        int lookUp = GLFW_KEY_UP;
        int lookDown = GLFW_KEY_DOWN;
        int slowDown = GLFW_KEY_LEFT_SHIFT;
    };

    void moveInPlaneXZ(GLFWwindow* window, float dt, GameObject& gameObject);

    KeyMappings keys{};
    float moveSpeed{DEFAULT_MOVE_SPEED};
    float lookSpeed{1.5f};
  };
}