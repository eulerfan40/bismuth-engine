# Model Component

The Model component manages vertex data and vertex buffers for rendering geometry in the Bismuth Engine. It provides an abstraction layer for loading vertex data into GPU memory and binding it to the graphics pipeline.

## Overview

**Purpose:** Encapsulate vertex data management, including buffer creation, memory allocation, and rendering commands.

**Key Responsibilities:**
- Store vertex data in GPU-accessible memory
- Define vertex input layout (binding and attribute descriptions)
- Bind vertex buffers to command buffers
- Issue draw commands

**Location:** `engine/src/Model.hpp`, `engine/src/Model.cpp`

---

## Architecture

### Class Structure

```cpp
class Model {
public:
    struct Vertex {
        glm::vec2 position;
        
        static std::vector<VkVertexInputBindingDescription> getBindingDescriptions();
        static std::vector<VkVertexInputAttributeDescription> getAttributeDescriptions();
    };

    Model(Device& device, const std::vector<Vertex>& vertices);
    ~Model();
    
    void bind(VkCommandBuffer commandBuffer);
    void draw(VkCommandBuffer commandBuffer);

private:
    void createVertexBuffers(const std::vector<Vertex>& vertices);
    
    Device& device;
    VkBuffer vertexBuffer;
    VkDeviceMemory vertexBufferMemory;
    uint32_t vertexCount;
};
```

### Vertex Structure

The `Vertex` struct defines the layout of vertex data:

```cpp
struct Vertex {
    glm::vec2 position;  // 2D position in clip space
};
```

**Current attributes:**
- `position` (vec2): 2D coordinates for triangle rendering

**Future expansion:** Additional attributes like colors, normals, texture coordinates can be added as fields in this struct.

---

## Initialization

### Constructor

```cpp
Model::Model(Device &device, const std::vector<Vertex> &vertices) : device{device} {
    createVertexBuffers(vertices);
}
```

**Parameters:**
- `device`: Reference to the Device component (needed for buffer creation)
- `vertices`: Vector of vertex data to upload to GPU

**What it does:**
1. Stores reference to Device
2. Calls `createVertexBuffers()` to allocate GPU memory

### Destructor

```cpp
Model::~Model() {
    vkDestroyBuffer(device.device(), vertexBuffer, nullptr);
    vkFreeMemory(device.device(), vertexBufferMemory, nullptr);
}
```

**Cleanup:**
- Destroys the Vulkan buffer object
- Frees the associated GPU memory

---

## Vertex Buffer Creation

### createVertexBuffers()

```cpp
void Model::createVertexBuffers(const std::vector<Vertex> &vertices) {
    vertexCount = static_cast<uint32_t>(vertices.size());
    assert(vertexCount >= 3 && "Vertex count must be at least 3.");

    VkDeviceSize bufferSize = sizeof(vertices[0]) * vertexCount;

    device.createBuffer(
        bufferSize,
        VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
        vertexBuffer,
        vertexBufferMemory);

    void *data;
    vkMapMemory(device.device(), vertexBufferMemory, 0, bufferSize, 0, &data);
    memcpy(data, vertices.data(), static_cast<size_t>(bufferSize));
    vkUnmapMemory(device.device(), vertexBufferMemory);
}
```

**Steps:**

1. **Validation:** Ensures at least 3 vertices (minimum for a triangle)
2. **Calculate size:** `sizeof(Vertex) * vertexCount`
3. **Create buffer:** Uses Device helper to allocate GPU memory
4. **Map memory:** Makes GPU memory accessible from CPU
5. **Copy data:** Transfers vertex data from CPU to GPU
6. **Unmap memory:** Releases CPU access to GPU memory

**Memory properties:**
- `VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT`: CPU can access this memory
- `VK_MEMORY_PROPERTY_HOST_COHERENT_BIT`: Writes are immediately visible to GPU (no manual flush needed)

**Trade-off:** Host-visible memory is slower than device-local memory, but simpler for small static geometry. For larger meshes or dynamic data, consider using staging buffers to transfer data to device-local memory.

---

## Vertex Input Descriptors

These static methods describe the vertex layout to the graphics pipeline.

### Binding Descriptions

```cpp
std::vector<VkVertexInputBindingDescription> Model::Vertex::getBindingDescriptions() {
    std::vector<VkVertexInputBindingDescription> bindingDescriptions(1);
    bindingDescriptions[0].binding = 0;
    bindingDescriptions[0].stride = sizeof(Vertex);
    bindingDescriptions[0].inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
    return bindingDescriptions;
}
```

**Fields:**
- `binding = 0`: Which binding point this buffer uses
- `stride = sizeof(Vertex)`: Bytes between consecutive vertices
- `inputRate = VK_VERTEX_INPUT_RATE_VERTEX`: Advance per vertex (not per instance)

