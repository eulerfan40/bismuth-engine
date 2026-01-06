# GameObject Documentation

Complete technical documentation for the GameObject component system.

**Files:** `engine/src/GameObject.hpp`, `engine/src/GameObject.cpp`  
**Purpose:** Game object representation with transform, rendering data, and lighting support  
**Pattern:** Component-based architecture with unique IDs

---

## Table of Contents

- [Overview](#overview)
- [Class Interface](#class-interface)
- [transformComponent](#transformcomponent)
- [GameObject Management](#gameobject-management)
- [Design Patterns](#design-patterns)
- [Usage Examples](#usage-examples)
- [Integration with Rendering](#integration-with-rendering)
- [Common Issues](#common-issues)
- [Future Enhancements](#future-enhancements)

---

## Overview

### What Is a GameObject?

A **GameObject** represents a renderable entity in the scene with:
- **Unique ID:** Distinguishes objects for tracking and management
- **Model:** Reference to geometry and vertex data
- **Color:** RGB tint applied during rendering
- **Transform:** Position, rotation, and scale in 2D space

```cpp
GameObject triangle = GameObject::createGameObject();
triangle.model = triangleModel;
triangle.color = {0.1f, 0.8f, 0.1f};  // Green
triangle.transform.translation = {0.2f, 0.0f};
triangle.transform.scale = {2.0f, 0.5f};
triangle.transform.rotation = glm::radians(90.0f);
```

### Purpose

**Separation of Concerns:**
- **Model:** Geometry data (vertices, indices) - reusable across objects
- **GameObject:** Instance-specific data (position, color, scale)
- **FirstApp:** Scene management and rendering orchestration

**Benefits:**
- Multiple objects can share the same model (instancing ready)
- Easy to add/remove objects from scene
- Transforms isolated from geometry
- Foundation for entity-component system (ECS)

---

## Class Interface

### Header Definition

```cpp
namespace engine {
  class GameObject {
  public:
    using id_t = unsigned int;

    static GameObject createGameObject();

    GameObject(const GameObject &) = delete;
    GameObject &operator=(const GameObject &) = delete;
    GameObject(GameObject &&) = default;
    GameObject &operator=(GameObject &&) = default;

    const id_t getId() { return id; }

    std::shared_ptr<Model> model{};
    glm::vec3 color{};
    TransformComponent transform{};

  private:
    GameObject(id_t objId) : id{objId} {}
    id_t id;
  };
}
```

### Factory Method

```cpp
static GameObject createGameObject();
```

**Purpose:** Create new GameObject with unique ID.

**Why Static Factory?**
- Constructor is private (controlled creation)
- Automatic ID generation
- Ensures all objects have unique identifiers

**Implementation:**
```cpp
static GameObject createGameObject() {
    static id_t currentId = 0;
    return GameObject(currentId++);
}
```

**ID Generation:**
- Starts at 0
- Increments for each new object
- Static variable persists across calls
- Thread-unsafe (acceptable for single-threaded engine)

**Usage:**
```cpp
auto obj1 = GameObject::createGameObject();  // ID = 0
auto obj2 = GameObject::createGameObject();  // ID = 1
auto obj3 = GameObject::createGameObject();  // ID = 2
```

### Move Semantics

```cpp
GameObject(const GameObject &) = delete;           // No copy
GameObject &operator=(const GameObject &) = delete; // No copy assign
GameObject(GameObject &&) = default;                // Move OK
GameObject &operator=(GameObject &&) = default;     // Move assign OK
```

**Why No Copying?**
- Each GameObject has unique ID
- Copying would duplicate ID (breaks uniqueness)
- Shared model pointers should not be duplicated carelessly

**Why Allow Moving?**
- Efficient transfer of ownership
- Required for storing in `std::vector`
- Preserves unique ID during transfer

**Example:**
```cpp
std::vector<GameObject> gameObjects;

auto triangle = GameObject::createGameObject();
triangle.model = model;

gameObjects.push_back(std::move(triangle));  // OK - move
// gameObjects.push_back(triangle);          // ERROR - can't copy
```

### Public Members

```cpp
std::shared_ptr<Model> model{};
glm::vec3 color{};
transformComponent transform{};
```

**Why Public?**
- Simple data container pattern
- Direct access is convenient
- Future: Could add getters/setters for validation

**Model as shared_ptr:**
- Multiple GameObjects can reference same Model
- Model persists until all references destroyed
- Efficient memory usage (geometry shared)

**Color (RGB):**
- Per-object color tint
- Passed to fragment shader via push constants
- Range: 0.0 (black) to 1.0 (full intensity)

**Transform:**
- Position, rotation, scale in 3D space
- See [TransformComponent](#transformcomponent) section

---

## TransformComponent

### Structure Definition

```cpp
struct TransformComponent {
    glm::vec3 translation{};            // Position offset (x, y, z)
    glm::vec3 scale{1.0f, 1.0f, 1.0f};  // Scale factor (default: 1.0 = original size)
    glm::vec3 rotation{};               // Rotation angles in radians (x, y, z)

    glm::mat4 mat4();                   // Generate 4×4 transformation matrix
    glm::mat3 normalMatrix();           // Generate 3×3 normal transformation matrix for lighting
};
```

### Translation

```cpp
glm::vec3 translation{};
```

**Purpose:** 3D position offset in world/clip space coordinates.

**Coordinate System:**
- x: -1.0 (left) to +1.0 (right)
- y: -1.0 (up) to +1.0 (down) - Vulkan's Y-axis points down
- z: 0.0 (near) to 1.0 (far) - Vulkan depth range with `GLM_FORCE_DEPTH_ZERO_TO_ONE`
- Origin (0, 0, 0) is screen center at near plane

**Example:**
```cpp
transform.translation = {0.5f, -0.3f, 0.5f};  // Right, up, mid-depth
```

**Important:** Without a camera/projection matrix, you're rendering directly in clip space, so Z values must be in the [0, 1] range to be visible.

### Scale

```cpp
glm::vec3 scale{1.0f, 1.0f, 1.0f};
```

**Purpose:** Non-uniform scaling along x, y, and z axes.

**Default:** `{1.0f, 1.0f, 1.0f}` - original size

**Examples:**
```cpp
scale = {2.0f, 2.0f, 2.0f};     // 2x larger in all dimensions
scale = {0.5f, 0.5f, 0.5f};     // Half size
scale = {2.0f, 0.5f, 1.0f};     // Wide, short, normal depth (stretched horizontally)
scale = {1.0f, -1.0f, 1.0f};    // Vertical flip
scale = {1.0f, 1.0f, 2.0f};     // Stretched along Z-axis (depth)
```

**Non-Uniform Scaling:**
- Independent x, y, and z scaling
- Can create stretched/squashed effects in 3D
- Useful for aspect ratio adjustments or special effects

### Rotation

```cpp
glm::vec3 rotation{};
```

**Purpose:** Rotation angles in radians for each axis (Euler angles).

**Components:**
- `rotation.x` - Rotation around X-axis (pitch)
- `rotation.y` - Rotation around Y-axis (yaw)
- `rotation.z` - Rotation around Z-axis (roll)

**Radian Conversion:**
```cpp
rotation.y = glm::radians(90.0f);          // 90 degrees around Y-axis
rotation.x = 0.25f * glm::two_pi<float>(); // Quarter turn around X-axis
rotation = {0.0f, glm::pi<float>(), 0.0f}; // 180° around Y-axis
```

**Rotation Order (Tait-Bryan Angles):**

The transformation matrix applies rotations in this order: **Z → X → Y**
- First: Rotate around Z-axis (roll)
- Second: Rotate around X-axis (pitch)
- Third: Rotate around Y-axis (yaw)

This corresponds to Tait-Bryan angles of Y(1), X(2), Z(3).

**Visual Guide:**
- **X-axis rotation (pitch):** Nodding your head up/down
- **Y-axis rotation (yaw):** Shaking your head left/right
- **Z-axis rotation (roll):** Tilting your head sideways

**Animation Examples:**
```cpp
// Continuous Y-axis rotation (spinning around vertical axis)
obj.transform.rotation.y = glm::mod(obj.transform.rotation.y + 0.01f, glm::two_pi<float>());

// Continuous X-axis rotation (tumbling forward)
obj.transform.rotation.x = glm::mod(obj.transform.rotation.x + 0.005f, glm::two_pi<float>());

// Combined rotation (complex motion)
obj.transform.rotation.y += 0.01f;
obj.transform.rotation.x += 0.005f;
```

**Gimbal Lock Warning:**
- Euler angles can suffer from gimbal lock at certain orientations
- Occurs when two rotation axes align
- For advanced applications, consider using quaternions instead

### Transformation Matrix

```cpp
glm::mat4 mat4() {
    const float c3 = glm::cos(rotation.z);
    const float s3 = glm::sin(rotation.z);
    const float c2 = glm::cos(rotation.x);
    const float s2 = glm::sin(rotation.x);
    const float c1 = glm::cos(rotation.y);
    const float s1 = glm::sin(rotation.y);
    return glm::mat4{
        {
            scale.x * (c1 * c3 + s1 * s2 * s3),
            scale.x * (c2 * s3),
            scale.x * (c1 * s2 * s3 - c3 * s1),
            0.0f,
        },
        {
            scale.y * (c3 * s1 * s2 - c1 * s3),
            scale.y * (c2 * c3),
            scale.y * (c1 * c3 * s2 + s1 * s3),
            0.0f,
        },
        {
            scale.z * (c2 * s1),
            scale.z * (-s2),
            scale.z * (c1 * c2),
            0.0f,
        },
        {translation.x, translation.y, translation.z, 1.0f}
    };
}
```

**Purpose:** Combine scale, rotation, and translation into a single 4×4 transformation matrix.

**Matrix Structure:**

```
[  Rotation & Scale (3×3)   |  0  ]
[         (combined)        |  0  ]
[                          |  0  ]
[-------------------------+-----]
[    Translation (x,y,z)   |  1  ]
```

**Transformation Order:** Scale → Rotate (Z→X→Y) → Translate

**Why This Specific Order?**

The matrix is constructed to apply transformations in this sequence:
1. **Scale:** Resize the object uniformly or non-uniformly
2. **Rotate (Z-axis):** Roll rotation
3. **Rotate (X-axis):** Pitch rotation  
4. **Rotate (Y-axis):** Yaw rotation
5. **Translate:** Move to final world position

**Matrix Breakdown:**

**Upper-left 3×3 block:** Combined rotation and scale
- Each element is a combination of sine/cosine terms from all three rotation axes
- Scale factors are multiplied into each row
- This is the result of matrix multiplication: Ry × Rx × Rz × Scale

**Bottom row:** Translation vector `{tx, ty, tz, 1}`
- Places the object at its world position
- Applied last in the transformation pipeline

**Right column:** Homogeneous coordinate `{0, 0, 0, 1}`
- Required for 4×4 affine transformation matrix
- The `1` in the bottom-right allows proper translation

**Application in Shader:**
```glsl
// Vertex shader
gl_Position = push.transform * vec4(position, 1.0);
              ^^^^^^^^^^^^^^   ^^^^^^^^^^^^^^^^^^
              4×4 Matrix       Vertex as vec4
```

**Transformation Flow:**
```
Local Vertex Position (from Model)
    ↓
Scale (resize object)
    ↓
Rotate Z (roll)
    ↓
Rotate X (pitch)
    ↓
Rotate Y (yaw)
    ↓
Translate (world position)
    ↓
Final Position (clip space)
```

**Why mat4 (not mat3)?**
- 3D transformations require 4×4 homogeneous matrices
- Includes translation in the matrix itself (unlike 2D which used separate offset)
- Standard for 3D graphics (OpenGL, DirectX, Vulkan all use 4×4)
- Enables projection transformations (future camera support)

**Performance Note:**
- Pre-computing sin/cos values (`c1, s1, c2, s2, c3, s3`) avoids redundant calculations
- Matrix construction happens once per object per frame
- Much faster than separate glm::rotate() and glm::scale() calls

**Tait-Bryan Angle Reference:**
This implementation uses Tait-Bryan angles (also called Cardan angles), which are commonly used in aerospace and robotics. The rotation order Y(1), X(2), Z(3) corresponds to yaw-pitch-roll.

**See:** [Wikipedia - Euler Angles](https://en.wikipedia.org/wiki/Euler_angles#Rotation_matrix) for detailed mathematical derivation.

### Normal Matrix

```cpp
glm::mat3 normalMatrix() {
    const float c3 = glm::cos(rotation.z);
    const float s3 = glm::sin(rotation.z);
    const float c2 = glm::cos(rotation.x);
    const float s2 = glm::sin(rotation.x);
    const float c1 = glm::cos(rotation.y);
    const float s1 = glm::sin(rotation.y);
    const glm::vec3 invScale = 1.0f / scale;

    return glm::mat3{
        {
            invScale.x * (c1 * c3 + s1 * s2 * s3),
            invScale.x * (c2 * s3),
            invScale.x * (c1 * s2 * s3 - c3 * s1)
        },
        {
            invScale.y * (c3 * s1 * s2 - c1 * s3),
            invScale.y * (c2 * c3),
            invScale.y * (c1 * c3 * s2 + s1 * s3)
        },
        {
            invScale.z * (c2 * s1),
            invScale.z * (-s2),
            invScale.z * (c1 * c2)
        }
    };
}
```

**Purpose:** Generate a 3×3 matrix for transforming surface normals in lighting calculations.

**Why Normal Matrix is Different:**

Normal vectors require special treatment because they represent directions perpendicular to surfaces, not positions. When an object is scaled non-uniformly, normals need to be transformed differently to maintain perpendicularity.

**Key Differences from Model Matrix:**
1. **3×3 (not 4×4):** Normals are directions, not positions - no translation needed
2. **Inverse scale:** Uses `1.0f / scale` instead of `scale`
3. **No translation:** Translation doesn't affect direction vectors

**Why Inverse Scale?**

Consider a sphere scaled by `{2.0, 1.0, 1.0}` (stretched horizontally):
- **Without inverse scale:** Normals would stretch with the surface, pointing in wrong directions
- **With inverse scale:** Normals contract perpendicular to stretched axis, maintaining correct perpendicularity

**Mathematical Background:**

The normal matrix is the transpose of the inverse of the upper-left 3×3 portion of the model matrix:
```
Normal Matrix = transpose(inverse(mat3(modelMatrix)))
```

For our transform (rotation + non-uniform scale), this simplifies to applying rotation with inverse scale.

**Usage in Rendering:**

```cpp
// In render system
SimplePushConstantData push{};
push.transform = projectionView * obj.transform.mat4();
push.normalMatrix = obj.transform.normalMatrix();

// In vertex shader
vec3 normalWorldSpace = normalize(mat3(push.normalMatrix) * normal);
```

**Performance Note:**
- Computed once per object per frame
- More efficient than computing inverse-transpose on GPU
- Pre-computes sin/cos values (shared with mat4() calculation)

**When Normals Don't Need Special Treatment:**
- **Uniform scale:** If `scale.x == scale.y == scale.z`, inverse scale equals `1/scale` uniformly
- **Rotation only:** Pure rotation preserves perpendicularity
- **Translation only:** Doesn't affect normals (they're directions)

**Example Transformation:**

```cpp
// Object scaled wider in X direction
transform.scale = {2.0f, 1.0f, 1.0f};
transform.rotation = {0.0f, glm::radians(45.0f), 0.0f};

// Normal pointing up (0, 1, 0)
glm::vec3 normal = {0.0f, 1.0f, 0.0f};

// After normal matrix transformation
glm::vec3 transformedNormal = normalMatrix() * normal;
// Result: Normal remains perpendicular to surface despite non-uniform scaling
```

**Implementation Location:**

Both `mat4()` and `normalMatrix()` are now implemented in `GameObject.cpp` (extracted from the header) to reduce compilation dependencies and improve code organization.

---

## GameObject Management

### Creation Pattern

```cpp
// In FirstApp::loadGameObjects()
auto model = std::make_shared<Model>(device, vertices);

auto triangle = GameObject::createGameObject();
triangle.model = model;
triangle.color = {0.1f, 0.8f, 0.1f};
triangle.transform.translation = {0.2f, 0.0f};
triangle.transform.scale = {2.0f, 0.5f};
triangle.transform.rotation = glm::radians(90.0f);

gameObjects.push_back(std::move(triangle));
```

**Steps:**
1. Create shared Model (geometry)
2. Create GameObject via factory
3. Assign model reference
4. Set color and transform
5. Move into scene container

### Storage

```cpp
// In FirstApp.hpp
std::vector<GameObject> gameObjects;
```

**Why vector?**
- Contiguous memory (cache-friendly)
- Dynamic size (add/remove objects)
- Sequential iteration during rendering

**Alternatives:**
- `std::unordered_map<id_t, GameObject>` - Fast lookup by ID
- `std::list<GameObject>` - Stable pointers, slower iteration
- Custom pools - Advanced memory management

### Rendering Loop

```cpp
void FirstApp::renderGameObjects(VkCommandBuffer commandBuffer) {
    pipeline->bind(commandBuffer);

    for (auto& obj : gameObjects) {
        // Update animation
        obj.transform.rotation = glm::mod(obj.transform.rotation + 0.01f, glm::two_pi<float>());

        // Prepare push constants
        SimplePushConstantData push{};
        push.offset = obj.transform.translation;
        push.color = obj.color;
        push.transform = obj.transform.mat2();

        // Upload to GPU
        vkCmdPushConstants(commandBuffer, pipelineLayout,
                          VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT,
                          0, sizeof(SimplePushConstantData), &push);

        // Render
        obj.model->bind(commandBuffer);
        obj.model->draw(commandBuffer);
    }
}
```

**Per-Object Rendering:**
1. Bind pipeline once (shared by all objects)
2. For each object:
   - Update transform (animation)
   - Build push constants from GameObject data
   - Upload push constants
   - Bind model's vertex buffer
   - Draw model

**Instancing:**
- Same model drawn multiple times with different transforms
- Efficient: Only upload small push constants, not vertex data

---

## Design Patterns

### Component-Based Architecture

**Current Structure:**
```
GameObject
    ├── id (unique identifier)
    ├── model (render component)
    ├── color (visual component)
    └── transform (spatial component)
```

**Benefits:**
- Composition over inheritance
- Easy to add new components
- Flexible and extensible

**Comparison to Traditional OOP:**
```cpp
// Traditional (inheritance)
class Entity { };
class RenderableEntity : public Entity { Model model; };
class ColoredEntity : public RenderableEntity { glm::vec3 color; };
// Adding new features requires new inheritance chains!

// Component-based (composition)
struct GameObject {
    Model* model;
    glm::vec3 color;
    transform transform;
    // Easy to add: PhysicsComponent, AudioComponent, etc.
};
```

### Factory Pattern

```cpp
static GameObject createGameObject();
```

**Advantages:**
- Controlled object creation
- Automatic ID management
- Future: Could add object pooling
- Future: Could register objects globally

**Alternative Patterns:**
- Builder pattern for complex initialization
- Prototype pattern for cloning objects
- Object pool for reusing objects

### Data-Oriented Design

**Current Approach:**
```cpp
std::vector<GameObject> gameObjects;  // Array of structures (AoS)
```

**Future Structure-of-Arrays (SoA):**
```cpp
struct GameObjectManager {
    std::vector<id_t> ids;
    std::vector<Model*> models;
    std::vector<glm::vec3> colors;
    std::vector<transform> transforms;
};
```

**SoA Benefits:**
- Better cache locality
- SIMD optimization opportunities
- Skip components not needed for specific systems

---

## Usage Examples

### Example 1: Multiple Triangles

```cpp
void FirstApp::loadGameObjects() {
    std::vector<Model::Vertex> vertices{
        {{0.0f, -0.5f}, {1.0f, 0.0f, 0.0f}},
        {{0.5f, 0.5f}, {0.0f, 1.0f, 0.0f}},
        {{-0.5f, 0.5f}, {0.0f, 0.0f, 1.0f}}
    };

    auto triangleModel = std::make_shared<Model>(device, vertices);

    // Create 5 triangles with different properties
    for (int i = 0; i < 5; i++) {
        auto triangle = GameObject::createGameObject();
        triangle.model = triangleModel;  // Share same geometry
        triangle.color = {0.2f * i, 0.0f, 1.0f - 0.2f * i};  // Blue to magenta gradient
        triangle.transform.translation = {-0.8f + 0.4f * i, 0.0f};  // Spread horizontally
        triangle.transform.scale = {0.5f, 0.5f};  // Half size
        gameObjects.push_back(std::move(triangle));
    }
}
```

### Example 2: Animated Spinner

```cpp
auto spinner = GameObject::createGameObject();
spinner.model = triangleModel;
spinner.color = {1.0f, 0.5f, 0.0f};  // Orange
spinner.transform.translation = {0.0f, 0.0f};  // Center
spinner.transform.scale = {0.8f, 0.8f};
spinner.transform.rotation = 0.0f;

gameObjects.push_back(std::move(spinner));

// In renderGameObjects():
obj.transform.rotation += 0.05f;  // Rotate every frame
```

### Example 3: Pulsing Scale Animation

```cpp
// In renderGameObjects():
static float time = 0.0f;
time += 0.016f;  // ~60 FPS

for (auto& obj : gameObjects) {
    float pulse = 1.0f + 0.3f * glm::sin(time * 2.0f);  // Oscillate between 0.7 and 1.3
    obj.transform.scale = {pulse, pulse};
    
    // Update push constants and render...
}
```

### Example 4: Different Geometries

```cpp
// Triangle
std::vector<Model::Vertex> triangleVerts{...};
auto triangleModel = std::make_shared<Model>(device, triangleVerts);

// Square
std::vector<Model::Vertex> squareVerts{
    {{-0.5f, -0.5f}, {1.0f, 0.0f, 0.0f}},
    {{0.5f, -0.5f}, {0.0f, 1.0f, 0.0f}},
    {{0.5f, 0.5f}, {0.0f, 0.0f, 1.0f}},
    {{-0.5f, 0.5f}, {1.0f, 1.0f, 0.0f}}
};
auto squareModel = std::make_shared<Model>(device, squareVerts);

auto triangle = GameObject::createGameObject();
triangle.model = triangleModel;
triangle.transform.translation = {-0.5f, 0.0f};

auto square = GameObject::createGameObject();
square.model = squareModel;
square.transform.translation = {0.5f, 0.0f};

gameObjects.push_back(std::move(triangle));
```

### Example 5: Camera Viewer GameObject

**GameObjects can represent non-rendered entities like camera positions:**

```cpp
#include "KeyboardMovementController.hpp"

void FirstApp::run() {
    Camera camera{};
    
    // Create a GameObject to represent the camera/viewer position and orientation
    // This GameObject has no model - it's just a transform that drives the camera
    auto viewerObject = GameObject::createGameObject();
    viewerObject.transform.translation = {0.0f, 0.0f, -2.5f};  // Initial camera position
    viewerObject.transform.rotation = {0.0f, 0.0f, 0.0f};      // Initial camera rotation
    
    // Controller modifies the viewer GameObject's transform
    KeyboardMovementController cameraController{};
    cameraController.moveSpeed = 3.0f;
    cameraController.lookSpeed = 1.5f;
    
    auto currentTime = std::chrono::high_resolution_clock::now();
    
    while (!window.shouldClose()) {
        glfwPollEvents();
        
        // Calculate delta time
        auto newTime = std::chrono::high_resolution_clock::now();
        float frameTime = std::chrono::duration<float, std::chrono::seconds::period>(
            newTime - currentTime
        ).count();
        currentTime = newTime;
        
        // Clamp frame time to prevent huge jumps (window drag, breakpoints, etc.)
        const float MAX_FRAME_TIME = 1.0f;
        frameTime = glm::min(frameTime, MAX_FRAME_TIME);
        
        // Update viewer GameObject based on keyboard input
        cameraController.moveInPlaneXZ(window.getGLFWwindow(), frameTime, viewerObject);
        
        // Camera reads viewer GameObject's transform
        camera.setViewYXZ(viewerObject.transform.translation, viewerObject.transform.rotation);
        
        // Set projection and render...
        float aspect = renderer.getAspectRatio();
        camera.setPerspectiveProjection(glm::radians(50.0f), aspect, 0.1f, 10.0f);
        
        // Render scene from camera's perspective
        if (auto commandBuffer = renderer.beginFrame()) {
            renderer.beginSwapChainRenderPass(commandBuffer);
            simpleRenderSystem.renderGameObjects(commandBuffer, gameObjects, camera);
            renderer.endSwapChainRenderPass(commandBuffer);
            renderer.endFrame();
        }
    }
}
```

**Key Points:**

1. **GameObject as Data Container:** The viewer GameObject has no `model` - it's purely a transform container.
   - `viewerObject.transform.translation` → Camera position
   - `viewerObject.transform.rotation` → Camera orientation (pitch, yaw, roll)

2. **Separation of Concerns:**
   - `KeyboardMovementController` modifies GameObject transforms
   - `Camera` reads GameObject transforms
   - Neither component needs to know about the other

3. **Reusability:** The same GameObject/transform system used for rendered objects also works for:
   - Camera/viewer positions
   - Invisible target points
   - Waypoints and markers
   - Collision volumes
   - AI navigation nodes

4. **Transform Component Consistency:** All systems use the same `TransformComponent` structure:
   ```cpp
   viewerObject.transform.translation  // vec3 position
   viewerObject.transform.rotation     // vec3 Euler angles (yaw, pitch, roll)
   viewerObject.transform.scale        // vec3 scale (unused for camera)
   ```

**See:** [KEYBOARDMOVEMENTCONTROLLER.md](KEYBOARDMOVEMENTCONTROLLER.md) for camera control details

---

## Integration with Rendering

### Data Flow

```
GameObject                        GPU
----------                        ---
transform.translation    →    push.offset (vec2)
transform.mat2()         →    push.transform (mat2)
color                      →    push.color (vec3)
model                      →    Vertex Buffer
    ↓
SimplePushConstantData
    ↓
vkCmdPushConstants()
    ↓
Shader reads push constants
    ↓
Vertex transformation:
    finalPos = transform * position + offset
```

### Push Constants Structure

```cpp
struct SimplePushConstantData {
    glm::mat2 transform{1.f};  // 16 bytes (aligned)
    glm::vec2 offset;          // 8 bytes
    alignas(16) glm::vec3 color;  // 12 bytes (aligned to 16)
    // Total: 40 bytes (within 128-byte limit)
};
```

**Memory Layout:**
```
Offset  Size  Field
0x00    16    mat2 transform
0x10    8     vec2 offset
0x18    12    vec3 color (aligned to 0x20)
0x20    -     (end)
```

### Shader Usage

```glsl
// Vertex shader
layout(push_constant) uniform Push {
  mat2 transform;
  vec2 offset;
  vec3 color;
} push;

void main() {
    gl_Position = vec4(push.transform * position + push.offset, 0.0, 1.0);
}

// Fragment shader
layout(push_constant) uniform Push {
  mat2 transform;
  vec2 offset;
  vec3 color;
} push;

void main() {
    outColor = vec4(push.color, 1.0);
}
```

**See:** [SHADER.md](SHADER.md) for complete shader documentation

---

## Common Issues

### Issue 1: GameObject Not Rendering

**Symptom:** Object created but nothing appears on screen.

**Possible Causes:**

1. **Model not assigned:**
```cpp
auto obj = GameObject::createGameObject();
// obj.model = nullptr; (forgot to assign!)
gameObjects.push_back(std::move(obj));
```

**Solution:** Always assign model before adding to scene.

2. **Position off-screen:**
```cpp
obj.transform.translation = {5.0f, 5.0f};  // Outside clip space [-1, 1]!
```

**Solution:** Keep translation within visible range [-1.0, 1.0].

3. **Scale too small:**
```cpp
obj.transform.scale = {0.0001f, 0.0001f};  // Invisible!
```

**Solution:** Use reasonable scale values (0.1 to 10.0 typical range).

4. **Color matches background:**
```cpp
obj.color = {0.0f, 0.0f, 0.0f};  // Black on black background!
```

**Solution:** Use contrasting colors.

### Issue 2: Rotation Not Working

**Symptom:** Setting rotation has no effect.

**Possible Causes:**

1. **Rotation in degrees instead of radians:**
```cpp
obj.transform.rotation = 90.0f;  // WRONG - should be radians!
```

**Solution:**
```cpp
obj.transform.rotation = glm::radians(90.0f);  // Correct
```

2. **Transform not uploaded to GPU:**
```cpp
SimplePushConstantData push{};
push.offset = obj.transform.translation;
// Forgot: push.transform = obj.transform.mat2();
```

**Solution:** Always call `mat2()` and assign to push constants.

### Issue 3: Objects Sharing Unwanted State

**Symptom:** Modifying one object affects another.

**Cause:** Accidentally copying instead of moving:
```cpp
auto obj1 = GameObject::createGameObject();
// auto obj2 = obj1;  // ERROR - copy deleted!
```

**Or sharing references incorrectly:**
```cpp
GameObject& obj1 = gameObjects[0];
GameObject& obj2 = obj1;  // Same object!
obj2.color = {1.0f, 0.0f, 0.0f};  // Both obj1 and obj2 affected
```

**Solution:** GameObjects can't be copied (by design). Use proper indexing or iteration.

### Issue 4: Transform Matrix Incorrect

**Symptom:** Object appears stretched, skewed, or wrong orientation.

**Debugging:**
```cpp
auto mat = obj.transform.mat2();
std::cout << "Transform matrix:\n";
std::cout << mat[0][0] << " " << mat[0][1] << "\n";
std::cout << mat[1][0] << " " << mat[1][1] << "\n";
```

**Identity matrix (no transformation):**
```
1.0  0.0
0.0  1.0
```

**90° rotation (no scale):**
```
0.0  1.0
-1.0  0.0
```

**Solution:** Verify rotation angle and scale values are correct.

### Issue 5: Animation Jittering

**Symptom:** Rotation or movement appears jerky.

**Cause:** Inconsistent frame times or excessive rotation speed:
```cpp
obj.transform.rotation += 0.5f;  // Too fast! (~28° per frame)
```

**Solution:**
```cpp
obj.transform.rotation += 0.01f;  // ~0.57° per frame (smooth)
obj.transform.rotation = glm::mod(obj.transform.rotation, glm::two_pi<float>());
```

Or use delta time:
```cpp
float deltaTime = 0.016f;  // Frame time
obj.transform.rotation += glm::radians(60.0f) * deltaTime;  // 60°/sec
```

---

## Future Enhancements

### 1. Hierarchical Transforms (Parent-Child)

**Goal:** Objects with parent-child relationships.

```cpp
struct transformComponent {
    glm::vec2 translation{};
    glm::vec2 scale{1.f, 1.f};
    float rotation;
    
    GameObject* parent = nullptr;  // NEW
    
    glm::mat3 worldMatrix() {
        glm::mat3 local = localMatrix();
        if (parent) {
            return parent->transform.worldMatrix() * local;
        }
        return local;
    }
};
```

**Use Cases:**
- Solar system (planets orbit sun, moons orbit planets)
- Character with weapons (weapon follows character)
- Complex objects (car with wheels)

### 2. Component System

**Goal:** Flexible component attachment.

```cpp
class GameObject {
    id_t id;
    std::unordered_map<std::type_index, std::unique_ptr<Component>> components;
    
    template<typename T>
    T* addComponent() { /* ... */ }
    
    template<typename T>
    T* getComponent() { /* ... */ }
};

// Usage
obj.addComponent<TransformComponent>();
obj.addComponent<RenderComponent>();
obj.addComponent<PhysicsComponent>();
obj.addComponent<AudioComponent>();
```

### 3. Object Pooling

**Goal:** Reuse GameObject allocations for performance.

```cpp
class GameObjectPool {
    std::vector<GameObject> pool;
    std::vector<size_t> freeIndices;
    
    GameObject* acquire();
    void release(GameObject* obj);
};
```

### 4. Serialization

**Goal:** Save/load GameObject state.

```cpp
nlohmann::json GameObject::toJson() {
    return {
        {"id", id},
        {"color", {color.r, color.g, color.b}},
        {"translation", {transform.translation.x, transform.translation.y}},
        {"scale", {transform.scale.x, transform.scale.y}},
        {"rotation", transform.rotation}
    };
}
```

### 5. 3D Transforms

**Goal:** Extend to 3D space.

```cpp
struct Transform3DComponent {
    glm::vec3 translation{};
    glm::vec3 scale{1.f};
    glm::quat rotation{};  // Quaternion for 3D rotation
    
    glm::mat4 mat4();  // 4x4 transformation matrix
};
```

### 6. Animation System

**Goal:** Keyframe-based animation.

```cpp
struct AnimationComponent {
    std::vector<Keyframe> keyframes;
    float currentTime;
    
    void update(float deltaTime);
    transform interpolate();
};
```

---

## Related Documentation

- [MODEL.md](MODEL.md) - Vertex buffer and geometry data
- [SHADER.md](SHADER.md) - Transformation in shaders
- [PIPELINE.md](PIPELINE.md) - Push constants and pipeline layout
- [ARCHITECTURE.md](ARCHITECTURE.md) - Overall rendering architecture

---

