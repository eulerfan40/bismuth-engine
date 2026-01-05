# Renderer Documentation

Complete technical documentation for the Renderer abstraction layer.

**Files:** `engine/src/Renderer.hpp`, `engine/src/Renderer.cpp`  
**Purpose:** Centralized rendering abstraction managing frame lifecycle and command buffers  
**Pattern:** Facade pattern for swapchain and command buffer management

---

## Table of Contents

- [Overview](#overview)
- [Class Interface](#class-interface)
- [Frame Lifecycle](#frame-lifecycle)
- [Command Buffer Management](#command-buffer-management)
- [Swapchain Management](#swapchain-management)
- [Design Patterns](#design-patterns)
- [Usage Examples](#usage-examples)
- [Common Issues](#common-issues)
- [Related Documentation](#related-documentation)

---

## Overview

### What Is the Renderer?

The **Renderer** is a high-level abstraction that encapsulates the complexity of Vulkan rendering operations:

```
Application (FirstApp)
    ↓
Renderer (Facade)
    ↓
SwapChain + CommandBuffers + Frame Management
```

**Responsibilities:**
- **Frame Lifecycle:** Begin/end frame operations with proper synchronization
- **Command Buffer Management:** Allocate, track, and provide command buffers
- **Swapchain Management:** Create, recreate, and manage swapchain lifecycle
- **Render Pass Control:** Begin/end render passes with correct configuration
- **State Tracking:** Ensure operations happen in correct order with assertions

### Why a Renderer Abstraction?

**Before (FirstApp handled everything):**
```cpp
class FirstApp {
    std::unique_ptr<SwapChain> swapChain;
    std::vector<VkCommandBuffer> commandBuffers;
    
    void drawFrame() {
        uint32_t imageIndex;
        swapChain->acquireNextImage(&imageIndex);
        recordCommandBuffer(imageIndex);
        swapChain->submitCommandBuffers(&commandBuffers[imageIndex], &imageIndex);
    }
};
```

**After (Renderer handles complexity):**
```cpp
class FirstApp {
    Renderer renderer{window, device};
    
    void run() {
        if (auto commandBuffer = renderer.beginFrame()) {
            renderer.beginSwapChainRenderPass(commandBuffer);
            // Render here
            renderer.endSwapChainRenderPass(commandBuffer);
            renderer.endFrame();
        }
    }
};
```

**Benefits:**
- **Separation of Concerns:** FirstApp focuses on scene management, not rendering details
- **Reusability:** Renderer can be used by multiple application classes
- **Maintainability:** Rendering logic centralized in one place
- **Safety:** Assertions prevent invalid state transitions
- **Simplicity:** Clean API hides Vulkan complexity

---

## Class Interface

### Header Definition

```cpp
namespace engine {
  class Renderer {
  public:
    Renderer(Window& window, Device& device);
    ~Renderer();

    Renderer(const Renderer&) = delete;
    Renderer &operator=(const Renderer&) = delete;

    // Query methods
    VkRenderPass getSwapChainRenderPass() const { return swapChain->getRenderPass(); }
    float getAspectRatio() const { return swapChain->extentAspectRatio(); }
    bool isFrameInProgress() const { return isFrameStarted; }
    
    VkCommandBuffer getCurrentCommandBuffer() const {
      assert(isFrameStarted && "Cannot get command buffer when frame not in progress!");
      return commandBuffers[currentFrameIndex];
    }
    
    int getFrameIndex() const {
      assert(isFrameStarted && "Cannot get frame index when frame not in progress!");
      return currentFrameIndex;
    }

    // Frame lifecycle
    VkCommandBuffer beginFrame();
    void endFrame();
    
    // Render pass control
    void beginSwapChainRenderPass(VkCommandBuffer commandBuffer);
    void endSwapChainRenderPass(VkCommandBuffer commandBuffer);

  private:
    void createCommandBuffers();
    void freeCommandBuffers();
    void recreateSwapChain();

    Window& window;
    Device& device;
    std::unique_ptr<SwapChain> swapChain;
    std::vector<VkCommandBuffer> commandBuffers;

    uint32_t currentImageIndex;
    int currentFrameIndex{0};
    bool isFrameStarted{false};
  };
}
```

### Constructor

```cpp
Renderer(Window& window, Device& device);
```

**Purpose:** Initialize renderer with required dependencies and create initial swapchain.

**Parameters:**
- `window` - Reference to Window for surface and extent queries
- `device` - Reference to Device for Vulkan operations

**Initialization Sequence:**
1. Store window and device references
2. Call `recreateSwapChain()` - creates initial swapchain
3. Call `createCommandBuffers()` - allocates command buffers

**Usage:**
```cpp
Window window{800, 600, "My App"};
Device device{window};
Renderer renderer{window, device};  // Ready to render
```

### Destructor

```cpp
~Renderer();
```

**Purpose:** Clean up command buffers (swapchain cleaned up by unique_ptr).

**Cleanup Order:**
1. `freeCommandBuffers()` - returns command buffers to pool
2. `swapChain` unique_ptr automatically destroyed

### Non-Copyable

```cpp
Renderer(const Renderer&) = delete;
Renderer &operator=(const Renderer&) = delete;
```

**Why No Copying?**
- Owns Vulkan resources (command buffers, swapchain)
- Copying would duplicate resource handles
- Single renderer per application is typical pattern

### Query Methods

#### getSwapChainRenderPass()

```cpp
VkRenderPass getSwapChainRenderPass() const { return swapChain->getRenderPass(); }
```

**Purpose:** Get render pass for pipeline creation.

**Usage:**
```cpp
SimpleRenderSystem system{device, renderer.getSwapChainRenderPass()};
```

**When to Call:** During initialization when creating render systems/pipelines.

#### getAspectRatio()

```cpp
float getAspectRatio() const { return swapChain->extentAspectRatio(); }
```

**Purpose:** Get current swapchain aspect ratio for camera projection.

**Returns:** Width/height ratio (e.g., 1920/1080 = 1.78).

**Usage:**
```cpp
Camera camera;
float aspect = renderer.getAspectRatio();
camera.setPerspectiveProjection(glm::radians(50.0f), aspect, 0.1f, 10.0f);
```

**When to Call:** Every frame before setting camera projection (handles window resize).

**Implementation:**
```cpp
// In SwapChain
float extentAspectRatio() const {
  return static_cast<float>(swapChainExtent.width) / static_cast<float>(swapChainExtent.height);
}
```

**Why Important:**
- Prevents stretched/distorted rendering on non-square windows
- Updates automatically when window is resized
- Essential for correct perspective projection

**See:** [CAMERA.md](CAMERA.md) for projection matrix details

#### isFrameInProgress()

```cpp
bool isFrameInProgress() const { return isFrameStarted; }
```

**Purpose:** Check if a frame is currently being recorded.

**Returns:** `true` if between `beginFrame()` and `endFrame()`.

**Use Case:** Conditional rendering or assertions in render systems.

#### getCurrentCommandBuffer()

```cpp
VkCommandBuffer getCurrentCommandBuffer() const {
  assert(isFrameStarted && "Cannot get command buffer when frame not in progress!");
  return commandBuffers[currentFrameIndex];
}
```

**Purpose:** Get the active command buffer for current frame.

**Assertion:** Throws if called outside of frame recording.

**Use Case:** Passing to render systems that need command buffer.

#### getFrameIndex()

```cpp
int getFrameIndex() const {
  assert(isFrameStarted && "Cannot get frame index when frame not in progress!");
  return currentFrameIndex;
}
```

**Purpose:** Get current frame index (0, 1, or 2 for triple buffering).

**Assertion:** Throws if called outside of frame recording.

**Use Case:** Frame-specific resource management or debugging.

---

## Frame Lifecycle

### Frame States

```
[Idle] → beginFrame() → [Recording] → endFrame() → [Idle]
           ↓                             ↑
       Returns CommandBuffer         Submits & Presents
```

**State Tracking:**
- `isFrameStarted` flag ensures correct state transitions
- Assertions prevent invalid operations (e.g., ending non-started frame)

### beginFrame()

```cpp
VkCommandBuffer beginFrame();
```

**Purpose:** Start a new frame and begin command buffer recording.

**Returns:**
- `VkCommandBuffer` - command buffer to record into
- `nullptr` - if swapchain out of date (recreated internally)

**Implementation Flow:**

```cpp
VkCommandBuffer Renderer::beginFrame() {
    assert(!isFrameStarted && "Cannot begin a new frame while one is already in progress!");

    // 1. Acquire next swapchain image
    auto result = swapChain->acquireNextImage(&currentImageIndex);

    // 2. Handle out-of-date swapchain (window resized)
    if (result == VK_ERROR_OUT_OF_DATE_KHR) {
        recreateSwapChain();
        return nullptr;  // Caller should skip this frame
    }

    // 3. Handle errors
    if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR) {
        throw std::runtime_error("Failed to acquire swap chain image!");
    }

    // 4. Mark frame as started
    isFrameStarted = true;

    // 5. Get command buffer for this frame
    auto commandBuffer = getCurrentCommandBuffer();

    // 6. Begin command buffer recording
    VkCommandBufferBeginInfo beginInfo{};
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;

    if (vkBeginCommandBuffer(commandBuffer, &beginInfo) != VK_SUCCESS) {
        throw std::runtime_error("Failed to begin recording command buffer!");
    }

    return commandBuffer;
}
```

**Key Points:**

1. **Acquires Next Image:**
   - Gets next available swapchain image
   - Waits on GPU fences automatically
   - May return `VK_ERROR_OUT_OF_DATE_KHR` if window resized

2. **Handles Resize:**
   - Detects out-of-date swapchain
   - Recreates swapchain automatically
   - Returns `nullptr` to signal caller to skip frame

3. **Begins Recording:**
   - Calls `vkBeginCommandBuffer()` on current command buffer
   - Command buffer now in "recording" state
   - Ready for render pass and draw commands

**Usage Pattern:**
```cpp
if (auto commandBuffer = renderer.beginFrame()) {
    // Frame successfully started, proceed with rendering
} else {
    // Swapchain was out of date, skip this frame
}
```

### endFrame()

```cpp
void endFrame();
```

**Purpose:** Finish command buffer recording, submit to GPU, and present.

**Implementation Flow:**

```cpp
void Renderer::endFrame() {
    assert(isFrameStarted && "Cannot end a frame while there are no frames in progress!");

    // 1. Get current command buffer
    auto commandBuffer = getCurrentCommandBuffer();

    // 2. End command buffer recording
    if (vkEndCommandBuffer(commandBuffer) != VK_SUCCESS) {
        throw std::runtime_error("Failed to record command buffer!");
    }

    // 3. Submit to GPU and present
    auto result = swapChain->submitCommandBuffers(&commandBuffer, &currentImageIndex);
    
    // 4. Handle out-of-date or suboptimal swapchain
    if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR || window.wasWindowResized()) {
        window.resetWindowResizedFlag();
        recreateSwapChain();
    } else if (result != VK_SUCCESS) {
        throw std::runtime_error("Failed to present swap chain image!");
    }

    // 5. Mark frame as ended
    isFrameStarted = false;
    
    // 6. Advance to next frame
    currentFrameIndex = (currentFrameIndex + 1) % SwapChain::MAX_FRAMES_IN_FLIGHT;
}
```

**Key Points:**

1. **Ends Recording:**
   - Calls `vkEndCommandBuffer()` 
   - Command buffer now complete and ready for submission

2. **Submits to GPU:**
   - Submits command buffer to graphics queue
   - GPU begins executing recorded commands

3. **Presents Image:**
   - Queues image for presentation to screen
   - Waits for rendering to complete

4. **Handles Resize:**
   - Checks for out-of-date/suboptimal results
   - Checks window resize flag
   - Recreates swapchain if needed

5. **Frame Counter:**
   - Advances `currentFrameIndex` (0 → 1 → 2 → 0)
   - Enables triple buffering

**Important:** Must always call after `beginFrame()` returns non-null.

---

## Command Buffer Management

### Command Buffer Allocation

**Strategy:** Fixed-size pool based on frames in flight (3 buffers for triple buffering).

```cpp
void Renderer::createCommandBuffers() {
    commandBuffers.resize(SwapChain::MAX_FRAMES_IN_FLIGHT);  // 3 buffers

    VkCommandBufferAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    allocInfo.commandPool = device.getCommandPool();
    allocInfo.commandBufferCount = static_cast<uint32_t>(commandBuffers.size());

    if (vkAllocateCommandBuffers(device.device(), &allocInfo, commandBuffers.data()) != VK_SUCCESS) {
        throw std::runtime_error("Failed to allocate command buffers!");
    }
}
```

**Why 3 Buffers?**
- Frame 0: GPU executing
- Frame 1: CPU recording
- Frame 2: Waiting
- Prevents CPU waiting for GPU

**Primary Level:**
- Can be submitted directly to queue
- Can execute secondary command buffers (future feature)

### Command Buffer Deallocation

```cpp
void Renderer::freeCommandBuffers() {
    vkFreeCommandBuffers(device.device(),
                         device.getCommandPool(),
                         static_cast<uint32_t>(commandBuffers.size()),
                         commandBuffers.data());
    commandBuffers.clear();
}
```

**When Called:**
- During destruction (`~Renderer()`)
- Never called during normal swapchain recreation (buffers reused)

**Why Not Recreate?**
- Command buffers recorded every frame (dynamic recording)
- No pre-recorded buffers, so don't need to match swapchain image count
- Reusing buffers is more efficient

### Command Buffer Lifecycle

```
[Created] → beginFrame() → vkBeginCommandBuffer() → [Recording]
                ↓                                         ↓
            Returns to App                       Record render pass
                                                         ↓
                                               vkEndCommandBuffer()
                                                         ↓
                                                  [Executable]
                                                         ↓
                                              Submit to GPU Queue
                                                         ↓
                                               [Pending on GPU]
                                                         ↓
                                              GPU completes
                                                         ↓
                                               Back to [Created]
```

**Key Point:** Command buffers are **reused** each frame. Recording resets previous contents.

---

## Swapchain Management

### Initial Creation

**Constructor calls:**
```cpp
Renderer::Renderer(Window& window, Device& device) : window{window}, device{device} {
    recreateSwapChain();  // Initial creation
    createCommandBuffers();
}
```

**recreateSwapChain() handles first-time creation:**
```cpp
if (swapChain == nullptr) {
    swapChain = std::make_unique<SwapChain>(device, extent);
}
```

### Swapchain Recreation

```cpp
void Renderer::recreateSwapChain() {
    // 1. Get current window size
    auto extent = window.getExtent();
    
    // 2. Handle minimization (0×0 size)
    while (extent.width == 0 || extent.height == 0) {
        extent = window.getExtent();
        glfwWaitEvents();  // Pause until window restored
    }

    // 3. Wait for GPU to finish
    vkDeviceWaitIdle(device.device());

    // 4. Create new swapchain
    if (swapChain == nullptr) {
        // First-time creation
        swapChain = std::make_unique<SwapChain>(device, extent);
    } else {
        // Recreate with old swapchain
        std::shared_ptr<SwapChain> oldSwapChain = std::move(swapChain);
        swapChain = std::make_unique<SwapChain>(device, extent, oldSwapChain);

        // 5. Verify formats didn't change
        if (!oldSwapChain->compareSwapChainFormats(*swapChain.get())) {
            throw std::runtime_error("Swap chain image or depth format has changed!");
        }
    }
}
```

**When Recreated:**
1. Window resized (detected in `beginFrame()` or `endFrame()`)
2. Window minimized then restored
3. `VK_ERROR_OUT_OF_DATE_KHR` returned from acquire/present
4. `VK_SUBOPTIMAL_KHR` returned from present

**Why Wait for Idle?**
- Ensures GPU finished using old swapchain
- Prevents validation errors about resources in-use
- Brief pause acceptable for infrequent resize events

**Format Verification:**
- Ensures image format (e.g., SRGB) didn't change
- Ensures depth format (e.g., D32_SFLOAT) didn't change
- If changed, pipelines would be incompatible (requires rebuild)

### Minimization Handling

```cpp
while (extent.width == 0 || extent.height == 0) {
    extent = window.getExtent();
    glfwWaitEvents();  // Block until event occurs
}
```

**Why Loop?**
- Minimized windows report 0×0 size
- Cannot create swapchain with 0 dimensions
- `glfwWaitEvents()` pauses CPU until window event (restore, close, etc.)

**Alternative:** Could skip frame and check next frame, but wastes CPU cycles.

---

## Render Pass Control

### beginSwapChainRenderPass()

```cpp
void Renderer::beginSwapChainRenderPass(VkCommandBuffer commandBuffer) {
    assert(isFrameStarted && "Can't call beginSwapChainRenderPass if frame is not in progress!");
    assert(commandBuffer == getCurrentCommandBuffer() &&
      "Can't begin render pass on command buffer from a different frame!");

    // 1. Configure render pass
    VkRenderPassBeginInfo renderPassInfo{};
    renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    renderPassInfo.renderPass = swapChain->getRenderPass();
    renderPassInfo.framebuffer = swapChain->getFrameBuffer(currentImageIndex);

    renderPassInfo.renderArea.offset = {0, 0};
    renderPassInfo.renderArea.extent = swapChain->getSwapChainExtent();

    // 2. Set clear values
    std::array<VkClearValue, 2> clearValues{};
    clearValues[0].color = {0.01f, 0.01f, 0.01f, 0.01f};  // Dark gray
    clearValues[1].depthStencil = {1.0f, 0};              // Far depth
    renderPassInfo.clearValueCount = static_cast<uint32_t>(clearValues.size());
    renderPassInfo.pClearValues = clearValues.data();

    // 3. Begin render pass
    vkCmdBeginRenderPass(commandBuffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);

    // 4. Set dynamic viewport and scissor
    VkViewport viewport{};
    viewport.x = 0.0f;
    viewport.y = 0.0f;
    viewport.width = static_cast<float>(swapChain->getSwapChainExtent().width);
    viewport.height = static_cast<float>(swapChain->getSwapChainExtent().height);
    viewport.minDepth = 0.0f;
    viewport.maxDepth = 1.0f;
    VkRect2D scissor{{0, 0}, swapChain->getSwapChainExtent()};
    vkCmdSetViewport(commandBuffer, 0, 1, &viewport);
    vkCmdSetScissor(commandBuffer, 0, 1, &scissor);
}
```

**Responsibilities:**

1. **Begin Render Pass:**
   - Starts render pass with correct framebuffer
   - Clears color and depth attachments
   - Transitions attachments to correct layouts

2. **Set Clear Values:**
   - Color: Dark gray (0.01, 0.01, 0.01, 1.0)
   - Depth: 1.0 (far plane in Vulkan depth range [0, 1])

3. **Configure Dynamic State:**
   - Viewport: Full swapchain extent
   - Scissor: No clipping (full extent)
   - Set every frame to handle window resize

**Assertions:**
- Ensures frame started
- Ensures correct command buffer used

**INLINE Contents:**
- `VK_SUBPASS_CONTENTS_INLINE` - commands recorded directly in primary buffer
- Alternative: `SECONDARY_COMMAND_BUFFERS` for advanced multi-threading

### endSwapChainRenderPass()

```cpp
void Renderer::endSwapChainRenderPass(VkCommandBuffer commandBuffer) {
    assert(isFrameStarted && "Can't call endSwapChainRenderPass if frame is not in progress!");
    assert(commandBuffer == getCurrentCommandBuffer() &&
      "Can't end render pass on command buffer from a different frame!");

    vkCmdEndRenderPass(commandBuffer);
}
```

**Responsibilities:**
- End render pass
- Transition attachments to final layouts
- **Does NOT** end command buffer (that happens in `endFrame()`)

**Common Mistake:** Don't call `vkEndCommandBuffer()` here! Command buffer wraps entire frame, not just render pass.

---

## Design Patterns

### Facade Pattern

**Renderer** acts as a facade for complex Vulkan subsystems:

```
Application Code
    ↓
[Renderer Facade]
    ├→ SwapChain (image presentation)
    ├→ CommandBuffers (recording)
    ├→ Frame synchronization
    └→ Window integration
```

**Benefits:**
- Simplified API (4 main methods vs dozens of Vulkan calls)
- Hides implementation details
- Single point of control for rendering lifecycle

### RAII (Resource Acquisition Is Initialization)

```cpp
Renderer::Renderer(Window& window, Device& device) {
    recreateSwapChain();      // Acquire
    createCommandBuffers();   // Acquire
}

Renderer::~Renderer() {
    freeCommandBuffers();     // Release
    // swapChain releases automatically (unique_ptr)
}
```

**Benefits:**
- Resources acquired in constructor
- Resources released in destructor
- Exception-safe (RAII guarantees cleanup)

### State Machine Pattern

**Frame State:**
```
isFrameStarted == false:
    - Can call: beginFrame()
    - Cannot call: endFrame(), getCurrentCommandBuffer(), etc.

isFrameStarted == true:
    - Can call: endFrame(), getCurrentCommandBuffer(), render pass methods
    - Cannot call: beginFrame()
```

**Enforced by Assertions:**
- Debug builds catch state errors immediately
- Release builds assume correct usage (performance)

---

## Usage Examples

### Example 1: Basic Rendering Loop

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

### Example 2: Multiple Render Systems

```cpp
void FirstApp::run() {
    SimpleRenderSystem simpleSystem{device, renderer.getSwapChainRenderPass()};
    ParticleSystem particleSystem{device, renderer.getSwapChainRenderPass()};
    UISystem uiSystem{device, renderer.getSwapChainRenderPass()};

    while (!window.shouldClose()) {
        glfwPollEvents();

        if (auto commandBuffer = renderer.beginFrame()) {
            renderer.beginSwapChainRenderPass(commandBuffer);
            
            // Render in order (depth-sorted)
            simpleSystem.renderGameObjects(commandBuffer, gameObjects);
            particleSystem.render(commandBuffer, particles);
            uiSystem.render(commandBuffer, uiElements);
            
            renderer.endSwapChainRenderPass(commandBuffer);
            renderer.endFrame();
        }
    }

    vkDeviceWaitIdle(device.device());
}
```

### Example 3: Conditional Rendering

```cpp
if (auto commandBuffer = renderer.beginFrame()) {
    renderer.beginSwapChainRenderPass(commandBuffer);
    
    if (showDebugInfo) {
        debugRenderSystem.render(commandBuffer);
    }
    
    if (gameState == PLAYING) {
        gameRenderSystem.render(commandBuffer, gameObjects);
    } else if (gameState == MENU) {
        menuRenderSystem.render(commandBuffer);
    }
    
    renderer.endSwapChainRenderPass(commandBuffer);
    renderer.endFrame();
}
```

---

## Common Issues

### Issue 1: Command Buffer Not in Recording State

**Error:**
```
validation layer: Cannot be called for VkCommandBuffer when it is not in a recording state
```

**Cause:** Calling `vkEndCommandBuffer()` in both `endSwapChainRenderPass()` and `endFrame()`.

**Solution:** Only call `vkEndCommandBuffer()` in `endFrame()`. Render pass != command buffer.

```cpp
// WRONG
void endSwapChainRenderPass(VkCommandBuffer commandBuffer) {
    vkCmdEndRenderPass(commandBuffer);
    vkEndCommandBuffer(commandBuffer);  // ❌ Don't do this!
}

// CORRECT
void endSwapChainRenderPass(VkCommandBuffer commandBuffer) {
    vkCmdEndRenderPass(commandBuffer);  // ✅ Only end render pass
}
```

### Issue 2: Assertion Failure - Frame Not Started

**Error:**
```
assertion failed: isFrameStarted && "Cannot get command buffer when frame not in progress!"
```

**Cause:** Calling `getCurrentCommandBuffer()` outside of frame.

**Solution:** Only call within `beginFrame()` / `endFrame()` block.

```cpp
// WRONG
auto cb = renderer.getCurrentCommandBuffer();  // ❌ No frame started!
renderer.beginFrame();

// CORRECT
if (auto cb = renderer.beginFrame()) {
    // Use cb here ✅
    renderer.endFrame();
}
```

### Issue 3: beginFrame() Returns nullptr

**Symptom:** Frame skipped, nothing rendered for one frame.

**Cause:** Swapchain out-of-date (window resized).

**Solution:** This is normal behavior! Renderer recreates swapchain, skip the frame.

```cpp
if (auto commandBuffer = renderer.beginFrame()) {
    // Render
    renderer.endFrame();
}
// If nullptr, just continue to next iteration
```

### Issue 4: Swapchain Format Changed Error

**Error:**
```
runtime_error: Swap chain image or depth format has changed!
```

**Cause:** GPU driver changed preferred surface format between recreations (very rare).

**Solution:** 
- Usually indicates driver or hardware issue
- Requires pipeline recreation to match new format
- Consider adding format change handling if this occurs frequently

---

## Related Documentation

- [SWAPCHAIN.md](SWAPCHAIN.md) - Swapchain creation and image presentation
- [PIPELINE.md](PIPELINE.md) - Pipeline creation with render pass
- [ARCHITECTURE.md](ARCHITECTURE.md) - Overall rendering architecture
- [RENDERSYSTEM.md](RENDERSYSTEM.md) - Render system pattern

---

