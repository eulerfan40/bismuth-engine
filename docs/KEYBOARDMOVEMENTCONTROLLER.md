# KeyboardMovementController Documentation

## Overview

The `KeyboardMovementController` class provides keyboard-based camera and GameObject control for first-person style navigation in 3D space. It translates keyboard input into smooth, frame-rate-independent movement and rotation of GameObjects, enabling interactive camera control through the WASD movement scheme and arrow key look controls.

**Purpose:** Enable user-controlled camera movement and viewing direction through keyboard input, creating an intuitive first-person navigation experience.

**Key Features:**
- **Frame-rate independent movement** - Consistent speed regardless of FPS using delta time
- **Configurable key mappings** - Customizable controls via KeyMappings struct
- **6-DOF movement** - Forward/backward, left/right, up/down translation
- **Smooth rotation** - Pitch and yaw control with configurable look speed
- **Rotation clamping** - Prevents camera gimbal lock with pitch limits

## Architecture

### Class Structure

```cpp
namespace engine {
  class KeyboardMovementController {
  public:
    struct KeyMappings {
        int moveLeft = GLFW_KEY_A;
        int moveRight = GLFW_KEY_D;
        int moveForward = GLFW_KEY_W;
        int moveBackward = GLFW_KEY_S;
        int moveUp = GLFW_KEY_E;
        int moveDown = GLFW_KEY_Q;
        int lookLeft = GLFW_KEY_LEFT;
        int lookRight = GLFW_KEY_RIGHT;
        int lookUp = GLFW_KEY_UP;
        int lookDown = GLFW_KEY_DOWN;
    };

    void moveInPlaneXZ(GLFWwindow* window, float dt, GameObject& gameObject);

    KeyMappings keys{};
    float moveSpeed{3.0f};
    float lookSpeed{1.5f};
  };
}
```

### Component Relationships

```
KeyboardMovementController
    ↓ reads input from
Window (GLFWwindow*)
    ↓ modifies
GameObject (transform component)
    ↓ used by
Camera (setViewYXZ)
```

## Core Components

### KeyMappings Struct

Defines the keyboard bindings for all movement and look controls.

**Default Mappings:**

| Action | Default Key | GLFW Constant |
|--------|-------------|---------------|
| Move Forward | W | `GLFW_KEY_W` |
| Move Backward | S | `GLFW_KEY_S` |
| Move Left | A | `GLFW_KEY_A` |
| Move Right | D | `GLFW_KEY_D` |
| Move Up | E | `GLFW_KEY_E` |
| Move Down | Q | `GLFW_KEY_Q` |
| Look Left | Left Arrow | `GLFW_KEY_LEFT` |
| Look Right | Right Arrow | `GLFW_KEY_RIGHT` |
| Look Up | Up Arrow | `GLFW_KEY_UP` |
| Look Down | Down Arrow | `GLFW_KEY_DOWN` |

**Customization:**
```cpp
KeyboardMovementController controller{};
controller.keys.moveForward = GLFW_KEY_UP;    // Change W to Up Arrow
controller.keys.moveBackward = GLFW_KEY_DOWN; // Change S to Down Arrow
controller.keys.moveLeft = GLFW_KEY_LEFT;     // Change A to Left Arrow
controller.keys.moveRight = GLFW_KEY_RIGHT;   // Change D to Right Arrow
```

### Movement Speed

Controls the rate of translation movement in world units per second.

**Type:** `float`  
**Default:** `3.0f`  
**Units:** World units per second

**Tuning Guidelines:**
- **Slow exploration:** `1.0f - 2.0f`
- **Normal walking:** `3.0f - 5.0f`
- **Fast movement:** `6.0f - 10.0f`
- **Flying/debugging:** `10.0f+`

```cpp
controller.moveSpeed = 5.0f; // Faster movement
```

### Look Speed

Controls the rate of rotation in radians per second.

**Type:** `float`  
**Default:** `1.5f`  
**Units:** Radians per second

