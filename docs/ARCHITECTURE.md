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

### 5. Model System

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
  ├→ SwapChain(device, window extent)
  │   ├→ createSwapChain()          // Create swapchain images
  │   ├→ createImageViews()         // Wrap images in views
  │   ├→ createRenderPass()         // Define render pass
  │   ├→ createDepthResources()     // Depth buffer
  │   ├→ createFramebuffers()       // Per-image framebuffers
  │   └→ createSyncObjects()        // Semaphores + fences
  │
  ├→ loadModels()
  │   └→ Model(device, vertices)    // Create vertex buffers
  │
  ├→ createPipelineLayout()         // Descriptor sets, push constants
  ├→ createPipeline()
  │   ├→ Read shader files (.spv)
  │   ├→ createShaderModule() (vert)
  │   ├→ createShaderModule() (frag)
  │   ├→ Get vertex input descriptors from Model
  │   └→ vkCreateGraphicsPipelines()
  │
  └→ createCommandBuffers()
      ├→ Begin command buffer recording
      ├→ Begin render pass
      ├→ pipeline->bind()            // Bind graphics pipeline
      ├→ model->bind()               // Bind vertex buffer
      ├→ model->draw()               // Issue draw command
      └→ End render pass
```

---

## Render Loop

### Frame Rendering Sequence

```cpp
while (!window.shouldClose()) {
    glfwPollEvents();              // Handle input
    drawFrame();                   // Render one frame
}
vkDeviceWaitIdle();                // Wait for GPU to finish
```

### drawFrame() Breakdown

```cpp
1. acquireNextImage(&imageIndex)
   ├→ vkWaitForFences(inFlightFences[currentFrame])  // Wait for frame
   └→ vkAcquireNextImageKHR(imageAvailableSemaphores[currentFrame])
      └→ Returns imageIndex (0, 1, or 2)

2. submitCommandBuffers(commandBuffer, imageIndex)
   ├→ Wait on: imageAvailableSemaphores[currentFrame]
   ├→ Execute: commandBuffers[imageIndex]
   ├→ Signal: renderFinishedSemaphores[currentFrame]
   └→ vkQueueSubmit(inFlightFences[currentFrame])

3. Present to screen
   ├→ Wait on: renderFinishedSemaphores[currentFrame]
   └→ vkQueuePresentKHR()

4. Advance frame counter
   └→ currentFrame = (currentFrame + 1) % 3
```

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