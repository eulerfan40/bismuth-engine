# Render System Documentation

Complete technical documentation for the Render System pattern.

**Files:** `engine/src/SimpleRenderSystem.hpp`, `engine/src/SimpleRenderSystem.cpp`  
**Purpose:** Encapsulate rendering logic for specific object types  
**Pattern:** System pattern from Entity-Component-System (ECS) architecture

---

## Table of Contents

- [Overview](#overview)
- [Class Interface](#class-interface)
- [Pipeline Management](#pipeline-management)
- [Rendering Implementation](#rendering-implementation)
- [Design Patterns](#design-patterns)
- [Usage Examples](#usage-examples)
- [Extending with New Systems](#extending-with-new-systems)
- [Common Issues](#common-issues)
- [Related Documentation](#related-documentation)

---

## Overview

### What Is a Render System?

A **Render System** is a specialized class that knows how to render a specific category of objects:

```
Application
    ↓
Render Systems (specialized rendering logic)
    ├→ SimpleRenderSystem (basic geometry with transforms)
    ├→ ParticleSystem (particle effects)
    ├→ TerrainSystem (heightmap terrain)
    └→ UISystem (user interface)
    ↓
GameObjects / Entities (data)
```

**Responsibilities:**
- **Pipeline Management:** Create and own graphics pipeline
- **Shader Management:** Load appropriate shaders for object type
- **Rendering Logic:** Iterate over objects and issue draw commands
- **Push Constants:** Prepare and upload per-object data
- **State Management:** Bind pipeline and configure state

### Why Use Render Systems?

**Before (All rendering in one place):**
```cpp
class FirstApp {
    void renderGameObjects(VkCommandBuffer cb) {
        // All rendering logic mixed together
        pipeline->bind(cb);
        for (auto& obj : gameObjects) {
            // Push constants
            // Draw
        }
        // What about particles? UI? Different pipelines?
    }
};
```

**After (Separated by system):**
```cpp
class SimpleRenderSystem {
    void renderGameObjects(VkCommandBuffer cb, 
                          std::vector<GameObject>& gameObjects,
                          const Camera& camera);
};

class FirstApp {
    void run() {
        SimpleRenderSystem system{device, renderer.getSwapChainRenderPass()};
        Camera camera{};
        // render
        system.renderGameObjects(commandBuffer, gameObjects, camera);
    }
};
```

**Benefits:**
- **Separation of Concerns:** Each system handles one rendering approach
- **Modularity:** Add/remove systems independently
- **Reusability:** Systems can be used across multiple applications
- **Scalability:** Easy to add new rendering techniques
- **Testability:** Test each system in isolation
- **Performance:** Different pipelines optimized for different object types

---

## Class Interface

### Header Definition

```cpp
namespace engine {
  class SimpleRenderSystem {
  public:
    SimpleRenderSystem(Device& device, VkRenderPass renderPass);
    ~SimpleRenderSystem();

    SimpleRenderSystem(const SimpleRenderSystem &) = delete;
    SimpleRenderSystem &operator=(const SimpleRenderSystem &) = delete;

    void renderGameObjects(VkCommandBuffer commandBuffer, 
                          std::vector<GameObject>& gameObjects,
                          const Camera& camera);

  private:
    void createPipelineLayout();
    void createPipeline(VkRenderPass renderPass);

    Device& device;
    std::unique_ptr<Pipeline> pipeline;
    VkPipelineLayout pipelineLayout;
  };
}
```

### Constructor

```cpp
SimpleRenderSystem(Device& device, VkRenderPass renderPass);
```

**Purpose:** Initialize render system with device and compatible render pass.

**Parameters:**
- `device` - Reference to Device for Vulkan operations
- `renderPass` - Render pass this system will render into (from Renderer)

**Initialization Sequence:**
1. Store device reference
2. Call `createPipelineLayout()` - configure push constants and descriptors
3. Call `createPipeline()` - load shaders and create graphics pipeline

**Usage:**
```cpp
SimpleRenderSystem system{device, renderer.getSwapChainRenderPass()};
```

**Why Pass Render Pass?**
- Pipeline must be compatible with render pass
- Attachment formats, subpass index must match
- Allows system to work with any compatible render pass

### Destructor

```cpp
~SimpleRenderSystem();
```

**Cleanup:**
```cpp
vkDestroyPipelineLayout(device.device(), pipelineLayout, nullptr);
// Pipeline destroyed by unique_ptr automatically
```

**Note:** Pipeline object destroyed first (destructor order), then layout.

### Non-Copyable

```cpp
SimpleRenderSystem(const SimpleRenderSystem &) = delete;
SimpleRenderSystem &operator=(const SimpleRenderSystem &) = delete;
```

**Why No Copying?**
- Owns Vulkan resources (pipeline, pipeline layout)
- Copying would duplicate resource handles
- Systems are typically singletons per application

---

## Pipeline Management

### Pipeline Layout Creation

```cpp
void SimpleRenderSystem::createPipelineLayout() {
    // 1. Define push constant range
    VkPushConstantRange pushConstantRange{};
    pushConstantRange.stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;
    pushConstantRange.offset = 0;
    pushConstantRange.size = sizeof(SimplePushConstantData);

    // 2. Create pipeline layout
    VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
    pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    pipelineLayoutInfo.setLayoutCount = 0;
    pipelineLayoutInfo.pSetLayouts = nullptr;
    pipelineLayoutInfo.pushConstantRangeCount = 1;
    pipelineLayoutInfo.pPushConstantRanges = &pushConstantRange;

    if (vkCreatePipelineLayout(device.device(), &pipelineLayoutInfo, nullptr, &pipelineLayout) != VK_SUCCESS) {
        throw std::runtime_error("Failed to create pipeline layout!");
    }
}
```

**Push Constant Structure:**
```cpp
struct SimplePushConstantData {
    glm::mat4 transform{1.f};     // 4x4 transformation matrix (projection * model)
    alignas(16) glm::vec3 color;  // RGB color
};
```

**Configuration:**
- **Stage Flags:** Both vertex and fragment shaders can access
- **Offset:** 0 (first/only push constant range)
- **Size:** 80 bytes (mat4 + vec3 with alignment)

**No Descriptor Sets:**
- `setLayoutCount = 0` - not using textures or uniform buffers yet
- Future systems may add descriptor sets for textures

**See:** [PIPELINE.md](PIPELINE.md) and [SHADER.md](SHADER.md) for push constant details

### Pipeline Creation

```cpp
void SimpleRenderSystem::createPipeline(VkRenderPass renderPass) {
    assert(pipelineLayout != nullptr && "Cannot create pipeline before pipeline layout!");

    // 1. Get default pipeline configuration
    PipelineConfigInfo pipelineConfig{};
    Pipeline::defaultPipelineConfigInfo(pipelineConfig);
    
    // 2. Configure for this system
    pipelineConfig.renderPass = renderPass;
    pipelineConfig.pipelineLayout = pipelineLayout;
    
    // 3. Create pipeline with shaders
    pipeline = std::make_unique<Pipeline>(
        device,
        std::string(COMPILED_SHADERS_DIR) + "simple_shader.vert.spv",
        std::string(COMPILED_SHADERS_DIR) + "simple_shader.frag.spv",
        pipelineConfig);
}
```

**Configuration:**
- Uses default config (dynamic viewport/scissor, triangle list, etc.)
- Associates with specific render pass
- Associates with pipeline layout (for push constants)

**Shader Paths:**
- `COMPILED_SHADERS_DIR` - CMake-defined constant
- Points to `engine/shaders/bin/`
- Loads pre-compiled SPIR-V bytecode

**Assertion:**
- Ensures pipeline layout created first
- Pipeline needs layout reference during creation

---

## Rendering Implementation

### renderGameObjects()

```cpp
void SimpleRenderSystem::renderGameObjects(
    VkCommandBuffer commandBuffer, 
    std::vector<GameObject>& gameObjects,
    const Camera& camera) {
    
    // 1. Bind pipeline (once for all objects)
    pipeline->bind(commandBuffer);

    // 2. Render each game object
    for (auto& obj : gameObjects) {
        // 3. Update animation
        obj.transform.rotation.y = glm::mod(
            obj.transform.rotation.y + 0.01f, 
            glm::two_pi<float>()
        );

        // 4. Prepare push constants from GameObject
        SimplePushConstantData push{};
        push.color = obj.color;
        push.transform = camera.getProjection() * obj.transform.mat4();

        // 5. Upload push constants to GPU
        vkCmdPushConstants(
            commandBuffer,
            pipelineLayout,
            VK_SHADER_STAGE_FRAGMENT_BIT | VK_SHADER_STAGE_VERTEX_BIT,
            0,
            sizeof(SimplePushConstantData),
            &push);

        // 6. Bind model's vertex buffer
        obj.model->bind(commandBuffer);
        
        // 7. Issue draw call
        obj.model->draw(commandBuffer);
    }
}
```

### Rendering Flow Breakdown

#### 1. Pipeline Binding

```cpp
pipeline->bind(commandBuffer);
```

**Purpose:** Set active graphics pipeline for subsequent draw calls.

**Efficiency:** Bind once, draw many objects (all use same pipeline).

**State Set:**
- Shaders (vertex, fragment)
- Pipeline configuration (rasterization, blending, etc.)
- Pipeline layout (push constants, descriptors)

#### 2. Animation Update

```cpp
obj.transform.rotation.y = glm::mod(obj.transform.rotation.y + 0.01f, glm::two_pi<float>());
```

**Purpose:** Continuous Y-axis rotation animation (spinning around vertical axis).

**`glm::mod()`:** Wraps rotation to [0, 2π] range.

**Increment:** 0.01 radians per frame ≈ 0.57° per frame ≈ 34°/sec at 60 FPS.

**Note:** This is demo code. Production would use delta time:
```cpp
obj.transform.rotation.y += rotationSpeed * deltaTime;
```

#### 3. Push Constants Preparation

```cpp
SimplePushConstantData push{};
push.color = obj.color;
push.transform = camera.getProjection() * obj.transform.mat4();
```

**Data Extraction:**
- `color` → `push.color` (vec3 RGB, currently unused - vertex colors used instead)
- Combined transformation matrix → `push.transform` (4×4 projection * model matrix)

**Transform Matrix Generation:**

```cpp
// Model matrix from GameObject (scale, rotation, translation)
glm::mat4 modelMatrix = obj.transform.mat4();

// Projection matrix from Camera
glm::mat4 projectionMatrix = camera.getProjection();

// Combined transformation (projection applied after model)
push.transform = projectionMatrix * modelMatrix;
```

**Matrix Multiplication Order:**
- **Right-to-left:** Model transform applied first, then projection
- Vertex transformed from local space → world space → clip space

**See:** 
- [GAMEOBJECT.md](GAMEOBJECT.md) for transform details
- [CAMERA.md](CAMERA.md) for projection matrix details

#### 4. Push Constants Upload

```cpp
vkCmdPushConstants(
    commandBuffer,
    pipelineLayout,
    VK_SHADER_STAGE_FRAGMENT_BIT | VK_SHADER_STAGE_VERTEX_BIT,
    0,
    sizeof(SimplePushConstantData),
    &push);
```

**Parameters:**
- `commandBuffer` - Command buffer being recorded
- `pipelineLayout` - Layout containing push constant ranges
- `stageFlags` - Which shaders can access (vertex + fragment)
- `offset` - Byte offset into push constant block (0)
- `size` - Number of bytes to upload (40)
- `pValues` - Pointer to data

**Efficiency:**
- Fast GPU memory update (registers or cache)
- No buffer allocation/deallocation
- Ideal for small, per-draw data

#### 5. Model Binding

```cpp
obj.model->bind(commandBuffer);
```

**Purpose:** Bind vertex buffer for this GameObject's geometry.

**Implementation (in Model):**
```cpp
void Model::bind(VkCommandBuffer commandBuffer) {
    VkBuffer buffers[] = {vertexBuffer};
    VkDeviceSize offsets[] = {0};
    vkCmdBindVertexBuffers(commandBuffer, 0, 1, buffers, offsets);
}
```

**Multiple Objects:**
- Different GameObjects can share same Model (instancing)
- Bind called for each object (even if same model - driver optimizes)

#### 6. Draw Call

```cpp
obj.model->draw(commandBuffer);
```

**Purpose:** Issue actual draw command.

**Implementation (in Model):**
```cpp
void Model::draw(VkCommandBuffer commandBuffer) {
    vkCmdDraw(commandBuffer, vertexCount, 1, 0, 0);
}
```

**Parameters:**
- `vertexCount` - Number of vertices to draw
- `instanceCount` - 1 (not using instanced rendering)
- `firstVertex` - 0 (start at beginning of buffer)
- `firstInstance` - 0 (instance index)

---

## Design Patterns

### System Pattern (from ECS)

**Entity-Component-System Architecture:**
- **Entity:** GameObject (ID + components)
- **Component:** Transform2D, Model, Color (data)
- **System:** SimpleRenderSystem (logic that operates on components)

```
Systems (Logic)          Components (Data)
     ↓                         ↑
SimpleRenderSystem  ←→  GameObject{Transform2D, Model, Color}
ParticleSystem      ←→  ParticleEntity{Transform, ParticleData}
PhysicsSystem       ←→  RigidBody{Transform, PhysicsData}
```

**Benefits:**
- Data and logic separated
- Systems can be parallelized
- Easy to add/remove functionality
- Cache-friendly data layout (future optimization)

### Single Responsibility Principle

Each render system has ONE job:

- **SimpleRenderSystem:** Render basic geometry with transforms
- **ParticleSystem:** Render particle effects
- **UISystem:** Render user interface
- **TerrainSystem:** Render heightmap terrain

**Benefits:**
- Easy to understand
- Easy to test
- Easy to replace/modify

### Dependency Injection

```cpp
SimpleRenderSystem(Device& device, VkRenderPass renderPass);
```

**Dependencies provided by caller:**
- `Device` - Vulkan device operations
- `RenderPass` - Where to render

**Benefits:**
- System doesn't create dependencies
- Testable (can inject mocks)
- Flexible (works with any compatible render pass)

---

## Usage Examples

### Example 1: Basic Usage

```cpp
void FirstApp::run() {
    SimpleRenderSystem system{device, renderer.getSwapChainRenderPass()};
    Camera camera{};

    while (!window.shouldClose()) {
        glfwPollEvents();

        float aspect = renderer.getAspectRatio();
        camera.setPerspectiveProjection(glm::radians(50.0f), aspect, 0.1f, 10.0f);

        if (auto commandBuffer = renderer.beginFrame()) {
            renderer.beginSwapChainRenderPass(commandBuffer);
            system.renderGameObjects(commandBuffer, gameObjects, camera);
            renderer.endSwapChainRenderPass(commandBuffer);
            renderer.endFrame();
        }
    }

    vkDeviceWaitIdle(device.device());
}
```

### Example 2: Multiple Systems

```cpp
void FirstApp::run() {
    SimpleRenderSystem simpleSystem{device, renderer.getSwapChainRenderPass()};
    ParticleSystem particleSystem{device, renderer.getSwapChainRenderPass()};

    while (!window.shouldClose()) {
        glfwPollEvents();

        if (auto commandBuffer = renderer.beginFrame()) {
            renderer.beginSwapChainRenderPass(commandBuffer);
            
            // Render opaque objects first
            simpleSystem.renderGameObjects(commandBuffer, gameObjects);
            
            // Render transparent particles after
            particleSystem.render(commandBuffer, particles);
            
            renderer.endSwapChainRenderPass(commandBuffer);
            renderer.endFrame();
        }
    }

    vkDeviceWaitIdle(device.device());
}
```

### Example 3: Conditional Rendering

```cpp
void FirstApp::run() {
    SimpleRenderSystem gameSystem{device, renderer.getSwapChainRenderPass()};
    DebugRenderSystem debugSystem{device, renderer.getSwapChainRenderPass()};

    while (!window.shouldClose()) {
        glfwPollEvents();
        processInput();

        if (auto commandBuffer = renderer.beginFrame()) {
            renderer.beginSwapChainRenderPass(commandBuffer);
            
            gameSystem.renderGameObjects(commandBuffer, gameObjects);
            
            if (showDebugInfo) {
                debugSystem.renderColliders(commandBuffer, gameObjects);
                debugSystem.renderGrid(commandBuffer);
            }
            
            renderer.endSwapChainRenderPass(commandBuffer);
            renderer.endFrame();
        }
    }

    vkDeviceWaitIdle(device.device());
}
```

---

## Extending with New Systems

### Creating a Custom Render System

**Step 1: Define the System Class**

```cpp
// ParticleSystem.hpp
class ParticleSystem {
public:
    ParticleSystem(Device& device, VkRenderPass renderPass);
    ~ParticleSystem();

    void render(VkCommandBuffer commandBuffer, std::vector<Particle>& particles);

private:
    void createPipelineLayout();
    void createPipeline(VkRenderPass renderPass);

    Device& device;
    std::unique_ptr<Pipeline> pipeline;
    VkPipelineLayout pipelineLayout;
};
```

**Step 2: Implement Pipeline Creation**

```cpp
void ParticleSystem::createPipelineLayout() {
    // Define push constants for particles
    VkPushConstantRange pushConstantRange{};
    pushConstantRange.stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;
    pushConstantRange.offset = 0;
    pushConstantRange.size = sizeof(ParticlePushConstants);

    VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
    pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    pipelineLayoutInfo.pushConstantRangeCount = 1;
    pipelineLayoutInfo.pPushConstantRanges = &pushConstantRange;

    if (vkCreatePipelineLayout(device.device(), &pipelineLayoutInfo, nullptr, &pipelineLayout) != VK_SUCCESS) {
        throw std::runtime_error("Failed to create pipeline layout!");
    }
}

void ParticleSystem::createPipeline(VkRenderPass renderPass) {
    PipelineConfigInfo pipelineConfig{};
    Pipeline::defaultPipelineConfigInfo(pipelineConfig);
    
    // Customize for particles
    pipelineConfig.inputAssemblyInfo.topology = VK_PRIMITIVE_TOPOLOGY_POINT_LIST;
    pipelineConfig.colorBlendAttachment.blendEnable = VK_TRUE;  // Enable blending
    pipelineConfig.renderPass = renderPass;
    pipelineConfig.pipelineLayout = pipelineLayout;
    
    pipeline = std::make_unique<Pipeline>(
        device,
        "shaders/particle.vert.spv",
        "shaders/particle.frag.spv",
        pipelineConfig);
}
```

**Step 3: Implement Rendering Logic**

```cpp
void ParticleSystem::render(VkCommandBuffer commandBuffer, std::vector<Particle>& particles) {
    pipeline->bind(commandBuffer);

    for (auto& particle : particles) {
        // Update particle physics
        particle.position += particle.velocity * deltaTime;
        particle.life -= deltaTime;

        if (particle.life <= 0.0f) continue;  // Skip dead particles

        // Prepare push constants
        ParticlePushConstants push{};
        push.position = particle.position;
        push.color = particle.color;
        push.size = particle.size;

        vkCmdPushConstants(commandBuffer, pipelineLayout,
                          VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT,
                          0, sizeof(ParticlePushConstants), &push);

        // Draw particle (using point primitive)
        vkCmdDraw(commandBuffer, 1, 1, 0, 0);
    }
}
```

**Step 4: Use in Application**

```cpp
ParticleSystem particleSystem{device, renderer.getSwapChainRenderPass()};
system.render(commandBuffer, particles);
```

### System Design Checklist

When creating a new render system:

- ✅ **Constructor:** Takes `Device&` and `VkRenderPass`
- ✅ **Pipeline Layout:** Define push constants and descriptor sets
- ✅ **Pipeline:** Create with appropriate shaders and configuration
- ✅ **Render Method:** Takes `VkCommandBuffer` and entity collection
- ✅ **Bind Pipeline:** Once at start of render method
- ✅ **Update Logic:** Per-entity updates (animation, physics, etc.)
- ✅ **Push Constants:** Per-entity data upload
- ✅ **Draw Calls:** Issue draws for each entity
- ✅ **Destructor:** Clean up pipeline layout (pipeline auto-deleted)

---

## Common Issues

### Issue 1: Pipeline Creation Fails

**Error:**
```
Failed to create pipeline layout!
```

**Possible Causes:**
1. Push constant size exceeds limit (>128 bytes)
2. Invalid stage flags
3. Pipeline layout created after pipeline

**Solution:**
```cpp
// Check push constant size
static_assert(sizeof(SimplePushConstantData) <= 128, "Push constants too large!");

// Ensure layout created first
void SimpleRenderSystem::SimpleRenderSystem(Device& device, VkRenderPass renderPass) {
    createPipelineLayout();  // Must be first!
    createPipeline(renderPass);
}
```

### Issue 2: Nothing Renders

**Symptom:** No objects visible on screen.

**Debugging Steps:**

1. **Check pipeline binding:**
```cpp
pipeline->bind(commandBuffer);  // Must be before draw calls
```

2. **Check model binding:**
```cpp
obj.model->bind(commandBuffer);  // Must be before each draw
```

3. **Check push constants uploaded:**
```cpp
vkCmdPushConstants(...);  // Must be before draw
```

4. **Verify draw call order:**
```cpp
// Correct order:
pipeline->bind(commandBuffer);
for (obj in objects) {
    vkCmdPushConstants(...);
    obj.model->bind(commandBuffer);
    obj.model->draw(commandBuffer);
}
```

### Issue 3: Validation Layer Errors

**Error:**
```
vkCmdDraw(): Pipeline is not compatible with render pass
```

**Cause:** Pipeline created with different render pass than currently bound.

**Solution:**
```cpp
// Ensure same render pass used
SimpleRenderSystem system{device, renderer.getSwapChainRenderPass()};
//                                ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
//                                Same render pass passed to renderer.beginSwapChainRenderPass()
```

### Issue 4: Objects Flicker or Wrong Colors

**Symptom:** Incorrect push constant data in shaders.

**Debugging:**

1. **Check struct alignment:**
```cpp
struct SimplePushConstantData {
    glm::mat2 transform{1.f};
    glm::vec2 offset;
    alignas(16) glm::vec3 color;  // Must align!
};
```

2. **Verify data before upload:**
```cpp
SimplePushConstantData push{};
push.transform = obj.transform2D.mat2();
std::cout << "Transform: " << push.transform[0][0] << "," << push.transform[0][1] << std::endl;
```

3. **Check shader declaration matches:**
```glsl
// Must match C++ struct exactly
layout(push_constant) uniform Push {
  mat2 transform;
  vec2 offset;
  vec3 color;
} push;
```

---

## Related Documentation

- [RENDERER.md](RENDERER.md) - Renderer abstraction and frame lifecycle
- [GAMEOBJECT.md](GAMEOBJECT.md) - GameObject and Transform2D components
- [PIPELINE.md](PIPELINE.md) - Pipeline configuration
- [SHADER.md](SHADER.md) - Shader implementation
- [ARCHITECTURE.md](ARCHITECTURE.md) - Overall system architecture

---

