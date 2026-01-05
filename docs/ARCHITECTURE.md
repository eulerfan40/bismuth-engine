# Architecture Overview

High-level overview of the Bismuth Engine architecture, design decisions, and system interactions.

**Version:** 0.1.0

---

## Table of Contents

- [System Overview](#system-overview)
- [Core Components](#core-components)
- [Initialization Flow](#initialization-flow)
- [Render Loop](#render-loop)
- [Memory Management](#memory-management)
- [Design Principles](#design-principles)

---

## System Overview

The Bismuth Engine is a custom game engine built from scratch using modern C++20 and the Vulkan graphics API. The architecture follows a modular design where each major system (windowing, device management, rendering pipeline, swapchain) is encapsulated in its own class.

### High-Level Architecture Diagram

```
┌─────────────────────────────────────────────────────────┐
│                      FirstApp                           │
│  (Application Layer - Orchestrates everything)          │
└────────────┬─────────────────────────────┬──────────────┘
             │                             │
             ▼                             ▼
    ┌────────────────┐           ┌─────────────────┐
    │     Window     │           │     Device      │
    │  (GLFW + volk) │           │ (Vulkan Setup)  │
    └────────────────┘           └────────┬────────┘
                                          │
                                          ▼
                          ┌───────────────────────────┐
                          │       SwapChain           │
                          │ (Frame Management)        │
                          └───────────┬───────────────┘
                                      │
                                      ▼
                          ┌───────────────────────────┐
                          │       Pipeline            │
                          │ (Shaders + Config)        │
                          └───────────────────────────┘
```

### Component Responsibilities

| Component | Responsibility | Dependencies |
|-----------|---------------|--------------|
| **FirstApp** | Application lifecycle, render loop coordination | Window, Device, SwapChain, Pipeline, Model |
| **Window** | Window creation, input handling, Vulkan surface | volk, GLFW |
| **Device** | Vulkan instance, physical/logical device, queues | Window |
| **SwapChain** | Framebuffers, render passes, frame synchronization | Device |
| **Pipeline** | Shader loading, graphics pipeline configuration | Device, Model |
| **Model** | Vertex data management, buffer creation, draw commands | Device |

---

## Core Components

### 1. Window System

**Purpose:** Manages the application window and provides a Vulkan rendering surface.

**Key Technologies:**
- **GLFW:** Cross-platform window management
- **volk:** Dynamic Vulkan function loading

**Initialization Order:**
1. `volkInitialize()` - Load base Vulkan loader
2. `glfwInit()` - Initialize GLFW library
3. `glfwCreateWindow()` - Create OS window
4. `glfwCreateWindowSurface()` - Create Vulkan surface for rendering

**Why volk?**
- No Vulkan SDK required for building/running
- Dynamic loading allows runtime Vulkan detection
- Enables graceful degradation if Vulkan unavailable

### 2. Device Management

**Purpose:** Initializes Vulkan, selects GPU, creates logical device, manages queues.

**Vulkan Objects Managed:**
- `VkInstance` - Vulkan API connection
- `VkPhysicalDevice` - GPU hardware handle
- `VkDevice` - Logical device interface
- `VkQueue` - Command submission queues (graphics, present)
- `VkCommandPool` - Command buffer allocation pool

**Initialization Stages:**

```
volkInitialize() (in Window)
        ↓
createInstance()
        ↓
volkLoadInstance() ← Load instance-level functions
        ↓
setupDebugMessenger() ← Enable validation layers (debug only)
        ↓
createSurface() ← Get window surface from GLFW
        ↓
pickPhysicalDevice() ← Select GPU
        ↓
createLogicalDevice()
        ↓
volkLoadDevice() ← Load device-level functions
        ↓
createCommandPool() ← Allocate command buffer pool
```

**Queue Families:**
- **Graphics Queue:** Executes rendering commands
- **Present Queue:** Presents images to screen (may be same as graphics)

**Why Separate Physical and Logical Device?**
- Physical Device = GPU hardware
- Logical Device = Interface to GPU with specific features enabled
- Allows selecting features/extensions per application need

### 3. SwapChain System

**Purpose:** Manages the image buffers that get presented to the screen.

**Key Concepts:**

**Swapchain Images:**
- Multiple framebuffers (typically 2-3) for double/triple buffering
- Prevents tearing by rendering to off-screen buffer
- One image shown while another is being rendered

**Synchronization Objects:**
```cpp
MAX_FRAMES_IN_FLIGHT = 3  // Matches swapchain image count
```

| Object | Purpose | Count |
|--------|---------|-------|
| `imageAvailableSemaphores` | Signals when swapchain image ready | 3 (per frame) |
| `renderFinishedSemaphores` | Signals when rendering complete | 3 (per frame) |
| `inFlightFences` | CPU-side synchronization | 3 (per frame) |

**Why 3 Frames in Flight?**
- Typical swapchain has 3 images (min 2 + 1)
- Each image needs dedicated synchronization objects
- Prevents semaphore reuse conflicts
- See [Synchronization Details](#synchronization-deep-dive)

**Render Pass:**
- Describes framebuffer attachments (color, depth)
- Defines clear operations and layout transitions
- Subpass dependencies ensure correct ordering

### 4. Graphics Pipeline

**Purpose:** Configures the rendering pipeline from vertices to pixels.

**Pipeline Stages:**

```
Vertex Shader (simple_shader.vert)
        ↓
Input Assembly (triangle list)
        ↓
Viewport Transform
        ↓
Rasterization (fill triangles)
        ↓
Fragment Shader (simple_shader.frag)
        ↓
Depth Testing
        ↓
Color Blending (disabled - direct write)
        ↓
Framebuffer Output
```

**Shader Compilation:**
- Source: GLSL (`.vert`, `.frag`)
- Compiler: `glslc` (from Vulkan SDK)
- Output: SPIR-V bytecode (`.spv`)
- Loaded at runtime via `vkCreateShaderModule()`

**Why SPIR-V?**
- Platform-independent intermediate representation
- Validated once, runs anywhere
- Faster loading than parsing GLSL at runtime

### 4. Renderer

**Purpose:** High-level rendering abstraction managing frame lifecycle and command buffers.

**Key Responsibilities:**
- Frame lifecycle management (begin/end frame)
- Command buffer allocation and management
- Swapchain creation and recreation
- Render pass control (begin/end)
- State tracking and validation

**Class Structure:**
```cpp
class Renderer {
public:
    Renderer(Window& window, Device& device);
    
    VkCommandBuffer beginFrame();
    void endFrame();
    void beginSwapChainRenderPass(VkCommandBuffer commandBuffer);
    void endSwapChainRenderPass(VkCommandBuffer commandBuffer);
    
    VkRenderPass getSwapChainRenderPass() const;
    bool isFrameInProgress() const;
    VkCommandBuffer getCurrentCommandBuffer() const;
    int getFrameIndex() const;

private:
    void createCommandBuffers();
    void freeCommandBuffers();
    void recreateSwapChain();

    Window& window;
    Device& device;
    std::unique_ptr<SwapChain> swapChain;
    std::vector<VkCommandBuffer> commandBuffers;
    bool isFrameStarted{false};
};
```

**Frame Lifecycle:**

```
[Idle] → beginFrame() → [Recording] → endFrame() → [Idle]
           ↓                             ↑
       Returns CommandBuffer         Submits & Presents
```

**Typical Usage:**
```cpp
Renderer renderer{window, device};

while (!window.shouldClose()) {
    glfwPollEvents();
    
    if (auto commandBuffer = renderer.beginFrame()) {
        renderer.beginSwapChainRenderPass(commandBuffer);
        // Render here
        renderer.endSwapChainRenderPass(commandBuffer);
        renderer.endFrame();
    }
}
```

**Design Benefits:**
- **Separation of Concerns:** FirstApp no longer manages swapchain/command buffers
- **Reusability:** Can be used by multiple application classes
- **Safety:** Assertions prevent invalid state transitions
- **Simplicity:** Clean API hides Vulkan complexity

**Key Features:**
- Automatic swapchain recreation on window resize
- Proper frame synchronization (triple buffering)
- Minimization handling (pauses rendering when window minimized)
- Format verification after swapchain recreation

**See:** [RENDERER.md](RENDERER.md) for complete Renderer documentation

### 5. Render Systems

**Purpose:** Specialized rendering logic for specific categories of objects.

**Key Responsibilities:**
- Pipeline management (create and own graphics pipeline)
- Shader loading (specific to object type)
- Rendering logic (iterate over objects, issue draw commands)
- Push constants (prepare and upload per-object data)

**Pattern:** System pattern from Entity-Component-System (ECS) architecture.

**SimpleRenderSystem Example:**
```cpp
class SimpleRenderSystem {
public:
    SimpleRenderSystem(Device& device, VkRenderPass renderPass);
    
    void renderGameObjects(VkCommandBuffer commandBuffer, 
                          std::vector<GameObject>& gameObjects);

private:
    void createPipelineLayout();
    void createPipeline(VkRenderPass renderPass);

    Device& device;
    std::unique_ptr<Pipeline> pipeline;
    VkPipelineLayout pipelineLayout;
};
```

**Rendering Flow:**
```cpp
void SimpleRenderSystem::renderGameObjects(VkCommandBuffer commandBuffer, 
                                          std::vector<GameObject>& gameObjects) {
    // 1. Bind pipeline once for all objects
    pipeline->bind(commandBuffer);

    // 2. Render each game object
    for (auto& obj : gameObjects) {
        // 3. Update animation
        obj.transform2D.rotation += 0.01f;

        // 4. Prepare push constants from GameObject
        SimplePushConstantData push{};
        push.offset = obj.transform2D.translation;
        push.color = obj.color;
        push.transform = obj.transform2D.mat2();

        // 5. Upload push constants to GPU
        vkCmdPushConstants(commandBuffer, pipelineLayout, ...);

        // 6. Bind model's vertex buffer
        obj.model->bind(commandBuffer);
        
        // 7. Issue draw call
        obj.model->draw(commandBuffer);
    }
}
```

**Architecture Pattern:**

```
Systems (Logic)          Components (Data)
     ↓                         ↑
SimpleRenderSystem  ←→  GameObject{Transform2D, Model, Color}
ParticleSystem      ←→  ParticleEntity{Transform, ParticleData}
PhysicsSystem       ←→  RigidBody{Transform, PhysicsData}
```

**Multiple Render Systems:**
```cpp
void FirstApp::run() {
    SimpleRenderSystem simpleSystem{device, renderer.getSwapChainRenderPass()};
    ParticleSystem particleSystem{device, renderer.getSwapChainRenderPass()};
    UISystem uiSystem{device, renderer.getSwapChainRenderPass()};

    while (!window.shouldClose()) {
        if (auto commandBuffer = renderer.beginFrame()) {
            renderer.beginSwapChainRenderPass(commandBuffer);
            
            // Render in order
            simpleSystem.renderGameObjects(commandBuffer, gameObjects);
            particleSystem.render(commandBuffer, particles);
            uiSystem.render(commandBuffer, uiElements);
            
            renderer.endSwapChainRenderPass(commandBuffer);
            renderer.endFrame();
        }
    }
}
```

**Design Benefits:**
- **Single Responsibility:** Each system handles one rendering approach
- **Modularity:** Add/remove systems independently
- **Scalability:** Easy to add new rendering techniques
- **Testability:** Test each system in isolation
- **Performance:** Different pipelines optimized for different object types

**See:** [RENDERSYSTEM.md](RENDERSYSTEM.md) for complete render system documentation

### 6. Model System

**Purpose:** Manages vertex data and vertex buffers for rendering geometry.

**Key Responsibilities:**
- Store vertex data in GPU memory
- Define vertex input layout (binding and attribute descriptions)
- Bind vertex buffers to command buffers
- Issue draw commands

**Vertex Structure:**
```cpp
struct Vertex {
    glm::vec2 position;  // 2D position
    glm::vec3 color;     // RGB color per vertex
};
```

**Buffer Management:**
- Creates `VkBuffer` for vertex data
- Allocates `VkDeviceMemory` (host-visible, coherent)
- Maps memory, copies data, unmaps
- RAII cleanup in destructor

**Vertex Input Descriptors:**
- Binding descriptions: How to read buffer (stride, input rate)
- Attribute descriptions: How to interpret data (format, location, offset)
  - Location 0: `position` (vec2, R32G32_SFLOAT)
  - Location 1: `color` (vec3, R32G32B32_SFLOAT)
- Used by Pipeline during creation

**Rendering Commands:**
```cpp
model->bind(commandBuffer);   // Bind vertex buffer
model->draw(commandBuffer);   // Issue draw call
```

**Design Decision: Hardcoded Vertices → Vertex Buffers**

Previously, vertex positions were hardcoded in the vertex shader:
```glsl
vec2 positions[3] = vec2[](vec2(0.0, -0.5), vec2(0.5, 0.5), vec2(-0.5, 0.5));
gl_Position = vec4(positions[gl_VertexIndex], 0.0, 1.0);
```

Now, vertices are loaded from CPU and passed via vertex buffers:
```cpp
std::vector<Model::Vertex> vertices {
    {{0.0f, -0.5f}, {1.0f, 0.0f, 0.0f}},  // Red
    {{0.5f, 0.5f}, {0.0f, 1.0f, 0.0f}},   // Green
    {{-0.5f, 0.5f}, {0.0f, 0.0f, 1.0f}}   // Blue
};
model = std::make_unique<Model>(device, vertices);
```

**Why this change?**
- **Flexibility:** Load geometry and colors from files, not hardcoded
- **Scalability:** Multiple models can share shaders
- **Industry standard:** Proper 3D engines use vertex buffers with attributes

**Color Interpolation:**
Per-vertex colors are automatically interpolated across triangle faces by the GPU rasterizer. The vertex shader outputs colors, which are then smoothly blended between vertices during rasterization, and the fragment shader receives the interpolated color for each pixel. This creates smooth color gradients without additional computation.

### 6. GameObject System

**Purpose:** Entity representation with transform, rendering, and identity.

**Key Responsibilities:**
- Unique identification for scene management
- Encapsulate model, color, and transform data
- Enable component-based architecture
- Support multiple instances of shared geometry

**Class Structure:**
```cpp
class GameObject {
public:
    using id_t = unsigned int;
    
    static GameObject createGameObject();  // Factory method
    
    const id_t getId() { return id; }
    
    std::shared_ptr<Model> model{};
    glm::vec3 color{};
    Transform2DComponent transform2D{};
    
private:
    GameObject(id_t objId) : id{objId} {}
    id_t id;
};
```

**Transform2DComponent:**
```cpp
struct Transform2DComponent {
    glm::vec2 translation{};      // Position offset
    glm::vec2 scale{1.f, 1.f};    // Scale factor
    float rotation;               // Rotation in radians
    
    glm::mat2 mat2() {            // Generate transformation matrix
        const float s = glm::sin(rotation);
        const float c = glm::cos(rotation);
        glm::mat2 rotMatrix{{c, s}, {-s, c}};
        glm::mat2 scaleMat{{scale.x, 0.0f}, {0.0f, scale.y}};
        return rotMatrix * scaleMat;
    }
};
```

**Creation Pattern:**
```cpp
auto model = std::make_shared<Model>(device, vertices);

auto triangle = GameObject::createGameObject();
triangle.model = model;
triangle.color = {0.1f, 0.8f, 0.1f};
triangle.transform2D.translation = {0.2f, 0.0f};
triangle.transform2D.scale = {2.0f, 0.5f};
triangle.transform2D.rotation = glm::radians(90.0f);

gameObjects.push_back(std::move(triangle));
```

**Rendering Integration:**
```cpp
for (auto& obj : gameObjects) {
    // Update animation
    obj.transform2D.rotation += 0.01f;
    
    // Build push constants from GameObject
    SimplePushConstantData push{};
    push.transform = obj.transform2D.mat2();
    push.offset = obj.transform2D.translation;
    push.color = obj.color;
    
    // Upload and render
    vkCmdPushConstants(commandBuffer, ...);
    obj.model->bind(commandBuffer);
    obj.model->draw(commandBuffer);
}
```

**Design Benefits:**
- **Separation of concerns:** Model (geometry) vs GameObject (instance data)
- **Memory efficiency:** Multiple objects share same Model
- **Flexibility:** Easy to add/remove objects from scene
- **Animation:** Transform changes per frame without touching vertex data
- **Extensibility:** Foundation for entity-component system (ECS)

**See:** [GAMEOBJECT.md](GAMEOBJECT.md) for complete GameObject documentation

### 7. Model System

**Purpose:** Vertex buffer management for 3D geometry.

**Key Responsibilities:**
- Store vertex data (positions, colors) in GPU memory
- Provide vertex layout descriptions for pipeline
- Bind vertex buffers for rendering
- Support per-vertex attributes

**Vertex Structure:**
```cpp
struct Vertex {
    glm::vec3 position;  // 3D position (x, y, z)
    glm::vec3 color;     // RGB color per vertex
};
```

**3D Geometry Example - Cube:**
```cpp
std::vector<Model::Vertex> vertices{
    // left face (white)
    {{-.5f, -.5f, -.5f}, {.9f, .9f, .9f}},
    {{-.5f, .5f, .5f}, {.9f, .9f, .9f}},
    {{-.5f, -.5f, .5f}, {.9f, .9f, .9f}},
    // ... 36 vertices total (6 faces × 2 triangles × 3 vertices)
};
```

**Design Benefits:**
- **Per-vertex colors:** Each vertex can have different color, enabling gradients
- **Reusable geometry:** Multiple GameObjects can share same Model
- **GPU-resident:** Vertex data stored in GPU memory for fast rendering
- **Standard format:** Compatible with typical 3D model formats

**See:** [MODEL.md](MODEL.md) for complete Model documentation

### 8. Camera System

**Purpose:** Projection and view matrix management for 3D rendering.

**Key Responsibilities:**
- Generate orthographic or perspective projection matrices
- Generate view matrices for camera position and orientation
- Handle aspect ratio corrections for window dimensions
- Define view frustum (near/far clipping planes)
- Convert world coordinates to camera space and then to clip space

**Projection Types:**

**Orthographic (Parallel) Projection:**
```cpp
Camera camera;
float aspect = renderer.getAspectRatio();
camera.setOrthographicProjection(-aspect, aspect, -1.0f, 1.0f, -1.0f, 1.0f);
```
- No perspective distortion
- Objects same size regardless of distance
- Perfect for 2D games, UI, isometric views

**Perspective Projection:**
```cpp
Camera camera;
float aspect = renderer.getAspectRatio();
camera.setPerspectiveProjection(glm::radians(50.0f), aspect, 0.1f, 10.0f);
```
- Realistic 3D rendering
- Distant objects appear smaller
- Field of view controls "zoom level"

**View Transformations:**

The camera also supports positioning and orienting the camera in 3D space through view transformations:

**1. Look at Target:**
```cpp
camera.setViewTarget(
    glm::vec3{-1.0f, -2.0f, 2.0f},  // Camera position
    glm::vec3{0.0f, 0.0f, 2.5f},    // Look at this point
    glm::vec3{0.0f, -1.0f, 0.0f}    // Up direction (Vulkan Y-down)
);
```
- Camera positioned at specified point, looking at target
- Perfect for orbit cameras and object following

**2. Look in Direction:**
```cpp
camera.setViewDirection(
    glm::vec3{0.0f, 0.0f, -3.0f},   // Camera position
    glm::vec3{0.0f, 0.0f, 1.0f},    // Look direction (forward)
    glm::vec3{0.0f, -1.0f, 0.0f}    // Up direction
);
```
- Camera positioned with explicit look direction
- Useful for free-flying cameras

**3. Euler Angle Rotation:**
```cpp
camera.setViewYXZ(
    glm::vec3{0.0f, 0.0f, -3.0f},   // Camera position
    glm::vec3{0.0f, 0.0f, 0.0f}     // Rotation (pitch, yaw, roll)
);
```
- Camera controlled by rotation angles
- Perfect for FPS-style mouse look

**Integration with Rendering:**
```cpp
// In FirstApp::run()
Camera camera{};
float aspect = renderer.getAspectRatio();
camera.setPerspectiveProjection(glm::radians(50.0f), aspect, 0.1f, 10.0f);
camera.setViewTarget(glm::vec3{-1.0f, -2.0f, 2.0f}, glm::vec3{0.0f, 0.0f, 2.5f});

// Pass camera to render system
simpleRenderSystem.renderGameObjects(commandBuffer, gameObjects, camera);

// In render system - combine projection, view, and model transforms
auto projectionView = camera.getProjection() * camera.getView();  // Once per frame
push.transform = projectionView * obj.transform.mat4();            // Per object
```

**Transformation Pipeline:**
```
Vertex (local space)
    ↓ [Model Matrix - GameObject transform]
World Space (scene coordinates)
    ↓ [View Matrix - Camera position/orientation]
Camera Space (relative to camera)
    ↓ [Projection Matrix - Camera frustum]
Clip Space (homogeneous coordinates)
    ↓ [Perspective Division - GPU automatic]
NDC (Normalized Device Coordinates)
    ↓ [Viewport Transform - GPU automatic]
Screen Space (pixels)
```

**Design Benefits:**
- **Natural coordinates:** Use world units instead of normalized clip space
- **Realistic rendering:** Perspective makes distant objects smaller
- **Flexible viewing:** Easy to switch projection types and camera positions
- **Aspect ratio handling:** Prevents distortion on non-square screens
- **Camera movement:** Position and orient camera freely in 3D space
- **Performance optimization:** Pre-compute projection-view matrix once per frame

**See:** [CAMERA.md](CAMERA.md) for complete Camera documentation

### 9. Push Constants

**Purpose:** Fast CPU-to-GPU data transfer for per-draw-call parameters.

**Key Responsibilities:**
- Define shader interface for small, frequently-changing data
- Transfer GameObject transform and color to GPU
- Support multiple draw calls with different parameters
- Minimal overhead compared to uniform buffers

**Data Structure:**
```cpp
struct SimplePushConstantData {
    glm::mat4 transform{1.f};    // 4x4 transformation matrix (projection * model)
    alignas(16) glm::vec3 color; // RGB color (16-byte aligned!)
};
```

**Memory Layout:**
```
Offset  Size  Field
0x00    64    mat4 transform (projection * model matrix)
0x40    12    vec3 color (aligned to 16)
Total: 80 bytes
```

**Pipeline Layout Configuration:**
```cpp
VkPushConstantRange pushConstantRange{};
pushConstantRange.stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;
pushConstantRange.offset = 0;
pushConstantRange.size = sizeof(SimplePushConstantData);
```

**GameObject to Push Constants:**
```cpp
// Optimization: Compute projection-view once per frame
auto projectionView = camera.getProjection() * camera.getView();

// Per object: Combine with model transform
SimplePushConstantData push{};
push.transform = projectionView * obj.transform.mat4();  // (Projection * View) * Model
push.color = obj.color;  // Color (currently unused - vertex colors used instead)

vkCmdPushConstants(commandBuffer, pipelineLayout, 
                   VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT,
                   0, sizeof(SimplePushConstantData), &push);
```

**Shader Usage:**
```glsl
// Vertex shader
layout(push_constant) uniform Push {
  mat4 transform;
  vec3 color;
} push;

gl_Position = push.transform * vec4(position, 1.0);
                   ^^^^^^^^^^^^^^   ^^^^^^^^^^^^^^^^
                   Combined matrix  Vertex promoted to vec4

// Fragment shader
outColor = vec4(push.color, 1.0);
```

**Transformation Order:**
1. Vertex position (local/model space)
2. Apply model matrix (scale, rotation, translation to world space)
3. Apply view matrix (transform relative to camera position/orientation)
4. Apply projection matrix (perspective or orthographic projection)
5. Result in clip space coordinates (homogeneous coords with w component)
6. GPU automatically performs perspective division (x/w, y/w, z/w)

**Why `alignas(16)` for vec3?**
- GLSL std140 layout requires vec3 to be 16-byte aligned
- Without alignment, GPU reads wrong memory offsets
- Results in incorrect colors or validation errors

**See:** [SHADER.md](SHADER.md) for complete shader implementation details

---

## Initialization Flow

### Complete Startup Sequence

```cpp
main()
  ↓
FirstApp::FirstApp()
  ├→ Window(width, height, name)
  │   ├→ volkInitialize()           // Load Vulkan loader
  │   ├→ glfwInit()                 // Initialize GLFW
  │   └→ glfwCreateWindow()         // Create OS window
  │
  ├→ Device(window)
  │   ├→ createInstance()
  │   │   ├→ Check validation layers
  │   │   ├→ vkCreateInstance()
  │   │   └→ volkLoadInstance()     // Load instance functions
  │   ├→ setupDebugMessenger()      // Validation layers
  │   ├→ createSurface()            // Window surface
  │   ├→ pickPhysicalDevice()       // Select GPU
  │   ├→ createLogicalDevice()
  │   │   └→ volkLoadDevice()       // Load device functions
  │   └→ createCommandPool()        // Command buffer allocation
  │
  ├→ Renderer(window, device)
  │   ├→ recreateSwapChain()
  │   │   └→ SwapChain(device, window extent)
  │   │       ├→ createSwapChain()          // Create swapchain images
  │   │       ├→ createImageViews()         // Wrap images in views
  │   │       ├→ createRenderPass()         // Define render pass
  │   │       ├→ createDepthResources()     // Depth buffer
  │   │       ├→ createFramebuffers()       // Per-image framebuffers
  │   │       └→ createSyncObjects()        // Semaphores + fences
  │   └→ createCommandBuffers()
  │
  └→ loadGameObjects()
      └→ GameObject::createGameObject()
          ├→ Assign Model (shared geometry)
          ├→ Set color
          └→ Set transform (translation, rotation, scale)

FirstApp::run()
  ↓
  ├→ Create render systems
  │   └→ SimpleRenderSystem(device, renderer.getSwapChainRenderPass())
  │       ├→ createPipelineLayout()         // Push constants
  │       └→ createPipeline()
  │           ├→ Read shader files (.spv)
  │           ├→ createShaderModule() (vert)
  │           ├→ createShaderModule() (frag)
  │           ├→ Get vertex input descriptors from Model
  │           └→ vkCreateGraphicsPipelines()
  │
  └→ Main loop
      ├→ glfwPollEvents()
      └→ renderer.beginFrame()
          ├→ renderer.beginSwapChainRenderPass()
          ├→ renderSystem.renderGameObjects()  // Render all systems
          ├→ renderer.endSwapChainRenderPass()
          └→ renderer.endFrame()
```

---

## Render Loop

### Frame Rendering Sequence (New Architecture)

```cpp
void FirstApp::run() {
    SimpleRenderSystem renderSystem{device, renderer.getSwapChainRenderPass()};

    while (!window.shouldClose()) {
        glfwPollEvents();
        
        if (auto commandBuffer = renderer.beginFrame()) {
            renderer.beginSwapChainRenderPass(commandBuffer);
            renderSystem.renderGameObjects(commandBuffer, gameObjects);
            renderer.endSwapChainRenderPass(commandBuffer);
            renderer.endFrame();
        }
    }
    
    vkDeviceWaitIdle(device.device());
}
```

### Frame Breakdown (Renderer-Based)

```cpp
1. renderer.beginFrame()
   ├→ assert(!isFrameStarted)  // Validate state
   ├→ swapChain->acquireNextImage(&currentImageIndex)
   │  ├→ vkWaitForFences(inFlightFences[currentFrame])
   │  └→ vkAcquireNextImageKHR(imageAvailableSemaphores[currentFrame])
   │     ├→ Check for VK_ERROR_OUT_OF_DATE_KHR → recreateSwapChain(), return nullptr
   │     └→ Returns imageIndex (0, 1, or 2)
   ├→ isFrameStarted = true
   ├→ Get current command buffer
   ├→ vkBeginCommandBuffer()
   └→ Return commandBuffer

2. renderer.beginSwapChainRenderPass(commandBuffer)
   ├→ assert(isFrameStarted)  // Validate state
   ├→ vkCmdBeginRenderPass()
   │  ├→ Set clear values (color, depth)
   │  └→ Bind framebuffer for currentImageIndex
   ├→ vkCmdSetViewport() (dynamic state)
   └→ vkCmdSetScissor() (dynamic state)

3. renderSystem.renderGameObjects(commandBuffer, gameObjects)
   ├→ pipeline->bind(commandBuffer)
   └→ For each GameObject:
      ├→ Update transform (animation)
      ├→ Build push constants from GameObject
      │  ├→ push.transform = obj.transform2D.mat2()
      │  ├→ push.offset = obj.transform2D.translation
      │  └→ push.color = obj.color
      ├→ vkCmdPushConstants() (upload to GPU)
      ├→ obj.model->bind(commandBuffer)
      └→ obj.model->draw(commandBuffer)

4. renderer.endSwapChainRenderPass(commandBuffer)
   ├→ assert(isFrameStarted)  // Validate state
   └→ vkCmdEndRenderPass()

5. renderer.endFrame()
   ├→ assert(isFrameStarted)  // Validate state
   ├→ vkEndCommandBuffer()
   ├→ swapChain->submitCommandBuffers(&commandBuffer, &currentImageIndex)
   │  ├→ Wait on: imageAvailableSemaphores[currentFrame]
   │  ├→ Execute: commandBuffer
   │  ├→ Signal: renderFinishedSemaphores[currentFrame]
   │  ├→ vkQueueSubmit(inFlightFences[currentFrame])
   │  └→ vkQueuePresentKHR()
   │     └→ Check for VK_ERROR_OUT_OF_DATE_KHR / VK_SUBOPTIMAL_KHR / window resize
   │        └→ If out of date → recreateSwapChain()
   ├→ isFrameStarted = false
   └→ currentFrameIndex = (currentFrameIndex + 1) % MAX_FRAMES_IN_FLIGHT
```

**Key Architecture Changes:**

1. **Renderer Abstraction:**
   - Was: FirstApp managed swapchain and command buffers directly
   - Now: Renderer encapsulates all frame lifecycle complexity
   - Benefit: FirstApp focuses on scene management, not rendering details

2. **Render Systems:**
   - Was: All rendering logic in FirstApp::renderGameObjects()
   - Now: Separate SimpleRenderSystem class with own pipeline
   - Benefit: Modular, extensible, supports multiple render systems

3. **Command Buffer Recording:**
   - Was: Pre-recorded command buffers, recreated on resize
   - Now: Recorded every frame within beginFrame()/endFrame()
   - Benefit: Supports dynamic content, simpler state management

4. **State Validation:**
   - Assertions ensure correct order: beginFrame() → renderPass → endFrame()
   - Prevents common errors like double-ending command buffers
   - Debug builds catch mistakes immediately

### Synchronization Deep Dive

**Problem:** Prevent race conditions between CPU and GPU.

**Solution:** Semaphores (GPU-GPU sync) + Fences (CPU-GPU sync)

**Timeline Example (3 frames in flight):**

```
Frame 0:  CPU records → GPU renders → Present
Frame 1:  CPU records → GPU renders → Present
Frame 2:  CPU records → GPU renders → Present
Frame 3:  CPU WAITS for Frame 0 fence → Reuse resources
```

**Why MAX_FRAMES_IN_FLIGHT = 3?**
- Swapchain typically has 3 images
- Need one semaphore set per image
- Mismatch causes "semaphore reuse" errors

**Fence vs Semaphore:**

| Fence | Semaphore |
|-------|-----------|
| CPU-GPU sync | GPU-GPU sync |
| Can be checked by CPU | Only GPU can wait |
| Reset by CPU | Auto-reset |
| Example: "Is this frame done?" | Example: "Is image ready?" |

### Window Resizing and Swapchain Recreation

**Dynamic Window Support:**

The engine now supports runtime window resizing through a complete swapchain recreation mechanism.

**Resize Detection Flow:**

```
User resizes window
    ↓
GLFW detects resize → calls frameBufferResizeCallback()
    ↓
Window::framebufferResized flag set to true
    ↓
drawFrame() checks:
    - VK_ERROR_OUT_OF_DATE_KHR from acquireNextImage()
    - VK_ERROR_OUT_OF_DATE_KHR or VK_SUBOPTIMAL_KHR from present
    - window.wasWindowResized() flag
    ↓
recreateSwapChain() triggered
```

**Swapchain Recreation Process:**

```cpp
void recreateSwapChain() {
    // 1. Get new window dimensions
    auto extent = window.getExtent();
    
    // 2. Handle minimization (0×0 size)
    while (extent.width == 0 || extent.height == 0) {
        extent = window.getExtent();
        glfwWaitEvents();  // Pause rendering
    }
    
    // 3. Wait for GPU to finish current work
    vkDeviceWaitIdle(device.device());
    
    // 4. Create new swapchain (passing old one for optimization)
    if (swapChain == nullptr) {
        swapChain = std::make_unique<SwapChain>(device, extent);
    } else {
        swapChain = std::make_unique<SwapChain>(device, extent, std::move(swapChain));
        
        // 5. Check if image count changed (rare)
        if (swapChain->imageCount() != commandBuffers.size()) {
            freeCommandBuffers();
            createCommandBuffers();
        }
    }
    
    // 6. Recreate pipeline (depends on swapchain render pass)
    createPipeline();
}
```

**Key Design Decisions:**

1. **SwapChain as unique_ptr:**
   - Was: Direct member variable `SwapChain swapChain`
   - Now: `std::unique_ptr<SwapChain> swapChain`
   - Reason: Enables recreation without manually destroying old swapchain

2. **Old Swapchain Parameter:**
   - Passed to new SwapChain constructor
   - Vulkan can optimize recreation by reusing resources
   - Reduces visual artifacts during resize

3. **Dynamic Viewport/Scissor:**
   - Set per-frame in command buffer recording
   - No pipeline recreation needed for viewport changes
   - Significantly faster than recreating pipeline

4. **Command Buffer Management:**
   - Command buffers recorded every frame (not pre-recorded)
   - Allows dynamic viewport/scissor updates
   - Handles swapchain image count changes

**Why vkDeviceWaitIdle()?**
- Ensures GPU finished using old swapchain
- Prevents validation errors about resources in use
- Brief pause (~1-2ms) acceptable for resize event

**Minimization Handling:**
- Minimized windows report 0×0 extent
- Cannot create 0×0 swapchain (Vulkan error)
- Solution: Pause rendering with `glfwWaitEvents()` until restored

**Performance Impact:**
- Swapchain recreation: ~5-10ms
- Pipeline recreation: ~2-5ms
- Total resize cost: ~10-15ms
- Infrequent operation, acceptable latency

---

## Memory Management

### Current State (v0.1.0)

**Manual Management:**
- All Vulkan objects explicitly created/destroyed
- RAII patterns via destructors
- No smart pointers for Vulkan handles yet

**Destruction Order (Critical!):**

```cpp
~FirstApp()
  └→ vkDestroyPipelineLayout()

~Model()
  ├→ vkDestroyBuffer(vertexBuffer)
  └→ vkFreeMemory(vertexBufferMemory)

~Pipeline()
  ├→ vkDestroyShaderModule(vert)
  ├→ vkDestroyShaderModule(frag)
  └→ vkDestroyPipeline()

~SwapChain()
  ├→ vkDestroyImageView() (all views)
  ├→ vkDestroySwapchainKHR()
  ├→ vkDestroyImage() (depth)
  ├→ vkFreeMemory() (depth)
  ├→ vkDestroyFramebuffer() (all)
  ├→ vkDestroyRenderPass()
  ├→ vkDestroySemaphore() (all)
  └→ vkDestroyFence() (all)

~Device()
  ├→ vkDestroyCommandPool()
  ├→ vkDestroyDevice()
  ├→ DestroyDebugUtilsMessengerEXT()
  ├→ vkDestroySurfaceKHR()
  └→ vkDestroyInstance()

~Window()
  ├→ glfwDestroyWindow()
  └→ glfwTerminate()
```

**Why Order Matters:**
- Must destroy child before parent
- Example: Destroy swapchain before device
- Violating order = validation errors or crashes

### Future Improvements

- **VMA (Vulkan Memory Allocator):** Efficient GPU memory management
- **Smart pointers:** RAII for Vulkan handles
- **Resource pools:** Reuse frequently created objects

---

## Design Principles

### 1. Separation of Concerns

Each class has a single, well-defined responsibility:
- `Window` - Windowing only
- `Device` - Vulkan device management only
- `SwapChain` - Frame management only
- `Pipeline` - Rendering configuration only

### 2. Explicit Over Implicit

- No hidden state
- Clear initialization order
- Explicit dependencies in constructors

### 3. Copy Prevention

```cpp
Window(const Window&) = delete;           // No copy constructor
Window& operator=(const Window&) = delete; // No copy assignment
```

**Why?**
- Vulkan objects are non-copyable
- Prevents accidental resource duplication
- Forces explicit ownership semantics

### 4. Exception Safety

- All Vulkan failures throw `std::runtime_error`
- RAII ensures cleanup even on exceptions
- Main catches and reports errors

### 5. Modern C++20 Patterns

- `std::unique_ptr` for ownership

---

## Performance Characteristics

### Bottlenecks (to address later)

1. **Command Buffer Recording:** Currently per-frame (should be once)
2. **No Culling:** Renders everything every frame
3. **No Instancing:** Each object = separate draw call
4. **No Multithreading:** Single-threaded command recording

---

**Component Documentation:**
- **[Window Component](WINDOW.md)** - Window creation and surface management
- **[Device Component](DEVICE.md)** - Vulkan device initialization and management
- **[SwapChain Component](SWAPCHAIN.md)** - Frame management and synchronization
- **[Pipeline Component](PIPELINE.md)** - Graphics pipeline configuration
- **[Model Component](MODEL.md)** - Vertex data and buffer management