**Tuning Guidelines:**
- **Slow/precise:** `0.5f - 1.0f`
- **Normal:** `1.5f - 2.5f`
- **Fast/responsive:** `3.0f - 5.0f`

```cpp
controller.lookSpeed = 2.0f; // Faster camera rotation
```

## Primary Method

### moveInPlaneXZ()

Updates a GameObject's transform based on keyboard input, providing smooth frame-rate-independent movement and rotation.

**Signature:**
```cpp
void moveInPlaneXZ(GLFWwindow* window, float dt, GameObject& gameObject);
```

**Parameters:**

| Parameter | Type | Description |
|-----------|------|-------------|
| `window` | `GLFWwindow*` | GLFW window handle for polling keyboard state |
| `dt` | `float` | Delta time (frame time) in seconds for frame-rate independence |
| `gameObject` | `GameObject&` | GameObject whose transform will be modified |

**Behavior:**

1. **Rotation Processing:**
   - Polls arrow keys for look input
   - Accumulates rotation changes based on `lookSpeed * dt`
   - Normalizes rotation vector if magnitude > epsilon
   - Applies rotation to `gameObject.transform.rotation`
   - Clamps pitch (x-axis) to prevent gimbal lock: `[-1.5, 1.5]` radians (~±85°)
   - Wraps yaw (y-axis) to `[0, 2π]` for continuity

2. **Movement Processing:**
   - Calculates forward, right, and up direction vectors from current yaw
   - Polls WASD/EQ keys for movement input
   - Accumulates movement in local space
   - Normalizes movement vector if magnitude > epsilon
   - Applies movement at `moveSpeed * dt` to `gameObject.transform.translation`

**Direction Vectors:**
```cpp
float yaw = gameObject.transform.rotation.y;
const glm::vec3 forwardDir{sin(yaw), 0.0f, cos(yaw)};
const glm::vec3 rightDir{forwardDir.z, 0.0f, -forwardDir.x};
const glm::vec3 upDir{0.0f, -1.0f, 0.0f};
```

**Why XZ Plane Movement?**

The method name indicates movement primarily occurs in the XZ horizontal plane (ground plane), with Y representing vertical/up-down movement. This is standard for first-person controls where:
- **X-axis:** Left/right (strafe)
- **Y-axis:** Up/down (vertical movement)
- **Z-axis:** Forward/backward

## Usage Patterns

### Basic Camera Control

The most common usage pattern: controlling a viewer GameObject that drives the camera.

```cpp
#include "KeyboardMovementController.hpp"
#include "Camera.hpp"
#include "GameObject.hpp"
#include <chrono>

// In FirstApp or main game loop class
Camera camera{};
auto viewerObject = GameObject::createGameObject();
KeyboardMovementController cameraController{};

auto currentTime = std::chrono::high_resolution_clock::now();

while (!window.shouldClose()) {
  glfwPollEvents();
  
  // Calculate delta time
  auto newTime = std::chrono::high_resolution_clock::now();
  float frameTime = std::chrono::duration<float, std::chrono::seconds::period>(
    newTime - currentTime
  ).count();
  currentTime = newTime;
  
  // Update viewer position and rotation based on input
  cameraController.moveInPlaneXZ(window.getGLFWwindow(), frameTime, viewerObject);
  
  // Apply viewer transform to camera view
  camera.setViewYXZ(viewerObject.transform.translation, viewerObject.transform.rotation);
  
  // Set camera projection
  float aspect = renderer.getAspectRatio();
  camera.setPerspectiveProjection(glm::radians(50.0f), aspect, 0.1f, 10.0f);
  
  // Render frame...
}
```

**Key Points:**
- Use `std::chrono::high_resolution_clock` for accurate delta time
- Update controller before setting camera view
- Calculate frame time as duration between frames in seconds
- Pass `window.getGLFWwindow()` to access the underlying GLFW window handle

### Custom Key Bindings

Configure alternative control schemes for different play styles.

