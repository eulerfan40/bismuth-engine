# SwapChain Component

Complete technical documentation for the SwapChain frame management system.

**File:** `engine/src/SwapChain.hpp`, `engine/src/SwapChain.cpp`  
**Purpose:** Frame buffering, render passes, synchronization  
**Dependencies:** Device 

---

## Table of Contents

- [Overview](#overview)
- [Class Interface](#class-interface)
- [Swapchain Basics](#swapchain-basics)
- [Initialization Sequence](#initialization-sequence)
- [Synchronization System](#synchronization-system)
- [Frame Acquisition](#frame-acquisition)
- [Command Submission](#command-submission)
- [Render Pass](#render-pass)
- [Depth Resources](#depth-resources)
- [Common Issues](#common-issues)

---

## Overview

The `SwapChain` class manages the entire frame presentation pipeline, from creating framebuffers to synchronizing CPU-GPU execution.

### Core Responsibilities

1. **Swapchain Creation** - Multiple image buffers for rendering
2. **Image Views** - Wrap swapchain images for access
3. **Render Pass** - Define rendering operations and attachments
4. **Depth Buffer** - Per-image depth testing resources
5. **Framebuffers** - Combine color and depth attachments
6. **Synchronization** - Semaphores and fences for frame coordination

### Why "SwapChain"?

The swapchain **swaps** between multiple image buffers:

```
Frame 0: Render to Image 0 → Display Image 2
Frame 1: Render to Image 1 → Display Image 0
Frame 2: Render to Image 2 → Display Image 1
```

This prevents **tearing** (displaying partially rendered frames).

---

## Class Interface

### Public Interface

```cpp
class SwapChain {
public:
    static constexpr int MAX_FRAMES_IN_FLIGHT = 3;

    SwapChain(Device &deviceRef, VkExtent2D windowExtent);
    ~SwapChain();

    // Non-copyable
    SwapChain(const SwapChain &) = delete;
    void operator=(const SwapChain &) = delete;

    // Accessors
    VkFramebuffer getFrameBuffer(int index);
    VkRenderPass getRenderPass();
    VkImageView getImageView(int index);
    size_t imageCount();
    VkFormat getSwapChainImageFormat();
    VkExtent2D getSwapChainExtent();
    uint32_t width();
    uint32_t height();
    float extentAspectRatio();

    // Depth format query
    VkFormat findDepthFormat();

    // Frame management
    VkResult acquireNextImage(uint32_t *imageIndex);
    VkResult submitCommandBuffers(const VkCommandBuffer *buffers, uint32_t *imageIndex);

private:
    void createSwapChain();
    void createImageViews();
    void createDepthResources();
    void createRenderPass();
    void createFramebuffers();
    void createSyncObjects();

    // Helper functions
    VkSurfaceFormatKHR chooseSwapSurfaceFormat(...);
    VkPresentModeKHR chooseSwapPresentMode(...);
    VkExtent2D chooseSwapExtent(...);

    // Vulkan objects
    VkSwapchainKHR swapChain;
    std::vector<VkImage> swapChainImages;
    std::vector<VkImageView> swapChainImageViews;
    VkFormat swapChainImageFormat;
    VkExtent2D swapChainExtent;

    VkRenderPass renderPass;
    std::vector<VkFramebuffer> swapChainFramebuffers;

    std::vector<VkImage> depthImages;
    std::vector<VkDeviceMemory> depthImageMemorys;
    std::vector<VkImageView> depthImageViews;

    // Synchronization
    std::vector<VkSemaphore> imageAvailableSemaphores;
    std::vector<VkSemaphore> renderFinishedSemaphores;
    std::vector<VkFence> inFlightFences;
    std::vector<VkFence> imagesInFlight;
    size_t currentFrame = 0;

    Device &device;
    VkExtent2D windowExtent;
};
```

---

## Swapchain Basics

### What Is a Swapchain?

A swapchain is a **queue of images** waiting to be presented to the screen.

```
┌──────────────────────────────────────────┐
│         Swapchain (3 images)             │
├──────────────────────────────────────────┤
│  Image 0  │  Image 1  │  Image 2         │
│  [Ready]  │  [Render] │  [Display]       │
└──────────────────────────────────────────┘
     ↑           ↑            ↑
     │           │            └─ Currently on screen
     │           └─ GPU rendering to this
     └─ Available for next frame
```

### Double vs Triple Buffering

**Double Buffering (2 images):**
```
Image 0: Rendering...
Image 1: Displaying
```
- CPU might wait for GPU
- Lower latency
- Possible stuttering

**Triple Buffering (3 images - typical):**
```
Image 0: Ready for next frame
Image 1: Rendering...
Image 2: Displaying
```
- CPU rarely waits
- Higher latency (by one frame)
- Smoother performance

**Our Configuration:**
```cpp
uint32_t imageCount = swapChainSupport.capabilities.minImageCount + 1;
// Usually: minImageCount = 2, so we get 3 images
```

### Swapchain Images

**Important:** Swapchain images are **owned by the swapchain**, not your application!

```cpp
// WRONG - Can't create swapchain images directly
VkImage myImage;
vkCreateImage(device, &info, nullptr, &myImage);  // Not a swapchain image

// CORRECT - Query swapchain for its images
std::vector<VkImage> swapChainImages;
vkGetSwapchainImagesKHR(device, swapChain, &count, swapChainImages.data());
```

**Lifecycle:**
- Created by `vkCreateSwapchainKHR()`
- Destroyed by `vkDestroySwapchainKHR()`
- You never call `vkCreateImage()` or `vkDestroyImage()` on them

---

## Initialization Sequence

### Constructor Flow

```cpp
SwapChain::SwapChain(Device &deviceRef, VkExtent2D extent)
    : device{deviceRef}, windowExtent{extent} {
    createSwapChain();      // 1. Create swapchain and images
    createImageViews();     // 2. Wrap images in views
    createRenderPass();     // 3. Define render pass
    createDepthResources(); // 4. Create depth buffers
    createFramebuffers();   // 5. Create framebuffers
    createSyncObjects();    // 6. Create semaphores/fences
}
```

### 1. createSwapChain()

```cpp
void SwapChain::createSwapChain() {
    SwapChainSupportDetails swapChainSupport = device.getSwapChainSupport();

    // Choose best settings
    VkSurfaceFormatKHR surfaceFormat = chooseSwapSurfaceFormat(swapChainSupport.formats);
    VkPresentModeKHR presentMode = chooseSwapPresentMode(swapChainSupport.presentModes);
    VkExtent2D extent = chooseSwapExtent(swapChainSupport.capabilities);

    // Determine image count
    uint32_t imageCount = swapChainSupport.capabilities.minImageCount + 1;
    if (swapChainSupport.capabilities.maxImageCount > 0 &&
        imageCount > swapChainSupport.capabilities.maxImageCount) {
        imageCount = swapChainSupport.capabilities.maxImageCount;
    }

    // Create swapchain
    VkSwapchainCreateInfoKHR createInfo = {};
    createInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
    createInfo.surface = device.surface();
    createInfo.minImageCount = imageCount;
    createInfo.imageFormat = surfaceFormat.format;
    createInfo.imageColorSpace = surfaceFormat.colorSpace;
    createInfo.imageExtent = extent;
    createInfo.imageArrayLayers = 1;  // 1 for non-stereoscopic
    createInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

    // Queue family handling
    QueueFamilyIndices indices = device.findPhysicalQueueFamilies();
    uint32_t queueFamilyIndices[] = {indices.graphicsFamily, indices.presentFamily};

    if (indices.graphicsFamily != indices.presentFamily) {
        createInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
        createInfo.queueFamilyIndexCount = 2;
        createInfo.pQueueFamilyIndices = queueFamilyIndices;
    } else {
        createInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
    }

    createInfo.preTransform = swapChainSupport.capabilities.currentTransform;
    createInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
    createInfo.presentMode = presentMode;
    createInfo.clipped = VK_TRUE;
    createInfo.oldSwapchain = VK_NULL_HANDLE;

    vkCreateSwapchainKHR(device.device(), &createInfo, nullptr, &swapChain);

    // Retrieve swapchain images
    vkGetSwapchainImagesKHR(device.device(), swapChain, &imageCount, nullptr);
    swapChainImages.resize(imageCount);
    vkGetSwapchainImagesKHR(device.device(), swapChain, &imageCount, swapChainImages.data());

    swapChainImageFormat = surfaceFormat.format;
    swapChainExtent = extent;
}
```

#### Surface Format Selection

```cpp
VkSurfaceFormatKHR SwapChain::chooseSwapSurfaceFormat(
    const std::vector<VkSurfaceFormatKHR> &availableFormats) {
    
    // Look for SRGB color space with BGRA8 format
    for (const auto &availableFormat : availableFormats) {
        if (availableFormat.format == VK_FORMAT_B8G8R8A8_UNORM &&
            availableFormat.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) {
            return availableFormat;
        }
    }

    // Fallback: Use first available
    return availableFormats[0];
}
```

**Why BGRA8 + SRGB?**

| Component | Purpose |
|-----------|---------|
| `VK_FORMAT_B8G8R8A8_UNORM` | 8 bits per channel (RGBA), normalized |
| `VK_COLOR_SPACE_SRGB_NONLINEAR_KHR` | Standard RGB color space |

**Color Space Importance:**
- SRGB matches monitor gamma curve
- Colors appear correct to human eye
- Industry standard for displays

#### Present Mode Selection

```cpp
VkPresentModeKHR SwapChain::chooseSwapPresentMode(
    const std::vector<VkPresentModeKHR> &availablePresentModes) {
    
    // Prefer mailbox (triple buffering)
    for (const auto &availablePresentMode : availablePresentModes) {
        if (availablePresentMode == VK_PRESENT_MODE_MAILBOX_KHR) {
            return availablePresentMode;
        }
    }

    // Fallback: FIFO (guaranteed available, v-sync)
    return VK_PRESENT_MODE_FIFO_KHR;
}
```

**Present Modes:**

| Mode | Behavior | Use Case |
|------|----------|----------|
| `IMMEDIATE` | No sync, possible tearing | Benchmarking |
| `FIFO` | V-sync, queue frames | Default, smooth |
| `MAILBOX` | Triple buffer, replace old | Low latency, smooth |
| `FIFO_RELAXED` | V-sync except when late | Adaptive |

**Why MAILBOX?**
- Low latency (replaces queued frames)
- No tearing (still synced to refresh)
- Best of both worlds

#### Image Sharing Mode

```cpp
if (indices.graphicsFamily != indices.presentFamily) {
    // Different queues: CONCURRENT (both can access)
    createInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
    createInfo.queueFamilyIndexCount = 2;
    createInfo.pQueueFamilyIndices = queueFamilyIndices;
} else {
    // Same queue: EXCLUSIVE (better performance)
    createInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
}
```

**Sharing Modes:**

| Mode | When | Performance |
|------|------|-------------|
| `EXCLUSIVE` | Graphics = Present queue | Better |
| `CONCURRENT` | Different queues | Slower |

**Why Different Modes?**
- EXCLUSIVE requires explicit ownership transfer
- CONCURRENT allows simultaneous access (simpler but slower)
- Most systems use same queue (EXCLUSIVE)

### 2. createImageViews()

```cpp
void SwapChain::createImageViews() {
    swapChainImageViews.resize(swapChainImages.size());
    
    for (size_t i = 0; i < swapChainImages.size(); i++) {
        VkImageViewCreateInfo viewInfo{};
        viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        viewInfo.image = swapChainImages[i];
        viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
        viewInfo.format = swapChainImageFormat;
        viewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        viewInfo.subresourceRange.baseMipLevel = 0;
        viewInfo.subresourceRange.levelCount = 1;
        viewInfo.subresourceRange.baseArrayLayer = 0;
        viewInfo.subresourceRange.layerCount = 1;

        vkCreateImageView(device.device(), &viewInfo, nullptr, &swapChainImageViews[i]);
    }
}
```

**What Is an Image View?**

An image view is a **window** into an image, describing how to interpret it.

```
VkImage (raw data)
    ↓
VkImageView (interpretation)
    ↓
Shader (uses view to read image)
```

**Why Needed?**
- Images are opaque memory
- Views specify format, mip levels, array layers
- Same image can have multiple views (different formats)

---

## Synchronization System

### The Synchronization Problem

**CPU and GPU run asynchronously:**

```
CPU Timeline:
Record Frame 0 → Record Frame 1 → Record Frame 2 → ...

GPU Timeline:
             Execute Frame 0 → Execute Frame 1 → ...

Problem: CPU might overwrite Frame 0 before GPU finishes!
```

**Solution:** Semaphores (GPU-GPU) + Fences (CPU-GPU)

### createSyncObjects()

```cpp
void SwapChain::createSyncObjects() {
    imageAvailableSemaphores.resize(MAX_FRAMES_IN_FLIGHT);  // 3
    renderFinishedSemaphores.resize(MAX_FRAMES_IN_FLIGHT);  // 3
    inFlightFences.resize(MAX_FRAMES_IN_FLIGHT);            // 3
    imagesInFlight.resize(imageCount(), VK_NULL_HANDLE);    // 3, initialized to null

    VkSemaphoreCreateInfo semaphoreInfo = {};
    semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

    VkFenceCreateInfo fenceInfo = {};
    fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;  // Start signaled!

    for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
        vkCreateSemaphore(device.device(), &semaphoreInfo, nullptr, 
                         &imageAvailableSemaphores[i]);
        vkCreateSemaphore(device.device(), &semaphoreInfo, nullptr, 
                         &renderFinishedSemaphores[i]);
        vkCreateFence(device.device(), &fenceInfo, nullptr, 
                     &inFlightFences[i]);
    }
}
```

### Synchronization Objects Explained

**1. imageAvailableSemaphores (3):**
- **Purpose:** Signals when swapchain image is ready to render to
- **Signaled by:** `vkAcquireNextImageKHR()`
- **Waited by:** Graphics queue submit

**2. renderFinishedSemaphores (3):**
- **Purpose:** Signals when rendering complete
- **Signaled by:** Graphics queue submit
- **Waited by:** `vkQueuePresentKHR()`

**3. inFlightFences (3):**
- **Purpose:** CPU waits for frame to complete
- **Signaled by:** Graphics queue submit
- **Waited by:** CPU (before reusing resources)

**4. imagesInFlight (3):**
- **Purpose:** Tracks which fence is using each image
- **Prevents:** Rendering to image still in use

### Why MAX_FRAMES_IN_FLIGHT = 3?

**The Critical Rule:** `MAX_FRAMES_IN_FLIGHT` must equal swapchain image count!

```cpp
Swapchain images: 3
Sync object sets: 3  ← MUST MATCH
```

**Why?**
Each image needs dedicated synchronization:

```
Image 0 → Semaphore Set 0
Image 1 → Semaphore Set 1
Image 2 → Semaphore Set 2
```

**What happens if mismatch?**

```
Images: 3, Semaphores: 2

Frame 0: Use Image 0, Semaphore Set 0 ✅
Frame 1: Use Image 1, Semaphore Set 1 ✅
Frame 2: Use Image 2, Semaphore Set 0 ❌ CONFLICT!
         (Set 0 still in use by Image 0!)
```

**Validation error:**
```
vkQueueSubmit(): pSignalSemaphores[0] is being signaled but may still be in use
```

### Fence Initial State: Signaled

```cpp
fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;
```

**Why start signaled?**

First frame:
```cpp
vkWaitForFences(..., inFlightFences[0], ...);  // Frame 0
// If unsignaled: Waits forever (nothing signaled it yet!)
// If signaled: Proceeds immediately ✅
```

Without this, first frame would deadlock!

---

## Frame Acquisition

### acquireNextImage()

```cpp
VkResult SwapChain::acquireNextImage(uint32_t *imageIndex) {
    // 1. Wait for this frame's previous use to complete
    vkWaitForFences(
        device.device(),
        1,
        &inFlightFences[currentFrame],
        VK_TRUE,
        std::numeric_limits<uint64_t>::max()
    );

    // 2. Acquire next available image
    VkResult result = vkAcquireNextImageKHR(
        device.device(),
        swapChain,
        std::numeric_limits<uint64_t>::max(),
        imageAvailableSemaphores[currentFrame],  // Signal when ready
        VK_NULL_HANDLE,
        imageIndex
    );

    return result;
}
```

### Step-by-Step Breakdown

**1. Wait for fence:**
```cpp
vkWaitForFences(device, 1, &inFlightFences[currentFrame], VK_TRUE, UINT64_MAX);
```

- **Blocks CPU** until GPU finishes previous frame using this slot
- **currentFrame** cycles 0 → 1 → 2 → 0 → 1 → 2...
- **Timeout:** `UINT64_MAX` = wait forever

**2. Acquire image:**
```cpp
vkAcquireNextImageKHR(device, swapChain, timeout, semaphore, fence, &imageIndex);
```

**Returns:**
- `VK_SUCCESS` - Image acquired
- `VK_SUBOPTIMAL_KHR` - Image acquired but swapchain suboptimal (resize?)
- `VK_ERROR_OUT_OF_DATE_KHR` - Swapchain invalid (must recreate)

**Signals:**
- `imageAvailableSemaphores[currentFrame]` when image ready

**Output:**
- `imageIndex` - Which image to render to (0, 1, or 2)

### Fence Wait Patterns

**Timeline visualization:**

```
Frame 0:
  Wait for fence 0 (signaled initially) → Pass immediately
  Acquire image 0
  Submit commands (signal fence 0 when done)
  currentFrame = 1

Frame 1:
  Wait for fence 1 (signaled initially) → Pass immediately
  Acquire image 1
  Submit commands (signal fence 1 when done)
  currentFrame = 2

Frame 2:
  Wait for fence 2 (signaled initially) → Pass immediately
  Acquire image 2
  Submit commands (signal fence 2 when done)
  currentFrame = 0

Frame 3:
  Wait for fence 0 → BLOCKS until Frame 0 GPU work done
  Acquire image 0
  ...
```

---

## Command Submission

### submitCommandBuffers()

```cpp
VkResult SwapChain::submitCommandBuffers(
    const VkCommandBuffer *buffers, 
    uint32_t *imageIndex) {
    
    // 1. Wait for previous frame using this image
    if (imagesInFlight[*imageIndex] != VK_NULL_HANDLE) {
        vkWaitForFences(device.device(), 1, &imagesInFlight[*imageIndex], VK_TRUE, UINT64_MAX);
    }
    imagesInFlight[*imageIndex] = inFlightFences[currentFrame];

    // 2. Submit command buffer
    VkSubmitInfo submitInfo = {};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

    VkSemaphore waitSemaphores[] = {imageAvailableSemaphores[currentFrame]};
    VkPipelineStageFlags waitStages[] = {VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};
    submitInfo.waitSemaphoreCount = 1;
    submitInfo.pWaitSemaphores = waitSemaphores;
    submitInfo.pWaitDstStageMask = waitStages;

    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = buffers;

    VkSemaphore signalSemaphores[] = {renderFinishedSemaphores[currentFrame]};
    submitInfo.signalSemaphoreCount = 1;
    submitInfo.pSignalSemaphores = signalSemaphores;

    vkResetFences(device.device(), 1, &inFlightFences[currentFrame]);
    
    vkQueueSubmit(device.graphicsQueue(), 1, &submitInfo, inFlightFences[currentFrame]);

    // 3. Present to screen
    VkPresentInfoKHR presentInfo = {};
    presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
    presentInfo.waitSemaphoreCount = 1;
    presentInfo.pWaitSemaphores = signalSemaphores;

    VkSwapchainKHR swapChains[] = {swapChain};
    presentInfo.swapchainCount = 1;
    presentInfo.pSwapchains = swapChains;
    presentInfo.pImageIndices = imageIndex;

    auto result = vkQueuePresentKHR(device.presentQueue(), &presentInfo);

    // 4. Advance to next frame slot
    currentFrame = (currentFrame + 1) % MAX_FRAMES_IN_FLIGHT;

    return result;
}
```

### Submission Pipeline

```
imageAvailableSemaphores[currentFrame]  (WAIT)
            ↓
    Execute Commands
            ↓
renderFinishedSemaphores[currentFrame]  (SIGNAL)
            ↓
    vkQueuePresentKHR (WAIT on renderFinished)
            ↓
    Display on Screen
```

### Wait Stage Explanation

```cpp
VkPipelineStageFlags waitStages[] = {VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};
```

**What does this mean?**

"Wait until the **color attachment output stage** before using the image."

**Why this stage?**
- Image isn't needed until we write colors
- Earlier stages (vertex processing) can run in parallel
- Maximizes GPU utilization

**Pipeline stages:**
```
Vertex Shader
    ↓
Rasterization
    ↓
Fragment Shader
    ↓
COLOR ATTACHMENT OUTPUT ← Wait here for image
    ↓
Final Image
```

---

## Render Pass

### createRenderPass()

```cpp
void SwapChain::createRenderPass() {
    // 1. Color attachment
    VkAttachmentDescription colorAttachment = {};
    colorAttachment.format = swapChainImageFormat;
    colorAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
    colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    colorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    colorAttachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

    // 2. Depth attachment
    VkAttachmentDescription depthAttachment{};
    depthAttachment.format = findDepthFormat();
    depthAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
    depthAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    depthAttachment.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    depthAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    depthAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    depthAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    depthAttachment.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

    // 3. Attachment references
    VkAttachmentReference colorAttachmentRef = {};
    colorAttachmentRef.attachment = 0;
    colorAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

    VkAttachmentReference depthAttachmentRef{};
    depthAttachmentRef.attachment = 1;
    depthAttachmentRef.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

    // 4. Subpass
    VkSubpassDescription subpass = {};
    subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    subpass.colorAttachmentCount = 1;
    subpass.pColorAttachments = &colorAttachmentRef;
    subpass.pDepthStencilAttachment = &depthAttachmentRef;

    // 5. Subpass dependency
    VkSubpassDependency dependency = {};
    dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
    dependency.dstSubpass = 0;
    dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | 
                              VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
    dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | 
                              VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
    dependency.srcAccessMask = 0;
    dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT | 
                               VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;

    // 6. Create render pass
    std::array<VkAttachmentDescription, 2> attachments = {colorAttachment, depthAttachment};
    VkRenderPassCreateInfo renderPassInfo = {};
    renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    renderPassInfo.attachmentCount = 2;
    renderPassInfo.pAttachments = attachments.data();
    renderPassInfo.subpassCount = 1;
    renderPassInfo.pSubpasses = &subpass;
    renderPassInfo.dependencyCount = 1;
    renderPassInfo.pDependencies = &dependency;

    vkCreateRenderPass(device.device(), &renderPassInfo, nullptr, &renderPass);
}
```

### Load/Store Operations

**Load Op (start of render pass):**

| Operation | Meaning |
|-----------|---------|
| `LOAD` | Preserve existing contents |
| `CLEAR` | Clear to a value |
| `DONT_CARE` | Don't care (fastest) |

**Store Op (end of render pass):**

| Operation | Meaning |
|-----------|---------|
| `STORE` | Save contents |
| `DONT_CARE` | Discard (faster) |

**Our choices:**
```cpp
colorAttachment.loadOp = CLEAR;        // Clear to background color
colorAttachment.storeOp = STORE;       // Save for presentation

depthAttachment.loadOp = CLEAR;        // Clear depth buffer
depthAttachment.storeOp = DONT_CARE;   // Don't need depth after rendering
```

### Image Layouts

```cpp
colorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
colorAttachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
```

**Layouts optimize memory access patterns:**

| Layout | Purpose |
|--------|---------|
| `UNDEFINED` | Don't care about previous contents |
| `COLOR_ATTACHMENT_OPTIMAL` | Optimal for rendering |
| `PRESENT_SRC_KHR` | Optimal for presentation |
| `DEPTH_STENCIL_ATTACHMENT_OPTIMAL` | Optimal for depth testing |

**Why UNDEFINED → PRESENT_SRC?**
- Start: Don't care (we're clearing anyway)
- End: Ready for presentation

---

## Depth Resources

### createDepthResources()

```cpp
void SwapChain::createDepthResources() {
    VkFormat depthFormat = findDepthFormat();
    
    // Create one depth image per swapchain image
    depthImages.resize(imageCount());
    depthImageMemorys.resize(imageCount());
    depthImageViews.resize(imageCount());

    for (int i = 0; i < depthImages.size(); i++) {
        // Create depth image
        VkImageCreateInfo imageInfo{};
        imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
        imageInfo.imageType = VK_IMAGE_TYPE_2D;
        imageInfo.extent.width = swapChainExtent.width;
        imageInfo.extent.height = swapChainExtent.height;
        imageInfo.extent.depth = 1;
        imageInfo.mipLevels = 1;
        imageInfo.arrayLayers = 1;
        imageInfo.format = depthFormat;
        imageInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
        imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        imageInfo.usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
        imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
        imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

        device.createImageWithInfo(imageInfo, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
                                  depthImages[i], depthImageMemorys[i]);

        // Create depth image view
        VkImageViewCreateInfo viewInfo{};
        viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        viewInfo.image = depthImages[i];
        viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
        viewInfo.format = depthFormat;
        viewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
        viewInfo.subresourceRange.baseMipLevel = 0;
        viewInfo.subresourceRange.levelCount = 1;
        viewInfo.subresourceRange.baseArrayLayer = 0;
        viewInfo.subresourceRange.layerCount = 1;

        vkCreateImageView(device.device(), &viewInfo, nullptr, &depthImageViews[i]);
    }
}
```

### Why One Depth Buffer Per Image?

**Each swapchain image needs its own depth buffer:**

```
Swapchain Image 0 → Depth Buffer 0
Swapchain Image 1 → Depth Buffer 1
Swapchain Image 2 → Depth Buffer 2
```

**Reason:** Prevents depth data conflicts between frames.

### Depth Format Selection

```cpp
VkFormat SwapChain::findDepthFormat() {
    return device.findSupportedFormat(
        {VK_FORMAT_D32_SFLOAT, VK_FORMAT_D32_SFLOAT_S8_UINT, VK_FORMAT_D24_UNORM_S8_UINT},
        VK_IMAGE_TILING_OPTIMAL,
        VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT
    );
}
```

**Format priority:**
1. `D32_SFLOAT` - 32-bit float depth (highest precision)
2. `D32_SFLOAT_S8_UINT` - 32-bit depth + 8-bit stencil
3. `D24_UNORM_S8_UINT` - 24-bit depth + 8-bit stencil (most common)

**Why multiple options?**
- Not all GPUs support all formats
- Query capabilities and pick best available

---

## Common Issues

### Issue 1: Semaphore Reuse Error

**Error:**
```
validation layer: vkQueueSubmit(): pSignalSemaphores[0] is being signaled 
but may still be in use by VkSwapchainKHR
```

**Cause:** `MAX_FRAMES_IN_FLIGHT` doesn't match swapchain image count

**Solution:**
```cpp
// Find actual image count
std::cout << "Swapchain images: " << swapChain.imageCount() << std::endl;

// Update MAX_FRAMES_IN_FLIGHT to match
static constexpr int MAX_FRAMES_IN_FLIGHT = 3;  // Must equal image count
```

### Issue 2: Swapchain Out of Date

**Error:**
```
vkAcquireNextImageKHR returned VK_ERROR_OUT_OF_DATE_KHR
```

**Cause:** Window resized or minimized

**Solution (not yet implemented):**
```cpp
if (result == VK_ERROR_OUT_OF_DATE_KHR) {
    recreateSwapChain();  // Future feature
    return;
}
```

### Issue 3: Validation Errors on Exit

**Error:**
```
validation layer: Object still in use when destroyed
```

**Cause:** Destroying swapchain while GPU still using it

**Solution:**
```cpp
// In FirstApp::run() before exiting
vkDeviceWaitIdle(device.device());  // ✅ Already doing this
```

### Issue 4: Black Screen

**Possible causes:**

1. **Render pass not clearing:**
   ```cpp
   clearValues[0].color = {0.1f, 0.1f, 0.1f, 1.0f};  // Check clear color
   ```

2. **Viewport/scissor mismatch:**
   ```cpp
   viewport.width = swapChainExtent.width;   // Must match
   viewport.height = swapChainExtent.height;
   ```

3. **Command buffer not submitted:**
   ```cpp
   swapChain.submitCommandBuffers(...);  // Don't forget this!
   ```

---

## Performance Considerations

### Frame Pacing

**Current implementation:**

```cpp
while (!window.shouldClose()) {
    glfwPollEvents();
    drawFrame();  // As fast as possible
}
```

**Present mode controls pacing:**
- `FIFO`: Locks to display refresh (60 FPS)
- `MAILBOX`: Renders as fast as possible, displays at refresh rate
- `IMMEDIATE`: No sync (unlimited FPS)

### Memory Usage

**Per swapchain image:**
- Color image: Width × Height × 4 bytes (RGBA)
- Depth image: Width × Height × 4 bytes (D32)
- **Total:** ~14MB for 1920×1080

**Three images:**
- 1920×1080: ~42MB
- 2560×1440: ~74MB
- 3840×2160: ~133MB

---

## Destruction Order

```cpp
SwapChain::~SwapChain() {
    // 1. Image views (oldest children first)
    for (auto imageView : swapChainImageViews) {
        vkDestroyImageView(device.device(), imageView, nullptr);
    }

    // 2. Swapchain (owns images)
    if (swapChain != nullptr) {
        vkDestroySwapchainKHR(device.device(), swapChain, nullptr);
    }

    // 3. Depth resources
    for (int i = 0; i < depthImages.size(); i++) {
        vkDestroyImageView(device.device(), depthImageViews[i], nullptr);
        vkDestroyImage(device.device(), depthImages[i], nullptr);
        vkFreeMemory(device.device(), depthImageMemorys[i], nullptr);
    }

    // 4. Framebuffers
    for (auto framebuffer : swapChainFramebuffers) {
        vkDestroyFramebuffer(device.device(), framebuffer, nullptr);
    }

    // 5. Render pass
    vkDestroyRenderPass(device.device(), renderPass, nullptr);

    // 6. Synchronization objects
    for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
        vkDestroySemaphore(device.device(), renderFinishedSemaphores[i], nullptr);
        vkDestroySemaphore(device.device(), imageAvailableSemaphores[i], nullptr);
        vkDestroyFence(device.device(), inFlightFences[i], nullptr);
    }
}
```

**Critical:** Destroy children before parents!

---

## Related Documentation

- [Device Component](DEVICE.md) - Provides device and queues
- [Pipeline Component](PIPELINE.md) - Uses render pass
- [Architecture Overview](ARCHITECTURE.md) - Synchronization explanation

---

**Next Component:** [Pipeline Component Documentation](PIPELINE.md)