**Purpose:** Tells Vulkan how to step through the vertex buffer.

### Attribute Descriptions

```cpp
std::vector<VkVertexInputAttributeDescription> Model::Vertex::getAttributeDescriptions() {
    std::vector<VkVertexInputAttributeDescription> attributeDescriptions(1);
    attributeDescriptions[0].binding = 0;
    attributeDescriptions[0].location = 0;
    attributeDescriptions[0].format = VK_FORMAT_R32G32_SFLOAT;
    attributeDescriptions[0].offset = 0;
    return attributeDescriptions;
}
```

**Fields:**
- `binding = 0`: References binding description [0]
- `location = 0`: Corresponds to `layout(location = 0)` in vertex shader
- `format = VK_FORMAT_R32G32_SFLOAT`: Two 32-bit floats (vec2)
- `offset = 0`: Position is at byte 0 of the Vertex struct

**Purpose:** Maps struct fields to vertex shader inputs.

**Shader correspondence:**
```glsl
layout(location = 0) in vec2 position;  // Matches location = 0, format = R32G32_SFLOAT
```

---

## Rendering Commands

### bind()

```cpp
void Model::bind(VkCommandBuffer commandBuffer) {
    VkBuffer buffers[] = {vertexBuffer};
    VkDeviceSize offsets[] = {0};
    vkCmdBindVertexBuffers(commandBuffer, 0, 1, buffers, offsets);
}
```

**What it does:**
- Binds the vertex buffer to binding point 0
- Sets offset to 0 (start at beginning of buffer)

**Must be called:** After binding the pipeline, before drawing.

### draw()

```cpp
void Model::draw(VkCommandBuffer commandBuffer) {
    vkCmdDraw(commandBuffer, vertexCount, 1, 0, 0);
}
```

**Parameters:**
- `vertexCount`: Number of vertices to draw
- `instanceCount = 1`: Not using instancing
- `firstVertex = 0`: Start at vertex 0
- `firstInstance = 0`: Start at instance 0

**Result:** Issues a draw call for all vertices in the buffer.

---

## Usage Example

### Loading a Triangle

```cpp
void FirstApp::loadModels() {
    std::vector<Model::Vertex> vertices {
        {{0.0f, -0.5f}},   // Top vertex
        {{0.5f, 0.5f}},    // Bottom-right vertex
        {{-0.5f, 0.5f}}    // bottom-left vertex
    };

    model = std::make_unique<Model>(device, vertices);
}
```

### Rendering Loop

```cpp
void FirstApp::createCommandBuffers() {
    // ... command buffer setup ...
    
    vkCmdBeginRenderPass(commandBuffers[i], &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);
    
    pipeline->bind(commandBuffers[i]);  // Bind pipeline first
    model->bind(commandBuffers[i]);     // Then bind vertex buffer
    model->draw(commandBuffers[i]);     // Finally, draw
    
    vkCmdEndRenderPass(commandBuffers[i]);
}
```

**Order matters:**
1. Begin render pass
2. Bind pipeline
3. Bind vertex buffers
4. Issue draw commands
5. End render pass

---

## Design Decisions

### Why Host-Visible Memory?

**Current approach:** `VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT`

**Pros:**
- Simple: Direct CPU-to-GPU transfer
- Fine for small, static meshes (like our triangle)
- No staging buffer needed

**Cons:**
- Slower GPU access than device-local memory
- Not suitable for large or dynamic geometry

**Future improvement:** Use staging buffers for device-local memory transfers:
1. Create host-visible staging buffer
2. Copy data to staging buffer
3. Create device-local vertex buffer
4. Use command buffer to copy staging → vertex buffer
5. Destroy staging buffer

### Why Static Descriptor Methods?

The binding and attribute descriptors are static methods because:
- They describe the vertex **layout**, not instance-specific data
- Pipeline needs them during creation (before Model instances exist)
- All Model instances share the same layout

### Vertex Data Separation

**Triangle vertices moved from shader to CPU:**

**Before:**
```glsl
// simple_shader.vert
vec2 positions[3] = vec2[](
    vec2(0.0, -0.5),
    vec2(0.5, 0.5),
    vec2(-0.5, 0.5)
);
gl_Position = vec4(positions[gl_VertexIndex], 0.0, 1.0);
```

**After:**
```cpp
// FirstApp.cpp
std::vector<Model::Vertex> vertices {
    {{0.0f, -0.5f}},
    {{0.5f, 0.5f}},
    {{-0.5f, 0.5f}}
};
model = std::make_unique<Model>(device, vertices);
```

```glsl
// simple_shader.vert
layout(location = 0) in vec2 position;
gl_Position = vec4(position, 0.0, 1.0);
```