```cpp
KeyboardMovementController controller{};

// Arrow key movement (classic style)
controller.keys.moveForward = GLFW_KEY_UP;
controller.keys.moveBackward = GLFW_KEY_DOWN;
controller.keys.moveLeft = GLFW_KEY_LEFT;
controller.keys.moveRight = GLFW_KEY_RIGHT;

// IJKL look controls
controller.keys.lookLeft = GLFW_KEY_J;
controller.keys.lookRight = GLFW_KEY_L;
controller.keys.lookUp = GLFW_KEY_I;
controller.keys.lookDown = GLFW_KEY_K;

// Space/Shift for vertical movement
controller.keys.moveUp = GLFW_KEY_SPACE;
controller.keys.moveDown = GLFW_KEY_LEFT_SHIFT;
```

### Adjustable Movement Parameters

Provide dynamic speed control for different gameplay contexts.

```cpp
KeyboardMovementController controller{};

// Exploration mode
controller.moveSpeed = 2.0f;
controller.lookSpeed = 1.0f;

// Fast travel mode (shift to run)
if (glfwGetKey(window.getGLFWwindow(), GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS) {
  controller.moveSpeed = 10.0f;
}

// Precision mode (ctrl for fine control)
if (glfwGetKey(window.getGLFWwindow(), GLFW_KEY_LEFT_CONTROL) == GLFW_PRESS) {
  controller.moveSpeed = 0.5f;
  controller.lookSpeed = 0.5f;
}
```

### Controlling Multiple Objects

While designed for camera control, the controller can manipulate any GameObject.

```cpp
KeyboardMovementController playerController{};
KeyboardMovementController debugCamController{};

// Player uses WASD
playerController.keys = {}; // Default WASD

// Debug camera uses IJKL
debugCamController.keys.moveForward = GLFW_KEY_I;
debugCamController.keys.moveBackward = GLFW_KEY_K;
debugCamController.keys.moveLeft = GLFW_KEY_J;
debugCamController.keys.moveRight = GLFW_KEY_L;

// In game loop
if (debugMode) {
  debugCamController.moveInPlaneXZ(window.getGLFWwindow(), dt, debugCamera);
} else {
  playerController.moveInPlaneXZ(window.getGLFWwindow(), dt, player);
}
```

## Integration with Camera

The KeyboardMovementController is designed to work seamlessly with the Camera component's view transformation methods.

### Camera.setViewYXZ() Integration

The controller directly outputs rotation in Yaw-Pitch-Roll format (Y-X-Z Euler angles), which matches `Camera::setViewYXZ()` perfectly.

```cpp
// Controller updates GameObject transform
cameraController.moveInPlaneXZ(window.getGLFWwindow(), frameTime, viewerObject);

// Camera reads the transform directly
camera.setViewYXZ(
  viewerObject.transform.translation,  // Camera position
  viewerObject.transform.rotation      // Camera orientation (yaw, pitch, roll)
);
```

**Rotation Axes:**
- `rotation.x` → Pitch (up/down looking)
- `rotation.y` → Yaw (left/right turning)
- `rotation.z` → Roll (typically unused, remains 0)

### Why Not setViewTarget()?

While `Camera::setViewTarget()` is available, `setViewYXZ()` is preferred for interactive control because:

1. **Direct rotation control** - User input directly manipulates Euler angles
2. **Predictable behavior** - Rotation clamping prevents unexpected camera flips
3. **No target point needed** - View direction calculated from angles
4. **Consistent with FPS controls** - Standard first-person camera behavior

**Comparison:**

```cpp
// setViewYXZ - Direct angle control (preferred for keyboard input)
camera.setViewYXZ(position, rotation);

// setViewTarget - Look at specific point (better for cutscenes/tracking)
camera.setViewTarget(cameraPos, targetPos);
```

## Design Decisions

### Frame-Rate Independence

**Problem:** Without delta time scaling, movement speed varies with frame rate.

**Solution:** All movement and rotation scale by `dt` (delta time in seconds):
```cpp
gameObject.transform.rotation += lookSpeed * dt * glm::normalize(rotate);
gameObject.transform.translation += moveSpeed * dt * glm::normalize(moveDir);
```

