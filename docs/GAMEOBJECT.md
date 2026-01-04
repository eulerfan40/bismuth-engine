# GameObject Documentation

Complete technical documentation for the GameObject component system.

**Files:** `engine/src/GameObject.hpp`, `engine/src/GameObject.cpp`  
**Purpose:** Game object representation with transform and rendering data  
**Pattern:** Component-based architecture with unique IDs

---

## Table of Contents

- [Overview](#overview)
- [Class Interface](#class-interface)
- [Transform2DComponent](#transform2dcomponent)
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
triangle.transform2D.translation = {0.2f, 0.0f};
triangle.transform2D.scale = {2.0f, 0.5f};
triangle.transform2D.rotation = glm::radians(90.0f);
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
    Transform2DComponent transform2D{};

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
Transform2DComponent transform2D{};
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

**Transform2D:**
- Position, rotation, scale in 2D space
- See [Transform2DComponent](#transform2dcomponent) section

---

## Transform2DComponent

### Structure Definition

```cpp
struct Transform2DComponent {
    glm::vec2 translation{};      // Position offset
    glm::vec2 scale{1.f, 1.f};    // Scale factor (default: 1.0 = original size)
    float rotation;               // Rotation in radians

    glm::mat2 mat2();             // Generate transformation matrix
};
```

### Translation

```cpp
glm::vec2 translation{};
```

**Purpose:** Position offset in clip space coordinates.

**Coordinate System:**
- x: -1.0 (left) to +1.0 (right)
- y: -1.0 (top) to +1.0 (bottom)
- Origin (0, 0) is screen center

**Example:**
```cpp
transform2D.translation = {0.5f, -0.3f};  // Right and slightly up
```

### Scale

```cpp
glm::vec2 scale{1.f, 1.f};
```

**Purpose:** Non-uniform scaling along x and y axes.

**Default:** `{1.0f, 1.0f}` - original size

**Examples:**
```cpp
scale = {2.0f, 2.0f};   // 2x larger in both dimensions
scale = {0.5f, 0.5f};   // Half size
scale = {2.0f, 0.5f};   // Wide and short (stretched horizontally)
scale = {1.0f, -1.0f};  // Vertical flip
```

**Non-Uniform Scaling:**
- Independent x and y scaling
- Can create stretched/squashed effects
- Useful for aspect ratio adjustments

### Rotation

```cpp
float rotation;
```

**Purpose:** Rotation angle in radians (counter-clockwise).

**Radian Conversion:**
```cpp
rotation = glm::radians(90.0f);           // 90 degrees
rotation = 0.25f * glm::two_pi<float>();  // Quarter turn
rotation = glm::two_pi<float>();          // Full circle (360°)
```

**Counter-Clockwise:**
- Positive rotation rotates counter-clockwise
- 0.0 = no rotation (points right)
- π/2 (90°) = points up
- π (180°) = points left
- 3π/2 (270°) = points down

**Animation Example:**
```cpp
// Continuous rotation (in render loop)
obj.transform2D.rotation = glm::mod(obj.transform2D.rotation + 0.01f, glm::two_pi<float>());
```

### Transformation Matrix

```cpp
glm::mat2 mat2() {
    const float s = glm::sin(rotation);
    const float c = glm::cos(rotation);
    glm::mat2 rotMatrix{{c, s}, {-s, c}};
    glm::mat2 scaleMat{{scale.x, 0.0f}, {0.0f, scale.y}};
    return rotMatrix * scaleMat;
}
```

**Purpose:** Combine rotation and scale into single 2×2 matrix.

**Matrix Math:**

**Rotation Matrix:**
```
[ cos(θ)  sin(θ) ]
[ -sin(θ) cos(θ) ]
```

**Scale Matrix:**
```
[ scale.x    0     ]
[   0     scale.y  ]
```

**Combined (Rotation × Scale):**
```
[ cos(θ)*scale.x   sin(θ)*scale.y  ]
[ -sin(θ)*scale.x  cos(θ)*scale.y  ]
```

**Order Matters:**
- `rotMatrix * scaleMat`: Scale first, then rotate (current)
- `scaleMat * rotMatrix`: Rotate first, then scale (different result!)

**Current order:** Scale → Rotate → Translate

**Application in Shader:**
```glsl
// Vertex shader
gl_Position = vec4(push.transform * position + push.offset, 0.0, 1.0);
                   ^^^^^^^^^^^^^^^^ ^^^^^^^^   ^^^^^^^^^^^^
                   Rotate & Scale   Vertex     Translation
```

**Why mat2 (not mat3)?**
- 2D transformations only need 2×2 for rotation/scale
- Translation applied separately (as vec2 offset)
- More efficient than 3×3 homogeneous matrix
- Smaller push constant size (16 bytes vs 36 bytes)

**Transformation Sequence:**
```
Vertex Position (from Model)
    ↓
Apply Scale (stretch/squash)
    ↓
Apply Rotation (orient)
    ↓
Apply Translation (position)
    ↓
Final Position (clip space)
```

---

## GameObject Management

### Creation Pattern

```cpp
// In FirstApp::loadGameObjects()
auto model = std::make_shared<Model>(device, vertices);

auto triangle = GameObject::createGameObject();
triangle.model = model;
triangle.color = {0.1f, 0.8f, 0.1f};
triangle.transform2D.translation = {0.2f, 0.0f};
triangle.transform2D.scale = {2.0f, 0.5f};
triangle.transform2D.rotation = glm::radians(90.0f);

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
        obj.transform2D.rotation = glm::mod(obj.transform2D.rotation + 0.01f, glm::two_pi<float>());

        // Prepare push constants
        SimplePushConstantData push{};
        push.offset = obj.transform2D.translation;
        push.color = obj.color;
        push.transform = obj.transform2D.mat2();

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
    └── transform2D (spatial component)
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
    Transform2D transform;
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
    std::vector<Transform2D> transforms;
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
        triangle.transform2D.translation = {-0.8f + 0.4f * i, 0.0f};  // Spread horizontally
        triangle.transform2D.scale = {0.5f, 0.5f};  // Half size
        gameObjects.push_back(std::move(triangle));
    }
}
```

### Example 2: Animated Spinner

```cpp
auto spinner = GameObject::createGameObject();
spinner.model = triangleModel;
spinner.color = {1.0f, 0.5f, 0.0f};  // Orange
spinner.transform2D.translation = {0.0f, 0.0f};  // Center
spinner.transform2D.scale = {0.8f, 0.8f};
spinner.transform2D.rotation = 0.0f;

gameObjects.push_back(std::move(spinner));

// In renderGameObjects():
obj.transform2D.rotation += 0.05f;  // Rotate every frame
```

### Example 3: Pulsing Scale Animation

```cpp
// In renderGameObjects():
static float time = 0.0f;
time += 0.016f;  // ~60 FPS

for (auto& obj : gameObjects) {
    float pulse = 1.0f + 0.3f * glm::sin(time * 2.0f);  // Oscillate between 0.7 and 1.3
    obj.transform2D.scale = {pulse, pulse};
    
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
triangle.transform2D.translation = {-0.5f, 0.0f};

auto square = GameObject::createGameObject();
square.model = squareModel;
square.transform2D.translation = {0.5f, 0.0f};

gameObjects.push_back(std::move(triangle));
gameObjects.push_back(std::move(square));
```

---

## Integration with Rendering

### Data Flow

```
GameObject                        GPU
----------                        ---
transform2D.translation    →    push.offset (vec2)
transform2D.mat2()         →    push.transform (mat2)
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
obj.transform2D.translation = {5.0f, 5.0f};  // Outside clip space [-1, 1]!
```

**Solution:** Keep translation within visible range [-1.0, 1.0].

3. **Scale too small:**
```cpp
obj.transform2D.scale = {0.0001f, 0.0001f};  // Invisible!
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
obj.transform2D.rotation = 90.0f;  // WRONG - should be radians!
```

**Solution:**
```cpp
obj.transform2D.rotation = glm::radians(90.0f);  // Correct
```

2. **Transform not uploaded to GPU:**
```cpp
SimplePushConstantData push{};
push.offset = obj.transform2D.translation;
// Forgot: push.transform = obj.transform2D.mat2();
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
auto mat = obj.transform2D.mat2();
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
obj.transform2D.rotation += 0.5f;  // Too fast! (~28° per frame)
```

**Solution:**
```cpp
obj.transform2D.rotation += 0.01f;  // ~0.57° per frame (smooth)
obj.transform2D.rotation = glm::mod(obj.transform2D.rotation, glm::two_pi<float>());
```

Or use delta time:
```cpp
float deltaTime = 0.016f;  // Frame time
obj.transform2D.rotation += glm::radians(60.0f) * deltaTime;  // 60°/sec
```

---

## Future Enhancements

### 1. Hierarchical Transforms (Parent-Child)

**Goal:** Objects with parent-child relationships.

```cpp
struct Transform2DComponent {
    glm::vec2 translation{};
    glm::vec2 scale{1.f, 1.f};
    float rotation;
    
    GameObject* parent = nullptr;  // NEW
    
    glm::mat3 worldMatrix() {
        glm::mat3 local = localMatrix();
        if (parent) {
            return parent->transform2D.worldMatrix() * local;
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
        {"translation", {transform2D.translation.x, transform2D.translation.y}},
        {"scale", {transform2D.scale.x, transform2D.scale.y}},
        {"rotation", transform2D.rotation}
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
    Transform2D interpolate();
};
```

---

## Related Documentation

- [MODEL.md](MODEL.md) - Vertex buffer and geometry data
- [SHADER.md](SHADER.md) - Transformation in shaders
- [PIPELINE.md](PIPELINE.md) - Push constants and pipeline layout
- [ARCHITECTURE.md](ARCHITECTURE.md) - Overall rendering architecture

---