**Why this change?**
- **Flexibility:** Can load geometry from files, not hardcoded in shaders
- **Scalability:** Different models can share the same shader
- **Standard practice:** Most engines use vertex buffers, not shader arrays

---

## Integration with Pipeline

The Pipeline component uses Model's vertex descriptors during creation:

```cpp
// Pipeline.cpp - createGraphicsPipeline()
auto bindingDescriptions = Model::Vertex::getBindingDescriptions();
auto attributeDescriptions = Model::Vertex::getAttributeDescriptions();

VkPipelineVertexInputStateCreateInfo vertexInputInfo{};
vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
vertexInputInfo.vertexBindingDescriptionCount = static_cast<uint32_t>(bindingDescriptions.size());
vertexInputInfo.vertexAttributeDescriptionCount = static_cast<uint32_t>(attributeDescriptions.size());
vertexInputInfo.pVertexAttributeDescriptions = attributeDescriptions.data();
vertexInputInfo.pVertexBindingDescriptions = bindingDescriptions.data();
```

**This tells the pipeline:**
- How to read from vertex buffers (binding descriptions)
- How to interpret vertex data (attribute descriptions)
- What shader inputs to expect

---

## Future Enhancements

### 1. Extended Vertex Attributes

```cpp
struct Vertex {
    glm::vec3 position;    // 3D positions
    glm::vec3 color;       // Per-vertex colors
    glm::vec3 normal;      // Surface normals for lighting
    glm::vec2 texCoord;    // Texture coordinates
};
```

**Requires updating:**
- `getAttributeDescriptions()` to include new attributes
- Vertex shader to accept new inputs
- `format` and `offset` fields in attribute descriptions

### 2. Index Buffers

**Problem:** Meshes have shared vertices (e.g., cube has 8 vertices, but 36 vertex references for 12 triangles)

**Solution:** Add index buffer support
```cpp
std::vector<uint32_t> indices;  // Vertex indices
VkBuffer indexBuffer;
VkDeviceMemory indexBufferMemory;

void drawIndexed(VkCommandBuffer commandBuffer);
```

### 3. Device-Local Memory

For better performance:
```cpp
void createVertexBuffers(const std::vector<Vertex>& vertices) {
    // Create staging buffer (host-visible)
    // Create vertex buffer (device-local)
    // Copy staging → vertex buffer via command buffer
    // Destroy staging buffer
}
```

### 4. Dynamic Vertex Data

For animated or procedural geometry:
- Use `VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT` without staging
- Implement `updateVertexBuffer()` method
- Map/copy/unmap on each frame (or when data changes)

### 5. Multiple Meshes

```cpp
class Model {
    struct Mesh {
        VkBuffer vertexBuffer;
        VkBuffer indexBuffer;
        uint32_t indexCount;
    };
    std::vector<Mesh> meshes;
};
```

---

## Memory Management

### Ownership Model

- **Model owns:** Vertex buffer and associated memory
- **Device provides:** Helper methods for buffer creation
- **FirstApp owns:** Model instance (via `std::unique_ptr`)

### Lifetime

1. **Creation:** Model constructor allocates GPU memory
2. **Usage:** Model persists during rendering loop
3. **Destruction:** Model destructor frees GPU memory (RAII)

**Important:** Model must be destroyed before Device (Device is referenced, not owned).

---

## Common Issues

### "Vertex count must be at least 3"

**Cause:** Trying to create a Model with < 3 vertices.

**Fix:** Ensure vertex vector has at least 3 elements.

### Validation Layer Error: "Vertex buffer not bound"

**Cause:** Called `model->draw()` without calling `model->bind()` first.

**Fix:** Always bind before drawing:
```cpp
model->bind(commandBuffer);
model->draw(commandBuffer);
```

### Black Screen / No Triangle

**Possible causes:**
1. Vertex positions outside clip space (-1 to 1)
2. Pipeline vertex input doesn't match Model descriptors
3. Shader `layout(location)` doesn't match attribute location
4. Forgot to compile shaders after updating vertex shader

---

## Component Dependencies

**Model depends on:**
- **Device** - for buffer creation and memory allocation
- **GLM** - for vec2 type (future: vec3, vec4, matrices)

**Components that depend on Model:**
- **Pipeline** - uses vertex descriptors during creation
- **FirstApp** - creates and manages Model instances

---

## Related Documentation

- **[Device Component](DEVICE.md)** - Buffer creation and memory management
- **[Pipeline Component](PIPELINE.md)** - How vertex input state is configured
- **[Architecture Overview](ARCHITECTURE.md)** - How Model fits into the rendering loop

---

**Next Component:** [Shader Documentation](SHADERS.md) _(when created)_