**Result:** Consistent movement speed whether running at 30 FPS or 240 FPS.

### Rotation Clamping

**Pitch Clamping:**
```cpp
gameObject.transform.rotation.x = glm::clamp(gameObject.transform.rotation.x, -1.5f, 1.5f);
```

- **Range:** Approximately ±85° (±1.5 radians)
- **Purpose:** Prevents camera from flipping upside down at ±90°
- **Why not ±90°?** Gimbal lock occurs exactly at ±π/2, so stay slightly under

**Yaw Wrapping:**
```cpp
gameObject.transform.rotation.y = glm::mod(gameObject.transform.rotation.y, glm::two_pi<float>());
```

- **Range:** [0, 2π) radians
- **Purpose:** Keeps yaw in canonical range, prevents float precision loss
- **Effect:** Seamless 360° rotation without numerical drift

### Movement in Local Space

Movement directions are calculated relative to the camera's current yaw:

```cpp
float yaw = gameObject.transform.rotation.y;
const glm::vec3 forwardDir{sin(yaw), 0.0f, cos(yaw)};
const glm::vec3 rightDir{forwardDir.z, 0.0f, -forwardDir.x};
```

**Why Local Space?**
- **Forward (W)** moves where the camera is looking (Z-axis)
- **Right (D)** strafes perpendicular to forward (X-axis)
- **Up (E)** always moves world-up, not camera-relative

This creates intuitive FPS-style controls where forward is "where you're looking."

### Epsilon Comparison for Normalization

```cpp
if (glm::dot(rotate, rotate) > std::numeric_limits<float>::epsilon())
```

**Purpose:** Avoid normalizing zero vectors, which is mathematically undefined.

**Why dot product?** `dot(v, v)` equals the squared magnitude, avoiding expensive `sqrt()`.

**Result:** Only normalize when actual input is detected, preventing NaN values.

### Up Direction Choice

```cpp
const glm::vec3 upDir{0.0f, -1.0f, 0.0f};
```

**Why negative Y?** Depends on coordinate system:
- **Vulkan NDC:** Y-axis points down (0 at top, 1 at bottom)
- **World space:** Typically Y-up, but camera view may invert it
- **This implementation:** Negative Y moves camera "up" in world space

*Note:* Coordinate system conventions vary by engine. Check your Camera implementation.

## Common Use Cases

### First-Person Camera

Standard FPS-style camera with WASD movement and arrow key look.

```cpp
Camera camera{};
auto viewerObject = GameObject::createGameObject();
viewerObject.transform.translation = {0.0f, 0.0f, -2.5f}; // Start position

KeyboardMovementController cameraController{};
cameraController.moveSpeed = 3.0f;
cameraController.lookSpeed = 1.5f;

// In game loop
cameraController.moveInPlaneXZ(window.getGLFWwindow(), deltaTime, viewerObject);
camera.setViewYXZ(viewerObject.transform.translation, viewerObject.transform.rotation);
```

### Third-Person Follow Camera

Control a player object, camera follows separately.

```cpp
auto player = GameObject::createGameObject();
KeyboardMovementController playerController{};

auto cameraObject = GameObject::createGameObject();
Camera camera{};

// In game loop
playerController.moveInPlaneXZ(window.getGLFWwindow(), deltaTime, player);

// Camera follows player with offset
glm::vec3 cameraOffset = {0.0f, 1.5f, -3.0f}; // Behind and above
cameraObject.transform.translation = player.transform.translation + cameraOffset;
cameraObject.transform.rotation = player.transform.rotation;

camera.setViewYXZ(cameraObject.transform.translation, cameraObject.transform.rotation);
```

### Spectator/Debug Camera

Free-flying camera for level editing or debugging.

