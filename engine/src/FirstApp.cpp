#include "FirstApp.hpp"

#include "SimpleRenderSystem.hpp"
#include "Camera.hpp"
#include "KeyboardMovementController.hpp"

// libs
#define GLM_FORCE_RADIANS
// Expect depth buffer values to range from 0 to 1 as opposed to OpenGL standard which is -1 to 1
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>
#include <glm/gtc/constants.hpp>

#include <stdexcept>
#include <chrono>
#include <array>
#include <iostream>

namespace engine {
  FirstApp::FirstApp() {
    loadGameObjects();
  }

  FirstApp::~FirstApp() {
  }

  void FirstApp::run() {
    const float MAX_FRAME_TIME = 1.0f;

    SimpleRenderSystem simpleRenderSystem{device, renderer.getSwapChainRenderPass()};
    Camera camera{};

    auto viewerObject = GameObject::createGameObject();
    KeyboardMovementController cameraController{};

    auto currentTime = std::chrono::high_resolution_clock::now();

    while (!window.shouldClose()) {
      glfwPollEvents(); // Events such as mouse clicks, moving the window, exiting the window

      auto newTime = std::chrono::high_resolution_clock::now();
      float frameTime = std::chrono::duration<float, std::chrono::seconds::period>(newTime - currentTime).count();
      currentTime = newTime;

      frameTime = glm::min(frameTime, MAX_FRAME_TIME);

      cameraController.moveInPlaneXZ(window.getGLFWwindow(), frameTime, viewerObject);
      camera.setViewYXZ(viewerObject.transform.translation, viewerObject.transform.rotation);

      float aspect = renderer.getAspectRatio();
      camera.setPerspectiveProjection(glm::radians(50.0f), aspect, 0.1f, 10.0f);

      if (auto commandBuffer = renderer.beginFrame()) {
        renderer.beginSwapChainRenderPass(commandBuffer);
        simpleRenderSystem.renderGameObjects(commandBuffer, gameObjects, camera);
        renderer.endSwapChainRenderPass(commandBuffer);
        renderer.endFrame();
      }
    }

    vkDeviceWaitIdle(device.device());
  }

  void FirstApp::loadGameObjects() {
    std::shared_ptr<Model> model = Model::createModelFromFile(
       device, std::string(MODELS_DIR) + "smooth_vase.obj");

    auto gameObject = GameObject::createGameObject();
    gameObject.model = model;
    gameObject.transform.translation = {0.0f, 0.0f, 2.5f};
    gameObject.transform.scale = glm::vec3(3.0f);

    std::shared_ptr<Model> model2 = Model::createModelFromFile(
       device, std::string(MODELS_DIR) + "skull.obj");

    auto gameObject1 = GameObject::createGameObject();
    gameObject1.model = model2;
    gameObject1.transform.translation = {2.0f, 0.0f, 2.5f};
    gameObject1.transform.rotation = {0.0f, 0.0f, glm::radians(90.0f)};
    gameObject1.transform.scale = glm::vec3(0.0175f);

    gameObjects.push_back(std::move(gameObject));
    gameObjects.push_back(std::move(gameObject1));
  }
}
