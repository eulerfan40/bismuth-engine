#pragma once

// libs
#define GLM_FORCE_RADIANS
// Expect depth buffer values to range from 0 to 1 as opposed to OpenGL standard which is -1 to 1
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>

namespace engine {
  class Camera {
  public:
    void setOrthographicProjection(float left,
                                   float right,
                                   float top,
                                   float bottom,
                                   float near,
                                   float far);

    void setPerspectiveProjection(float fovy, float aspect, float near, float far);

    const glm::mat4 &getProjection() const { return projectionMatrix; };

  private:
    glm::mat4 projectionMatrix{1.0f};
  };
}