```cpp
KeyboardMovementController debugCam{};
debugCam.moveSpeed = 10.0f;  // Fast movement
debugCam.lookSpeed = 2.5f;   // Responsive look

auto debugViewerObject = GameObject::createGameObject();
debugViewerObject.transform.translation = {0.0f, 5.0f, 0.0f}; // Start elevated

// No collision, no physics - pure free flight
debugCam.moveInPlaneXZ(window.getGLFWwindow(), deltaTime, debugViewerObject);
camera.setViewYXZ(debugViewerObject.transform.translation, debugViewerObject.transform.rotation);
```

## Troubleshooting

### Issue 1: Camera Moves Too Fast or Too Slow

**Symptoms:**
- Camera zooms across the scene uncontrollably
- Camera barely moves when keys are pressed

**Common Causes:**
1. **Delta time is wrong** - Check `dt` calculation
2. **moveSpeed misconfigured** - Check default value (3.0f)
3. **Frame time units wrong** - Ensure `dt` is in seconds, not milliseconds

**Solutions:**

```cpp
// Verify delta time is in seconds
auto newTime = std::chrono::high_resolution_clock::now();
float frameTime = std::chrono::duration<float, std::chrono::seconds::period>(
  newTime - currentTime
).count(); // Should be ~0.016 at 60 FPS, NOT 16.0

// Adjust speed parameters
controller.moveSpeed = 3.0f;   // Tune this value
controller.lookSpeed = 1.5f;   // Tune this value

// Debug output
std::cout << "Delta time: " << frameTime << "s\n";
std::cout << "Move speed: " << controller.moveSpeed << "\n";
```

### Issue 2: Camera Flips Upside Down

**Symptoms:**
- Looking up too far causes the camera to flip
- Controls become inverted
- Disorienting camera behavior

**Cause:** Pitch exceeds ±90°, causing gimbal lock.

**Solution:** The controller already clamps pitch to [-1.5, 1.5] radians. If still occurring:

```cpp
// Verify clamping is applied
gameObject.transform.rotation.x = glm::clamp(
  gameObject.transform.rotation.x, 
  -1.5f,  // ~-85 degrees
  1.5f    // ~+85 degrees
);

// Ensure rotation is applied correctly to camera
camera.setViewYXZ(viewerObject.transform.translation, viewerObject.transform.rotation);
```

### Issue 3: No Movement Response

**Symptoms:**
- Pressing keys has no effect
- Camera stays frozen

**Common Causes:**
1. **Wrong window handle** - Passing incorrect GLFWwindow pointer
2. **Delta time is zero** - Time not updating between frames
3. **GameObject not connected to camera** - Transform changes not affecting view

**Solutions:**

```cpp
// 1. Verify window handle
GLFWwindow* windowHandle = window.getGLFWwindow();
if (windowHandle == nullptr) {
  // Window.hpp needs getGLFWwindow() method
}

// 2. Verify delta time is non-zero
if (frameTime <= 0.0f) {
  std::cerr << "Invalid frame time: " << frameTime << "\n";
}

// 3. Verify GameObject updates camera
cameraController.moveInPlaneXZ(windowHandle, frameTime, viewerObject);
camera.setViewYXZ(viewerObject.transform.translation, viewerObject.transform.rotation);
// Must be called AFTER moveInPlaneXZ

// 4. Debug key state
if (glfwGetKey(windowHandle, GLFW_KEY_W) == GLFW_PRESS) {
  std::cout << "W key pressed\n";
}
```

### Issue 4: Jittery or Unstable Movement

**Symptoms:**
- Camera stutters or jumps
- Movement not smooth
- Inconsistent speed

**Common Causes:**
1. **Variable frame rate** - Delta time fluctuates wildly
2. **Time calculation error** - Clock not monotonic
3. **Input polling issues** - `glfwPollEvents()` not called

**Solutions:**

```cpp
// Ensure glfwPollEvents called every frame BEFORE input processing
while (!window.shouldClose()) {
  glfwPollEvents();  // MUST be first
  
  // Calculate time
  auto newTime = std::chrono::high_resolution_clock::now();
  float frameTime = std::chrono::duration<float>(newTime - currentTime).count();
  currentTime = newTime;
  
  // Cap delta time to prevent huge jumps
  frameTime = glm::min(frameTime, 0.1f); // Max 100ms per frame
  
  cameraController.moveInPlaneXZ(window.getGLFWwindow(), frameTime, viewerObject);
  // ...
}
```

