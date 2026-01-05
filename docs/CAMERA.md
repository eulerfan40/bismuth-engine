# Camera Documentation

Complete technical documentation for the Camera component.

**Files:** `engine/src/Camera.hpp`, `engine/src/Camera.cpp`  
**Purpose:** Projection matrix management for 3D rendering  
**Pattern:** Encapsulation of camera projection mathematics

---

## Table of Contents

- [Overview](#overview)
- [Class Interface](#class-interface)
- [Orthographic Projection](#orthographic-projection)
- [Perspective Projection](#perspective-projection)
- [Projection Mathematics](#projection-mathematics)
- [Usage Examples](#usage-examples)
- [Common Issues](#common-issues)
- [Future Enhancements](#future-enhancements)
- [Related Documentation](#related-documentation)

---

## Overview

### What Is the Camera?

The **Camera** component manages projection transformations that convert 3D world coordinates into 2D screen space:

```
3D World Coordinates
    ↓
Camera Projection Matrix
    ↓
Clip Space [-1, 1]
    ↓
Screen Space (pixels)
```

**Responsibilities:**
- **Projection Matrix Generation:** Create orthographic or perspective projection matrices
- **Aspect Ratio Handling:** Adapt to different window dimensions
- **View Frustum Definition:** Define visible volume of the scene
- **Depth Range Mapping:** Map 3D depth to Vulkan's [0, 1] range

### Why a Camera Component?

**Before (Direct clip space rendering):**
```cpp
// Objects rendered directly in clip space [-1, 1]
obj.transform.translation = {0.0f, 0.0f, 0.5f};  // Must manually fit in [0, 1] depth

// Push constants
push.transform = obj.transform.mat4();  // No projection
```

**After (Camera projection):**
```cpp
// Objects positioned in world space with natural units
obj.transform.translation = {0.0f, 0.0f, 2.5f};  // World units (meters, etc.)

// Camera defines projection
Camera camera;
camera.setPerspectiveProjection(glm::radians(50.0f), aspect, 0.1f, 10.0f);

// Push constants with projection
push.transform = camera.getProjection() * obj.transform.mat4();
```

**Benefits:**
- **Natural Coordinates:** Use world-space units instead of normalized clip space
- **Perspective Distortion:** Objects farther away appear smaller (realistic)
- **Flexible Viewing:** Easy to switch between orthographic and perspective
- **Aspect Ratio Correction:** Automatically handle different screen dimensions
- **View Frustum Control:** Define near/far clipping planes

---

## Class Interface

### Header Definition

```cpp
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
```

### Methods Overview

| Method | Purpose | Typical Use Case |
|--------|---------|------------------|
| `setOrthographicProjection()` | Parallel projection (no perspective) | 2D games, UI, CAD applications |
| `setPerspectiveProjection()` | Realistic 3D projection | 3D games, simulations, visualizations |
| `getProjection()` | Retrieve projection matrix | Multiply with model transforms |

---

## Orthographic Projection

### Method Signature

```cpp
void setOrthographicProjection(
    float left, float right, 
    float top, float bottom, 
    float near, float far);
```

**Purpose:** Create a parallel projection where objects maintain size regardless of distance.

### Parameters

| Parameter | Type | Description | Typical Values |
|-----------|------|-------------|----------------|
| `left` | `float` | Left edge of view volume | `-aspect` (e.g., -1.6) |
| `right` | `float` | Right edge of view volume | `+aspect` (e.g., 1.6) |
| `top` | `float` | Top edge of view volume | `-1.0` |
| `bottom` | `float` | Bottom edge of view volume | `1.0` |
| `near` | `float` | Near clipping plane distance | `-1.0` or `0.1` |
| `far` | `float` | Far clipping plane distance | `1.0` or `100.0` |

**Coordinate System:**
- X-axis: `left` to `right`
- Y-axis: `top` to `bottom` (Vulkan Y-down convention)
- Z-axis: `near` to `far` (Vulkan depth [0, 1])

### Usage Example

```cpp
Camera camera;
float aspect = renderer.getAspectRatio();  // e.g., 1920/1080 = 1.78

// Orthographic projection
camera.setOrthographicProjection(
    -aspect, aspect,  // left, right (maintains aspect ratio)
    -1.0f, 1.0f,      // top, bottom (vertical range)
    -1.0f, 1.0f       // near, far (depth range)
);
```

### Visual Comparison

```
Orthographic Projection:
┌────────────────────────┐ Far plane
│   ╔════╗               │
│   ║    ║  Same size    │
│   ╚════╝               │
│   ╔════╗               │
│   ║    ║  Same size    │
│   ╚════╝               │
└────────────────────────┘ Near plane
     ↑        ↑
   Close     Far
   (Both appear same size)
```

### Implementation Details

```cpp
void Camera::setOrthographicProjection(
    float left, float right, float top, float bottom, float near, float far) {
    projectionMatrix = glm::mat4{1.0f};
    projectionMatrix[0][0] = 2.f / (right - left);
    projectionMatrix[1][1] = 2.f / (bottom - top);
    projectionMatrix[2][2] = 1.f / (far - near);
    projectionMatrix[3][0] = -(right + left) / (right - left);
    projectionMatrix[3][1] = -(bottom + top) / (bottom - top);
    projectionMatrix[3][2] = -near / (far - near);
}
```

**Matrix Structure:**

```
┌                                           ┐
│  2/(r-l)      0          0       -(r+l)/(r-l) │
│    0       2/(b-t)       0       -(b+t)/(b-t) │
│    0          0       1/(f-n)      -n/(f-n)   │
│    0          0          0            1       │
└                                           ┘
```

**Transformations Applied:**
1. **Scale:** Map `[left, right]` → `[-1, 1]` (X-axis)
2. **Scale:** Map `[top, bottom]` → `[-1, 1]` (Y-axis)
3. **Scale:** Map `[near, far]` → `[0, 1]` (Z-axis, Vulkan depth)
4. **Translate:** Center the view volume around origin

**Key Properties:**
- **Parallel Lines:** Remain parallel (no vanishing point)
- **No Size Change:** Objects same size regardless of depth
- **Uniform Depth:** Z-buffering works but no perspective
- **Perfect for 2D:** UI, HUD, isometric games

### Use Cases

**2D Games:**
```cpp
// Classic 2D game view
camera.setOrthographicProjection(-10.0f, 10.0f, -10.0f, 10.0f, 0.0f, 1.0f);
```

**UI Rendering:**
```cpp
// Screen-space UI coordinates
float aspect = width / height;
camera.setOrthographicProjection(0.0f, width, 0.0f, height, -1.0f, 1.0f);
```

**CAD/Technical Drawing:**
```cpp
// Isometric view for engineering diagrams
camera.setOrthographicProjection(-aspect, aspect, -1.0f, 1.0f, 0.1f, 100.0f);
```

---

## Perspective Projection

### Method Signature

```cpp
void setPerspectiveProjection(float fovy, float aspect, float near, float far);
```

**Purpose:** Create realistic 3D projection where distant objects appear smaller.

### Parameters

| Parameter | Type | Description | Typical Values |
|-----------|------|-------------|----------------|
| `fovy` | `float` | Vertical field of view (radians) | `glm::radians(45.0f)` to `glm::radians(90.0f)` |
| `aspect` | `float` | Aspect ratio (width/height) | `1920.0f/1080.0f` = 1.78 |
| `near` | `float` | Near clipping plane distance | `0.1f` |
| `far` | `float` | Far clipping plane distance | `10.0f`, `100.0f`, `1000.0f` |

**Important Notes:**
- `fovy` must be in **radians**, not degrees: `glm::radians(50.0f)`
- `aspect` prevents distortion on non-square screens
- `near` and `far` define visible depth range (objects outside are clipped)
- `near` **must be > 0** (cannot be 0 or negative)

### Usage Example

```cpp
Camera camera;
float aspect = renderer.getAspectRatio();

// Perspective projection
camera.setPerspectiveProjection(
    glm::radians(50.0f),  // Field of view (50 degrees)
    aspect,               // Window aspect ratio
    0.1f,                 // Near plane (10cm in front)
    10.0f                 // Far plane (10m away)
);
```

### Visual Comparison

```
Perspective Projection:
        Far plane
    ╔════════════╗
    ║            ║
    ║   ╔════╗   ║
    ║   ║ Obj║   ║  Far = appears smaller
    ║   ╚════╝   ║
    ║            ║
    ║  ╔══════╗  ║
    ║  ║ Obj  ║  ║  Near = appears larger
    ║  ╚══════╝  ║
    ╚════════════╝
       Near plane
       
     View Frustum (pyramid shape)
```

### Field of View (FOV)

**FOV Controls "Zoom":**

```cpp
// Wide angle (fish-eye effect)
camera.setPerspectiveProjection(glm::radians(90.0f), aspect, 0.1f, 10.0f);

// Normal view (human eye ~50-60°)
camera.setPerspectiveProjection(glm::radians(50.0f), aspect, 0.1f, 10.0f);

// Narrow (telephoto/zoomed)
camera.setPerspectiveProjection(glm::radians(30.0f), aspect, 0.1f, 10.0f);
```

**Visual Effect:**

```
FOV = 90° (Wide)          FOV = 50° (Normal)       FOV = 30° (Narrow)
┌─────────────┐           ┌──────────┐             ┌─────┐
│ ╔═╗         │           │   ╔══╗   │             │ ╔═══╗ │
│ ╚═╝  More   │           │   ╚══╝   │             │ ╚═══╝ │
│   visible   │           │          │             │ Less  │
└─────────────┘           └──────────┘             └─────┘
```

### Implementation Details

```cpp
void Camera::setPerspectiveProjection(float fovy, float aspect, float near, float far) {
    assert(glm::abs(aspect - std::numeric_limits<float>::epsilon()) > 0.0f);
    const float tanHalfFovy = tan(fovy / 2.f);
    projectionMatrix = glm::mat4{0.0f};
    projectionMatrix[0][0] = 1.f / (aspect * tanHalfFovy);
    projectionMatrix[1][1] = 1.f / (tanHalfFovy);
    projectionMatrix[2][2] = far / (far - near);
    projectionMatrix[2][3] = 1.f;
    projectionMatrix[3][2] = -(far * near) / (far - near);
}
```

**Matrix Structure:**

```
┌                                        ┐
│  1/(a*tan(θ/2))      0           0         0      │
│       0          1/tan(θ/2)      0         0      │
│       0              0        f/(f-n)     -1      │
│       0              0       -fn/(f-n)     0      │
└                                        ┘

Where:
  a = aspect ratio
  θ = fovy (field of view Y)
  f = far plane
  n = near plane
```

**Key Components:**
1. **X-Scale:** `1/(aspect * tan(fovy/2))` - Horizontal FOV with aspect correction
2. **Y-Scale:** `1/tan(fovy/2)` - Vertical FOV
3. **Z-Transform:** `far/(far-near)` - Depth mapping to [0, 1]
4. **Perspective Divide:** `projectionMatrix[2][3] = 1.0` - Enables w-division

### Perspective Division

**How It Works:**

After matrix multiplication, `gl_Position` has components `(x, y, z, w)`:

```cpp
// After projection matrix
gl_Position = camera.getProjection() * modelTransform * vec4(position, 1.0);
// Result: gl_Position = vec4(x, y, z, w) where w ≠ 1.0

// GPU automatically performs perspective division:
x_clip = x / w
y_clip = y / w  
z_clip = z / w

// Objects further away have larger w → smaller x,y → appear smaller on screen
```

**Example:**

```
Object at z=1.0:  w=1.0  →  x/w = x/1.0 = x      (appears normal size)
Object at z=2.0:  w=2.0  →  x/w = x/2.0 = x/2    (appears half size)
Object at z=4.0:  w=4.0  →  x/w = x/4.0 = x/4    (appears quarter size)
```

### Near/Far Clipping Planes

**Purpose:** Define visible depth range.

```cpp
camera.setPerspectiveProjection(glm::radians(50.0f), aspect, 0.1f, 10.0f);
//                                                           ^^^^  ^^^^
//                                                           Near  Far
```

**Effects:**
- Objects with `z < near` (0.1): **Clipped** (not rendered)
- Objects with `near ≤ z ≤ far` (0.1 to 10.0): **Visible**
- Objects with `z > far` (10.0): **Clipped** (not rendered)

**Choosing Values:**

```cpp
// Small scene (room interior)
camera.setPerspectiveProjection(glm::radians(50.0f), aspect, 0.1f, 10.0f);

// Medium scene (outdoor environment)
camera.setPerspectiveProjection(glm::radians(50.0f), aspect, 0.1f, 100.0f);

// Large scene (open world)
camera.setPerspectiveProjection(glm::radians(50.0f), aspect, 0.1f, 1000.0f);
```

**Depth Precision Warning:**

```cpp
// BAD: Too large far/near ratio (1000000:1)
camera.setPerspectiveProjection(glm::radians(50.0f), aspect, 0.001f, 1000.0f);
// Result: Z-fighting artifacts, poor depth precision

// GOOD: Reasonable ratio (~100:1 to 1000:1)
camera.setPerspectiveProjection(glm::radians(50.0f), aspect, 0.1f, 100.0f);
// Result: Good depth precision, minimal artifacts
```

**Rule of Thumb:**
- Keep `far/near` ratio below 1000:1 for good depth precision
- Set `near` as large as possible without clipping nearby objects
- Set `far` as small as possible while showing entire scene

### Use Cases

**First-Person Games:**
```cpp
// Wide FOV for immersion
camera.setPerspectiveProjection(glm::radians(70.0f), aspect, 0.1f, 100.0f);
```

**Third-Person Games:**
```cpp
// Moderate FOV for natural perspective
camera.setPerspectiveProjection(glm::radians(50.0f), aspect, 0.5f, 200.0f);
```

**Cinematic View:**
```cpp
// Narrow FOV for "cinematic" feel
camera.setPerspectiveProjection(glm::radians(35.0f), aspect, 0.1f, 50.0f);
```

---

## Projection Mathematics

### Transformation Pipeline

**Complete vertex transformation:**

```cpp
// Shader code
gl_Position = projection * model * vec4(vertexPosition, 1.0);
              ^^^^^^^^^^   ^^^^^
              Camera       GameObject
```

**In application code:**

```cpp
// Per-frame: Update camera
Camera camera;
camera.setPerspectiveProjection(glm::radians(50.0f), aspect, 0.1f, 10.0f);

// Per-object: Combine transformations
SimplePushConstantData push{};
push.transform = camera.getProjection() * obj.transform.mat4();
//               ^^^^^^^^^^^^^^^^^^^^^^   ^^^^^^^^^^^^^^^^^^^
//               Camera projection         Model transform
```

**Transformation Stages:**

```
Local Space (model vertices)
    ↓ [Model Matrix]
World Space (scene positioning)
    ↓ [View Matrix - not yet implemented]
View Space (camera-relative)
    ↓ [Projection Matrix]
Clip Space (homogeneous coords)
    ↓ [Perspective Division: x/w, y/w, z/w]
NDC (Normalized Device Coordinates)
    ↓ [Viewport Transform]
Screen Space (pixels)
```

**Current Implementation:**

Since we don't have a view matrix yet, we're combining model-to-world and world-to-clip:

```
Local Space → [Model * Projection] → Clip Space
```

### Matrix Multiplication Order

**Critical:** Matrix multiplication is **right-to-left**:

```cpp
// CORRECT
push.transform = camera.getProjection() * obj.transform.mat4();
//               ^^^^^^^^^^^^^^^^^^^^^^   ^^^^^^^^^^^^^^^^^^^
//               Applied SECOND           Applied FIRST

// In shader: vertex transformed by model matrix first, then projection
gl_Position = push.transform * vec4(position, 1.0);
```

**Why This Order?**

```
Step 1: Model transform positions object in world
   vertex (0.5, 0.5, 0.5) * model → world position (0, 0, 2.5)

Step 2: Projection converts world to clip space  
   world position (0, 0, 2.5) * projection → clip space (x', y', z', w')
```

### Vulkan-Specific Adjustments

**GLM Configuration:**

```cpp
#define GLM_FORCE_RADIANS              // Use radians, not degrees
#define GLM_FORCE_DEPTH_ZERO_TO_ONE   // Vulkan depth [0, 1] not OpenGL [-1, 1]
#include <glm/glm.hpp>
```

**Depth Range:**
- **OpenGL:** Z-axis maps to [-1, 1]
- **Vulkan:** Z-axis maps to [0, 1]
- **GLM_FORCE_DEPTH_ZERO_TO_ONE:** Makes GLM generate Vulkan-compatible matrices

**Coordinate System:**
- **X-axis:** -1 (left) to +1 (right)
- **Y-axis:** -1 (top) to +1 (bottom) in Vulkan (flipped from OpenGL!)
- **Z-axis:** 0 (near) to 1 (far)

---

## Usage Examples

### Example 1: Basic Perspective Camera

```cpp
void FirstApp::run() {
    SimpleRenderSystem simpleRenderSystem{device, renderer.getSwapChainRenderPass()};
    Camera camera{};

    while (!window.shouldClose()) {
        glfwPollEvents();

        // Update camera projection every frame (handles window resize)
        float aspect = renderer.getAspectRatio();
        camera.setPerspectiveProjection(glm::radians(50.0f), aspect, 0.1f, 10.0f);

        if (auto commandBuffer = renderer.beginFrame()) {
            renderer.beginSwapChainRenderPass(commandBuffer);
            simpleRenderSystem.renderGameObjects(commandBuffer, gameObjects, camera);
            renderer.endSwapChainRenderPass(commandBuffer);
            renderer.endFrame();
        }
    }
}
```

### Example 2: Switching Projections

```cpp
bool useOrthographic = false;  // Toggle with key press

while (!window.shouldClose()) {
    float aspect = renderer.getAspectRatio();
    
    if (useOrthographic) {
        // Orthographic: isometric view
        camera.setOrthographicProjection(-aspect, aspect, -1.0f, 1.0f, -1.0f, 1.0f);
    } else {
        // Perspective: realistic 3D
        camera.setPerspectiveProjection(glm::radians(50.0f), aspect, 0.1f, 10.0f);
    }
    
    // Render...
}
```

### Example 3: Dynamic FOV (Zoom Effect)

```cpp
float currentFOV = 50.0f;  // Starting FOV

// In game loop
if (zoomIn) {
    currentFOV = std::max(20.0f, currentFOV - 1.0f);  // Narrow FOV (zoom in)
} else if (zoomOut) {
    currentFOV = std::min(90.0f, currentFOV + 1.0f);  // Wide FOV (zoom out)
}

camera.setPerspectiveProjection(glm::radians(currentFOV), aspect, 0.1f, 10.0f);
```

### Example 4: In Render System

```cpp
void SimpleRenderSystem::renderGameObjects(
    VkCommandBuffer commandBuffer,
    std::vector<GameObject> &gameObjects,
    const Camera &camera) {
    
    pipeline->bind(commandBuffer);

    for (auto &obj : gameObjects) {
        obj.transform.rotation.y = glm::mod(
            obj.transform.rotation.y + 0.01f, 
            glm::two_pi<float>()
        );

        SimplePushConstantData push{};
        push.color = obj.color;
        push.transform = camera.getProjection() * obj.transform.mat4();
        //               ^^^^^^^^^^^^^^^^^^^^^^
        //               Camera projection applied here

        vkCmdPushConstants(commandBuffer, pipelineLayout,
            VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT,
            0, sizeof(SimplePushConstantData), &push);

        obj.model->bind(commandBuffer);
        obj.model->draw(commandBuffer);
    }
}
```

---

## Common Issues

### Issue 1: Objects Not Visible

**Symptoms:** Black screen, no objects rendered.

**Causes:**

1. **Objects outside view frustum:**
```cpp
// Object positioned beyond far plane
obj.transform.translation = {0.0f, 0.0f, 15.0f};  // z=15
camera.setPerspectiveProjection(glm::radians(50.0f), aspect, 0.1f, 10.0f);
//                                                                   ^^^^ far=10
// Object at z=15 is clipped!
```

**Solution:** Adjust far plane or move object closer:
```cpp
camera.setPerspectiveProjection(glm::radians(50.0f), aspect, 0.1f, 20.0f);
// OR
obj.transform.translation = {0.0f, 0.0f, 5.0f};
```

2. **Objects behind near plane:**
```cpp
obj.transform.translation = {0.0f, 0.0f, 0.05f};  // z=0.05
camera.setPerspectiveProjection(glm::radians(50.0f), aspect, 0.1f, 10.0f);
//                                                           ^^^^ near=0.1
// Object at z=0.05 is behind camera!
```

**Solution:**
```cpp
obj.transform.translation = {0.0f, 0.0f, 0.5f};  // z > near
```

### Issue 2: Distorted/Stretched Geometry

**Symptoms:** Circles appear as ovals, squares as rectangles.

**Cause:** Incorrect aspect ratio.

```cpp
// WRONG: Hardcoded aspect ratio
camera.setPerspectiveProjection(glm::radians(50.0f), 1.0f, 0.1f, 10.0f);
//                                                   ^^^^ assumes square window
```

**Solution:** Use actual window aspect ratio:
```cpp
// CORRECT: Dynamic aspect ratio
float aspect = renderer.getAspectRatio();  // Updates on window resize
camera.setPerspectiveProjection(glm::radians(50.0f), aspect, 0.1f, 10.0f);
```

### Issue 3: FOV in Degrees Instead of Radians

**Symptoms:** Extremely narrow or broken view.

```cpp
// WRONG: Degrees
camera.setPerspectiveProjection(50.0f, aspect, 0.1f, 10.0f);
// Interprets 50 radians ≈ 2864 degrees!
```

**Solution:**
```cpp
// CORRECT: Convert to radians
camera.setPerspectiveProjection(glm::radians(50.0f), aspect, 0.1f, 10.0f);
```

### Issue 4: Z-Fighting (Flickering Depth)

**Symptoms:** Surfaces flicker between visible/hidden.

**Cause:** Poor depth precision from extreme near/far ratio:

```cpp
// BAD: far/near = 10000 (too large)
camera.setPerspectiveProjection(glm::radians(50.0f), aspect, 0.01f, 100.0f);
```

**Solution:** Reduce far/near ratio:
```cpp
// GOOD: far/near = 100 (reasonable)
camera.setPerspectiveProjection(glm::radians(50.0f), aspect, 0.1f, 10.0f);
```

### Issue 5: Wrong Matrix Multiplication Order

**Symptoms:** Objects appear in wrong position or not transformed correctly.

```cpp
// WRONG: Projection applied before model transform
push.transform = obj.transform.mat4() * camera.getProjection();
```

**Solution:**
```cpp
// CORRECT: Projection applied after model transform
push.transform = camera.getProjection() * obj.transform.mat4();
```

### Issue 6: Projection Not Updated on Resize

**Symptoms:** Stretched view after window resize.

**Cause:** Projection matrix set only once at initialization:

```cpp
// BAD: Camera created outside loop
Camera camera{};
camera.setPerspectiveProjection(glm::radians(50.0f), aspect, 0.1f, 10.0f);

while (!window.shouldClose()) {
    // aspect ratio changes on resize, but camera not updated!
    // ...render...
}
```

**Solution:** Update camera every frame or on resize events:
```cpp
// GOOD: Camera updated each frame
while (!window.shouldClose()) {
    float aspect = renderer.getAspectRatio();  // Gets current aspect
    camera.setPerspectiveProjection(glm::radians(50.0f), aspect, 0.1f, 10.0f);
    // ...render...
}
```

---

## Future Enhancements

### View Matrix (Camera Position & Orientation)

**Current Limitation:** Camera position and orientation are fixed.

**Future Implementation:**
```cpp
class Camera {
public:
    void setViewDirection(glm::vec3 position, glm::vec3 direction, glm::vec3 up);
    void setViewTarget(glm::vec3 position, glm::vec3 target, glm::vec3 up);
    void setViewYXZ(glm::vec3 position, glm::vec3 rotation);
    
    const glm::mat4 &getView() const { return viewMatrix; }
    const glm::mat4 &getProjection() const { return projectionMatrix; }

private:
    glm::mat4 viewMatrix{1.0f};
    glm::mat4 projectionMatrix{1.0f};
};
```

**Usage:**
```cpp
camera.setViewTarget(
    {0.0f, 0.0f, -3.0f},   // Camera position (behind origin)
    {0.0f, 0.0f, 0.0f},    // Look at origin
    {0.0f, -1.0f, 0.0f}    // Up direction (Vulkan Y-down)
);

// In render system
push.transform = camera.getProjection() * camera.getView() * obj.transform.mat4();
```

### Camera Controller

**Goal:** Handle user input for camera movement.

```cpp
class CameraController {
public:
    void moveInPlaneXZ(GLFWwindow* window, float dt, GameObject& cameraObject);
    
private:
    float moveSpeed{3.0f};
    float lookSpeed{1.5f};
};
```

### Projection-View Matrix Caching

**Optimization:** Compute once per frame instead of per object.

```cpp
class Camera {
public:
    const glm::mat4 &getProjectionView() const { 
        return projectionMatrix * viewMatrix; 
    }
};

// Usage
glm::mat4 projectionView = camera.getProjectionView();  // Once per frame
for (auto& obj : gameObjects) {
    push.transform = projectionView * obj.transform.mat4();  // Per object
}
```

### Frustum Culling

**Optimization:** Don't render objects outside camera view.

```cpp
class Camera {
public:
    bool isInFrustum(const glm::vec3& position, float radius) const;
    
private:
    std::array<glm::vec4, 6> frustumPlanes;  // Left, right, top, bottom, near, far
};
```

---

## Related Documentation

**Core Components:**
- [RENDERER.md](RENDERER.md) - Provides aspect ratio via `getAspectRatio()`
- [RENDERSYSTEM.md](RENDERSYSTEM.md) - Uses Camera in `renderGameObjects()`
- [GAMEOBJECT.md](GAMEOBJECT.md) - Transform matrices multiplied with projection

**Rendering Pipeline:**
- [SHADER.md](SHADER.md) - Vertex shader receives combined projection * model matrix
- [PIPELINE.md](PIPELINE.md) - Graphics pipeline configuration
- [SWAPCHAIN.md](SWAPCHAIN.md) - Provides aspect ratio calculation

**Mathematics:**
- [GLM Documentation](https://github.com/g-truc/glm) - Matrix math library
- [Vulkan Coordinate Systems](https://matthewwellings.com/blog/the-new-vulkan-coordinate-system/)

**Architecture:**
- [ARCHITECTURE.md](ARCHITECTURE.md) - Camera component overview and rendering flow

---

**Next Steps:**
- Read [RENDERSYSTEM.md](RENDERSYSTEM.md) to understand Camera integration
- See [GAMEOBJECT.md](GAMEOBJECT.md) for transform matrix usage
- Review [SHADER.md](SHADER.md) for vertex transformation details