### Issue 5: Rotation Wraps Unexpectedly

**Symptoms:**
- Yaw jumps from 2π back to 0
- Rotation direction reverses
- Interpolation issues

**Cause:** Yaw wrapping at 2π boundary.

**Solution:** This is expected behavior. For smooth interpolation, use:

```cpp
// For interpolation between rotations
float angleDiff = targetYaw - currentYaw;
angleDiff = glm::mod(angleDiff + glm::pi<float>(), glm::two_pi<float>()) - glm::pi<float>();
// This finds the shortest angular distance
```

## Performance Considerations

### Computational Cost

**Per-Frame Operations:**
- 10 `glfwGetKey()` calls (very fast, just state reads)
- Vector normalization (when input detected): ~2-3 `normalize()` calls per frame
- Trigonometric functions: `sin()`, `cos()` once per frame
- Glm vector operations: Addition, multiplication

**Overall:** Negligible performance impact, suitable for real-time use at any frame rate.

### Optimization Opportunities

**1. Cache Direction Vectors (if rotation unchanged):**
```cpp
// Only recalculate if yaw changed
static float cachedYaw = 0.0f;
static glm::vec3 cachedForward, cachedRight;

if (gameObject.transform.rotation.y != cachedYaw) {
  cachedYaw = gameObject.transform.rotation.y;
  cachedForward = {sin(cachedYaw), 0.0f, cos(cachedYaw)};
  cachedRight = {cachedForward.z, 0.0f, -cachedForward.x};
}
```

**2. Input Buffering (for network games):**
```cpp
// Store input states for later replay/reconciliation
struct InputState {
  glm::vec3 moveDir;
  glm::vec3 rotateDir;
  float timestamp;
};
```

## Future Enhancements

Potential improvements and extensions:

### Mouse Look Integration

```cpp
class KeyboardMovementController {
public:
  void moveInPlaneXZ(GLFWwindow* window, float dt, GameObject& gameObject);
  void mouseLook(GLFWwindow* window, double xpos, double ypos, GameObject& gameObject);
  
  bool enableMouseLook = false;
  float mouseSensitivity = 0.1f;
  
private:
  double lastMouseX = 0.0;
  double lastMouseY = 0.0;
  bool firstMouse = true;
};
```

### Smooth Acceleration/Deceleration

```cpp
class KeyboardMovementController {
public:
  float acceleration = 10.0f;
  float deceleration = 8.0f;
  
private:
  glm::vec3 currentVelocity{0.0f};
  
  // In moveInPlaneXZ:
  // Apply acceleration toward target velocity
  // currentVelocity += (targetVelocity - currentVelocity) * acceleration * dt;
};
```

### Sprint/Crouch Modifiers

```cpp
class KeyboardMovementController {
public:
  int sprintKey = GLFW_KEY_LEFT_SHIFT;
  int crouchKey = GLFW_KEY_LEFT_CONTROL;
  float sprintMultiplier = 2.0f;
  float crouchMultiplier = 0.5f;
};
```

### Gamepad Support

```cpp
void moveInPlaneXZ(GLFWwindow* window, int gamepadID, float dt, GameObject& gameObject);
// Use glfwGetGamepadState() for analog stick input
```

## Related Documentation

- **[Camera](CAMERA.md)** - Projection and view transformation systems
- **[GameObject](GAMEOBJECT.md)** - Entity and transform component details
- **[Window](WINDOW.md)** - GLFWwindow access and input handling
- **[Architecture](ARCHITECTURE.md)** - Overall system design and integration patterns

## Code Reference

**Header:** `engine/src/KeyboardMovementController.hpp`  
**Implementation:** `engine/src/KeyboardMovementController.cpp`  
**Usage Example:** `engine/src/FirstApp.cpp`
