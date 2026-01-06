# Model Component

The Model component manages vertex data and vertex buffers for rendering geometry in the Bismuth Engine. It provides an abstraction layer for loading vertex data into GPU memory and binding it to the graphics pipeline.

## Overview

**Purpose:** Encapsulate vertex data management, including buffer creation, memory allocation, OBJ file loading, and rendering commands.

**Key Responsibilities:**
- Load 3D models from OBJ files with automatic vertex deduplication
- Store vertex data in GPU-accessible memory
- Define vertex input layout (binding and attribute descriptions)
- Bind vertex buffers to command buffers
- Issue draw commands

**Location:** `engine/src/Model.hpp`, `engine/src/Model.cpp`

**Dependencies:** Device, GLM, tinyobjloader, Utils (for hash combining)

---

## Architecture

### Class Structure

```cpp
class Model {
public:
    struct Vertex {
        glm::vec3 position{};
        glm::vec3 color{};
        glm::vec3 normal{};
        glm::vec2 uv{};
        
        static std::vector<VkVertexInputBindingDescription> getBindingDescriptions();
        static std::vector<VkVertexInputAttributeDescription> getAttributeDescriptions();
        
        bool operator==(const Vertex& other) const;
    };

    struct Data {
        std::vector<Vertex> vertices{};
        std::vector<uint32_t> indices{};
        
        void loadModel(const std::string& filePath);
    };

    Model(Device& device, const Data& data);
    ~Model();
    
    static std::unique_ptr<Model> createModelFromFile(Device& device, const std::string& filePath);
    
    void bind(VkCommandBuffer commandBuffer);
    void draw(VkCommandBuffer commandBuffer);

private:
    void createVertexBuffers(const std::vector<Vertex>& vertices);
    void createIndexBuffer(const std::vector<uint32_t>& indices);
    
    Device& device;
    
    VkBuffer vertexBuffer;
    VkDeviceMemory vertexBufferMemory;
    uint32_t vertexCount;
    
    bool hasIndexBuffer = false;
    VkBuffer indexBuffer;
    VkDeviceMemory indexBufferMemory;
    uint32_t indexCount;
};
```

### Vertex Structure

The `Vertex` struct defines the layout of vertex data:

```cpp
struct Vertex {
    glm::vec3 position{};  // 3D position in model space
    glm::vec3 color{};     // RGB color per vertex
    glm::vec3 normal{};    // Surface normal for lighting calculations
    glm::vec2 uv{};        // Texture coordinates
    
    bool operator==(const Vertex& other) const {
        return position == other.position && 
               color == other.color && 
               normal == other.normal && 
               uv == other.uv;
    }
};
```

**Vertex attributes:**
- `position` (vec3): 3D coordinates in model space
- `color` (vec3): RGB color values (0.0 to 1.0 per channel)
- `normal` (vec3): Surface normal vector for lighting calculations
- `uv` (vec2): Texture coordinates for UV mapping (reserved for future texturing)

**Equality operator:** Required for using `Vertex` as a key in `std::unordered_map` for vertex deduplication during model loading.

**Default initialization:** All fields are value-initialized to zero using `{}` syntax.

**Lighting support:** Normal attributes are now actively used in the vertex shader for Gouraud shading (per-vertex lighting). UV attributes are loaded but reserved for future texturing systems.

### Data Structure

The `Data` struct encapsulates all geometry data for a model:

```cpp
struct Data {
    std::vector<Vertex> vertices{};  // Vertex data with positions, colors, normals, UVs
    std::vector<uint32_t> indices{}; // Optional index buffer for shared vertices
    
    void loadModel(const std::string& filePath);
};
```

**Purpose:** Groups vertex and index data together for cleaner API and easier model construction.

**Index buffer usage:**
- **With indices:** Efficient representation of meshes with shared vertices (e.g., cube has 24 vertices instead of 36)
- **Without indices:** Simple vertex-only rendering (empty indices vector)

**loadModel() method:** Populates vertices and indices from OBJ files using tinyobjloader, automatically deduplicating vertices.

---

## Initialization

### Constructor

```cpp
Model::Model(Device &device, const Data &data) : device{device} {
    createVertexBuffers(data.vertices);
    createIndexBuffer(data.indices);
}
```

**Parameters:**
- `device`: Reference to the Device component (needed for buffer creation)
- `data`: Model data containing vertices and optional indices

**What it does:**
1. Stores reference to Device
2. Calls `createVertexBuffers()` to allocate vertex buffer in GPU memory
3. Calls `createIndexBuffer()` to optionally create index buffer (if indices provided)

### Destructor

```cpp
Model::~Model() {
    vkDestroyBuffer(device.device(), vertexBuffer, nullptr);
    vkFreeMemory(device.device(), vertexBufferMemory, nullptr);
    if (hasIndexBuffer) {
        vkDestroyBuffer(device.device(), indexBuffer, nullptr);
        vkFreeMemory(device.device(), indexBufferMemory, nullptr);
    }
}
```

**Cleanup:**
- Destroys the vertex buffer object
- Frees the associated vertex buffer GPU memory
- If index buffer exists, destroys it and frees its GPU memory

---

## Buffer Creation

### createVertexBuffers()

```cpp
void Model::createVertexBuffers(const std::vector<Vertex> &vertices) {
    vertexCount = static_cast<uint32_t>(vertices.size());
    assert(vertexCount >= 3 && "Vertex count must be at least 3.");

    VkDeviceSize bufferSize = sizeof(vertices[0]) * vertexCount;

    VkBuffer stagingBuffer;
    VkDeviceMemory stagingBufferMemory;

    device.createBuffer(
        bufferSize,
        VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
        stagingBuffer,
        stagingBufferMemory);

    void *data;
    vkMapMemory(device.device(), stagingBufferMemory, 0, bufferSize, 0, &data);
    memcpy(data, vertices.data(), static_cast<size_t>(bufferSize));
    vkUnmapMemory(device.device(), stagingBufferMemory);

    device.createBuffer(
        bufferSize,
        VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
        vertexBuffer,
        vertexBufferMemory);

    device.copyBuffer(stagingBuffer, vertexBuffer, bufferSize);

    vkDestroyBuffer(device.device(), stagingBuffer, nullptr);
    vkFreeMemory(device.device(), stagingBufferMemory, nullptr);
}
```

**Steps:**

1. **Validation:** Ensures at least 3 vertices (minimum for a triangle)
2. **Calculate size:** `sizeof(Vertex) * vertexCount`
3. **Create staging buffer:** Host-visible temporary buffer for CPU access
4. **Map memory:** Makes staging buffer accessible from CPU
5. **Copy data:** Transfers vertex data from CPU to staging buffer
6. **Unmap memory:** Releases CPU access to staging buffer
7. **Create vertex buffer:** Device-local buffer for optimal GPU performance
8. **Copy staging → vertex:** Uses GPU command to transfer data
9. **Cleanup:** Destroys staging buffer and frees its memory

**Why staging buffers?**

**Previous approach (host-visible memory):**
- Simple: Direct CPU-to-GPU transfer
- Slow: Host-visible memory has lower GPU access performance

**Current approach (staging buffer + device-local memory):**
- **Optimal performance:** Device-local memory provides fastest GPU access
- **Two-step transfer:** CPU → staging buffer → device-local buffer
- **Best practice:** Standard approach for static geometry in production engines

**Memory properties:**
- **Staging buffer:**
  - `VK_BUFFER_USAGE_TRANSFER_SRC_BIT`: Can be used as transfer source
  - `VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT`: CPU can access
  - `VK_MEMORY_PROPERTY_HOST_COHERENT_BIT`: No manual cache flush needed
- **Vertex buffer:**
  - `VK_BUFFER_USAGE_VERTEX_BUFFER_BIT`: Used as vertex buffer
  - `VK_BUFFER_USAGE_TRANSFER_DST_BIT`: Can receive transferred data
  - `VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT`: Fastest GPU access

### createIndexBuffer()

```cpp
void Model::createIndexBuffer(const std::vector<uint32_t> &indices) {
    indexCount = static_cast<uint32_t>(indices.size());
    hasIndexBuffer = indexCount > 0;

    if (!hasIndexBuffer) return;

    VkDeviceSize bufferSize = sizeof(indices[0]) * indexCount;

    VkBuffer stagingBuffer;
    VkDeviceMemory stagingBufferMemory;

    device.createBuffer(
        bufferSize,
        VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
        stagingBuffer,
        stagingBufferMemory);

    void *data;
    vkMapMemory(device.device(), stagingBufferMemory, 0, bufferSize, 0, &data);
    memcpy(data, indices.data(), static_cast<size_t>(bufferSize));
    vkUnmapMemory(device.device(), stagingBufferMemory);

    device.createBuffer(
        bufferSize,
        VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
        indexBuffer,
        indexBufferMemory);

    device.copyBuffer(stagingBuffer, indexBuffer, bufferSize);

    vkDestroyBuffer(device.device(), stagingBuffer, nullptr);
    vkFreeMemory(device.device(), stagingBufferMemory, nullptr);
}
```

**Purpose:** Creates an optional index buffer for indexed rendering.

**Steps:**

1. **Count indices:** Store index count and set `hasIndexBuffer` flag
2. **Early return:** If no indices provided, skip index buffer creation
3. **Calculate size:** `sizeof(uint32_t) * indexCount`
4. **Create staging buffer:** Host-visible temporary buffer
5. **Map and copy:** Transfer index data from CPU to staging buffer
6. **Create index buffer:** Device-local buffer for optimal GPU performance
7. **Copy staging → index:** GPU-based transfer for efficiency
8. **Cleanup:** Destroy staging buffer

**Why index buffers?**

**Without indices (before):**
```cpp
// Cube requires 36 vertices (6 faces × 2 triangles × 3 vertices)
std::vector<Vertex> vertices(36);  // Many duplicate vertices
```

**With indices (after):**
```cpp
// Cube requires only 24 unique vertices (4 per face × 6 faces)
std::vector<Vertex> vertices(24);    // Unique vertices only
std::vector<uint32_t> indices(36);   // References to vertices
```

**Benefits:**
- **Memory savings:** 24 vertices vs 36 vertices (33% reduction for cubes)
- **Cache efficiency:** GPU can reuse recently processed vertices
- **Standard practice:** All modern 3D models use indexed geometry
- **Flexibility:** Can represent complex meshes efficiently

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
    std::vector<VkVertexInputAttributeDescription> attributeDescriptions{};
    
    attributeDescriptions.push_back({0, 0, VK_FORMAT_R32G32B32_SFLOAT, offsetof(Vertex, position)});
    attributeDescriptions.push_back({1, 0, VK_FORMAT_R32G32B32_SFLOAT, offsetof(Vertex, color)});
    attributeDescriptions.push_back({2, 0, VK_FORMAT_R32G32B32_SFLOAT, offsetof(Vertex, normal)});
    attributeDescriptions.push_back({3, 0, VK_FORMAT_R32G32_SFLOAT, offsetof(Vertex, uv)});
    
    return attributeDescriptions;
}
```

**Aggregate Initialization:** Uses C++ aggregate initialization with designated initializers for concise attribute setup.

**Structure:** `{location, binding, format, offset}`

**Attribute Breakdown:**

| Location | Binding | Format | Offset | Purpose |
|----------|---------|--------|--------|---------|
| 0 | 0 | `VK_FORMAT_R32G32B32_SFLOAT` | `offsetof(Vertex, position)` | Position (vec3) |
| 1 | 0 | `VK_FORMAT_R32G32B32_SFLOAT` | `offsetof(Vertex, color)` | Color (vec3) |
| 2 | 0 | `VK_FORMAT_R32G32B32_SFLOAT` | `offsetof(Vertex, normal)` | Normal (vec3) for lighting |
| 3 | 0 | `VK_FORMAT_R32G32_SFLOAT` | `offsetof(Vertex, uv)` | UV (vec2) - reserved for texturing |

**Purpose:** Maps struct fields to vertex shader inputs with matching location indices.

**Note:** The normal attribute is now actively used in the vertex shader for Gouraud shading (per-vertex lighting). UV coordinates remain reserved for future texturing.

**Why `offsetof()`?**
Using `offsetof()` instead of hardcoded byte offsets ensures correctness even if:
- Compiler adds padding between struct fields
- Struct field order changes
- New fields are added

**Shader correspondence:**
```glsl
layout(location = 0) in vec2 position;  // Matches location = 0, format = R32G32_SFLOAT
layout(location = 1) in vec3 color;     // Matches location = 1, format = R32G32B32_SFLOAT
```

---

## Rendering Commands

### bind()

```cpp
void Model::bind(VkCommandBuffer commandBuffer) {
    VkBuffer buffers[] = {vertexBuffer};
    VkDeviceSize offsets[] = {0};
    vkCmdBindVertexBuffers(commandBuffer, 0, 1, buffers, offsets);

    if (hasIndexBuffer) {
        vkCmdBindIndexBuffer(commandBuffer, indexBuffer, 0, VK_INDEX_TYPE_UINT32);
    }
}
```

**What it does:**
- Binds the vertex buffer to binding point 0
- Sets offset to 0 (start at beginning of buffer)
- If model has index buffer, binds it with `VK_INDEX_TYPE_UINT32`

**Must be called:** After binding the pipeline, before drawing.

**Index type:** Currently uses `VK_INDEX_TYPE_UINT32` for all indexed models. Future optimization could use `VK_INDEX_TYPE_UINT16` for models with fewer than 65,536 vertices.

### draw()

```cpp
void Model::draw(VkCommandBuffer commandBuffer) {
    if (hasIndexBuffer) {
        vkCmdDrawIndexed(commandBuffer, indexCount, 1, 0, 0, 0);
    } else {
        vkCmdDraw(commandBuffer, vertexCount, 1, 0, 0);
    }
}
```

**Behavior depends on index buffer presence:**

**Indexed drawing (`vkCmdDrawIndexed`):**
- `indexCount`: Number of indices to draw
- `instanceCount = 1`: Not using instancing
- `firstIndex = 0`: Start at index 0
- `vertexOffset = 0`: Base vertex offset
- `firstInstance = 0`: Start at instance 0

**Non-indexed drawing (`vkCmdDraw`):**
- `vertexCount`: Number of vertices to draw
- `instanceCount = 1`: Not using instancing
- `firstVertex = 0`: Start at vertex 0
- `firstInstance = 0`: Start at instance 0

**Result:** Issues appropriate draw call based on whether model uses indices.

---

## Model Loading from OBJ Files

### createModelFromFile()

```cpp
std::unique_ptr<Model> Model::createModelFromFile(Device &device, const std::string &filePath) {
    Data data{};
    data.loadModel(filePath);
    return std::make_unique<Model>(device, data);
}
```

**Purpose:** Factory method for loading models from OBJ files.

**Parameters:**
- `device`: Reference to Device component
- `filePath`: Path to .obj file (use `MODELS_DIR` macro for portability)

**Returns:** `std::unique_ptr<Model>` ready for rendering

**Usage:**
```cpp
std::shared_ptr<Model> model = Model::createModelFromFile(
    device, std::string(MODELS_DIR) + "smooth_vase.obj");
```

**Why factory method?** Separates file loading logic from Model construction, providing a cleaner API.

### Data::loadModel()

```cpp
void Model::Data::loadModel(const std::string &filePath) {
    tinyobj::attrib_t attrib;
    std::vector<tinyobj::shape_t> shapes;
    std::vector<tinyobj::material_t> materials;
    std::string warn, err;

    if (!tinyobj::LoadObj(&attrib, &shapes, &materials, &warn, &err, filePath.c_str())) {
        throw std::runtime_error(warn + err);
    }

    vertices.clear();
    indices.clear();

    std::unordered_map<Vertex, uint32_t> uniqueVertices{};
    for (const auto &shape: shapes) {
        for (const auto &index: shape.mesh.indices) {
            Vertex vertex{};

            if (index.vertex_index >= 0) {
                vertex.position = {
                    attrib.vertices[3 * index.vertex_index + 0],
                    attrib.vertices[3 * index.vertex_index + 1],
                    attrib.vertices[3 * index.vertex_index + 2]
                };

                auto colorIndex = 3 * index.vertex_index + 2;
                if (colorIndex < attrib.colors.size()) {
                    vertex.color = {
                        attrib.colors[colorIndex - 2],
                        attrib.colors[colorIndex - 1],
                        attrib.colors[colorIndex - 0]
                    };
                } else {
                    vertex.color = {1.0f, 1.0f, 1.0f};  // Default white
                };
            }

            if (index.normal_index >= 0) {
                vertex.normal = {
                    attrib.normals[3 * index.normal_index + 0],
                    attrib.normals[3 * index.normal_index + 1],
                    attrib.normals[3 * index.normal_index + 2]
                };
            }

            if (index.texcoord_index >= 0) {
                vertex.uv = {
                    attrib.texcoords[2 * index.texcoord_index + 0],
                    attrib.texcoords[2 * index.texcoord_index + 1]
                };
            }

            if (uniqueVertices.count(vertex) == 0) {
                uniqueVertices[vertex] = static_cast<uint32_t>(vertices.size());
                vertices.push_back(vertex);
            }
            indices.push_back(uniqueVertices[vertex]);
        }
    }
}
```

**Purpose:** Load geometry from OBJ file with automatic vertex deduplication.

**Process:**

1. **Parse OBJ file:** Uses tinyobjloader to parse the file into attributes, shapes, and materials
2. **Error handling:** Throws `std::runtime_error` if parsing fails
3. **Clear existing data:** Ensures clean state for loading
4. **Create deduplication map:** `std::unordered_map<Vertex, uint32_t>` tracks unique vertices
5. **Iterate shapes:** Process each shape in the OBJ file (models can have multiple meshes)
6. **Iterate indices:** Process each triangle vertex index
7. **Load position:** Extract XYZ coordinates from `attrib.vertices`
8. **Load color (optional):** Extract RGB if available, default to white (1,1,1)
9. **Load normal (optional):** Extract normal vector if present
10. **Load UV (optional):** Extract texture coordinates if present
11. **Deduplicate:** Check if vertex already exists in map
12. **Add unique vertex:** If new, add to vertices vector and map
13. **Build index buffer:** Always add index to indices vector

**Vertex Deduplication Algorithm:**

```cpp
if (uniqueVertices.count(vertex) == 0) {
    uniqueVertices[vertex] = static_cast<uint32_t>(vertices.size());
    vertices.push_back(vertex);
}
indices.push_back(uniqueVertices[vertex]);
```

**How it works:**
- Uses `std::unordered_map` with `Vertex` as key (requires `std::hash<Vertex>` specialization)
- When encountering a vertex, check if it's already in the map
- If new: assign it the next index, add to vertices, store in map
- If duplicate: reuse existing index
- Always add the index (new or existing) to the indices array

**Benefits:**
- **Memory reduction:** Typical savings of 40-60% for models with shared vertices
- **Performance:** O(1) average lookup time for deduplication
- **GPU cache efficiency:** Smaller vertex buffers improve cache hit rates

**Hash Function Requirement:**

The deduplication relies on a custom hash function defined in `Model.cpp`:

```cpp
namespace std {
    template <>
    struct hash<engine::Model::Vertex> {
        size_t operator()(engine::Model::Vertex const &vertex) const {
            size_t seed = 0;
            engine::hashCombine(seed, vertex.position, vertex.color, vertex.normal, vertex.uv);
            return seed;
        }
    };
}
```

**Requirements:**
- `Vertex::operator==()` for equality comparison
- `std::hash<Vertex>` specialization for hashing
- `#include <glm/gtx/hash.hpp>` with `GLM_ENABLE_EXPERIMENTAL` for GLM type hashing

**Supported OBJ Features:**
- ✅ Vertex positions (`v x y z`)
- ✅ Vertex normals (`vn x y z`)
- ✅ Texture coordinates (`vt u v`)
- ✅ Vertex colors (optional, defaults to white)
- ✅ Multiple shapes/meshes per file
- ✅ Triangulated faces (`f v1/vt1/vn1 v2/vt2/vn2 v3/vt3/vn3`)
- ❌ Materials (loaded but not used)
- ❌ Quads (must be triangulated before loading)

**Example OBJ File:**
```obj
# Vertex positions
v 0.0 0.0 0.0
v 1.0 0.0 0.0
v 0.5 1.0 0.0

# Texture coordinates
vt 0.0 0.0
vt 1.0 0.0
vt 0.5 1.0

# Normals
vn 0.0 0.0 1.0
vn 0.0 0.0 1.0
vn 0.0 0.0 1.0

# Face (position/uv/normal)
f 1/1/1 2/2/2 3/3/3
```

**Path Handling:**

Use the `MODELS_DIR` macro for portable paths:
```cpp
// Correct - works in all IDEs and build configurations
std::string(MODELS_DIR) + "mymodel.obj"

// Incorrect - depends on working directory
"engine/models/mymodel.obj"  // Breaks in some IDEs
```

**The `MODELS_DIR` macro is defined in CMake:**
```cmake
set(MODELS_DIR "${CMAKE_SOURCE_DIR}/engine/models/")
target_compile_definitions(bismuth_engine PRIVATE MODELS_DIR="${MODELS_DIR}")
```

---

## Usage Example

### Loading Models from OBJ Files

```cpp
void FirstApp::loadGameObjects() {
    // Load a smooth vase model
    std::shared_ptr<Model> model = Model::createModelFromFile(
        device, std::string(MODELS_DIR) + "smooth_vase.obj");

    auto gameObject = GameObject::createGameObject();
    gameObject.model = model;
    gameObject.transform.translation = {0.0f, 0.0f, 2.5f};
    gameObject.transform.scale = glm::vec3(3.0f);

    // Load a skull model
    std::shared_ptr<Model> model2 = Model::createModelFromFile(
        device, std::string(MODELS_DIR) + "skull.obj");

    auto gameObject1 = GameObject::createGameObject();
    gameObject1.model = model2;
    gameObject1.transform.translation = {2.0f, 0.0f, 2.5f};
    gameObject1.transform.rotation = {0.0f, 0.0f, glm::radians(90.0f)};
    gameObject1.transform.scale = glm::vec3(0.0175f);

    gameObjects.push_back(std::move(gameObject));
    gameObjects.push_back(std::move(gameObject1));
}
```

**Key points:**
- Use `MODELS_DIR` macro for portable file paths
- Models are loaded once and can be shared between GameObjects
- Adjust scale to fit model size (some models use different units)
- Transformations applied per GameObject instance

### Loading a Cube with Index Buffer (Manual)

```cpp
void FirstApp::loadGameObjects() {
    Model::Data modelData{};
    modelData.vertices = {
        // left face (white)
        {{-.5f, -.5f, -.5f}, {.9f, .9f, .9f}},
        {{-.5f, .5f, .5f}, {.9f, .9f, .9f}},
        {{-.5f, -.5f, .5f}, {.9f, .9f, .9f}},
        {{-.5f, .5f, -.5f}, {.9f, .9f, .9f}},
        
        // right face (yellow)
        {{.5f, -.5f, -.5f}, {.8f, .8f, .1f}},
        {{.5f, .5f, .5f}, {.8f, .8f, .1f}},
        {{.5f, -.5f, .5f}, {.8f, .8f, .1f}},
        {{.5f, .5f, -.5f}, {.8f, .8f, .1f}},
        
        // Additional faces...
    };
    
    // Apply offset to all vertices
    for (auto &v: modelData.vertices) {
        v.position += offset;
    }
    
    modelData.indices = {
        0, 1, 2, 0, 3, 1,      // left face (2 triangles)
        4, 5, 6, 4, 7, 5,      // right face
        8, 9, 10, 8, 11, 9,    // top face
        12, 13, 14, 12, 15, 13, // bottom face
        16, 17, 18, 16, 19, 17, // front face
        20, 21, 22, 20, 23, 21  // back face
    };

    model = std::make_unique<Model>(device, modelData);
}
```

**Key changes from non-indexed approach:**
- **24 unique vertices** instead of 36 duplicated vertices
- **36 indices** reference the unique vertices to form 12 triangles
- **Memory efficient:** Each vertex stored once, referenced multiple times
- **Cache friendly:** GPU can reuse transformed vertices

**Color Interpolation:**
The GPU automatically interpolates colors across the triangle surface. Each face has a single color at all vertices, creating solid-colored faces. The interpolation system still works, but produces uniform colors per face.

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

### Why Device-Local Memory with Staging Buffers?

**Current approach:** Staging buffer + device-local memory

**Process:**
1. Create host-visible staging buffer
2. Copy data from CPU to staging buffer
3. Create device-local vertex/index buffer
4. Use GPU command buffer to copy staging → final buffer
5. Destroy staging buffer

**Pros:**
- **Optimal GPU performance:** Device-local memory is fastest for GPU reads
- **Industry standard:** Used by all production game engines
- **Scales well:** Works for meshes of any size

**Cons:**
- More complex than direct host-visible approach
- Requires command buffer submission for copy operation
- Temporary staging buffer overhead (cleaned up immediately)

**Previous approach (host-visible memory):**
- Simple: Direct CPU-to-GPU transfer
- Slower: Host-visible memory has lower GPU access performance
- Acceptable only for very small, static meshes

### Why Static Descriptor Methods?

The binding and attribute descriptors are static methods because:
- They describe the vertex **layout**, not instance-specific data
- Pipeline needs them during creation (before Model instances exist)
- All Model instances share the same layout

### Vertex Data Separation

**Triangle vertices moved from shader to CPU:**

**Before (Hardcoded in Shader):**
```glsl
// simple_shader.vert
vec2 positions[3] = vec2[](
    vec2(0.0, -0.5),
    vec2(0.5, 0.5),
    vec2(-0.5, 0.5)
);
gl_Position = vec4(positions[gl_VertexIndex], 0.0, 1.0);
```

```glsl
// simple_shader.frag
void main() {
    outColor = vec4(1.0, 1.0, 0.0, 1.0);  // Solid yellow
}
```

**After (Vertex Buffers with Color Attributes):**
```cpp
// FirstApp.cpp
std::vector<Model::Vertex> vertices {
    {{0.0f, -0.5f}, {1.0f, 0.0f, 0.0f}},  // Position + Red
    {{0.5f, 0.5f}, {0.0f, 1.0f, 0.0f}},   // Position + Green
    {{-0.5f, 0.5f}, {0.0f, 0.0f, 1.0f}}   // Position + Blue
};
model = std::make_unique<Model>(device, vertices);
```

```glsl
// simple_shader.vert
layout(location = 0) in vec2 position;
layout(location = 1) in vec3 color;
layout(location = 0) out vec3 fragColor;

void main() {
    gl_Position = vec4(position, 0.0, 1.0);
    fragColor = color;  // Pass to fragment shader
}
```

```glsl
// simple_shader.frag
layout(location = 0) in vec3 fragColor;
layout(location = 0) out vec4 outColor;

void main() {
    outColor = vec4(fragColor, 1.0);  // Interpolated color
}
```

**Why these changes?**
- **Flexibility:** Can load geometry and colors from files, not hardcoded in shaders
- **Scalability:** Different models can share the same shader
- **Standard practice:** Most engines use vertex buffers with attributes
- **Visual richness:** Per-vertex colors enable smooth gradients via automatic interpolation

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

Currently implemented:
```cpp
struct Vertex {
    glm::vec3 position;    // ✅ 3D positions
    glm::vec3 color;       // ✅ Per-vertex colors (with interpolation)
};
```

Future extensions:
```cpp
struct Vertex {
    glm::vec3 position;    // ✅ Already implemented
    glm::vec3 color;       // ✅ Already implemented
    glm::vec3 normal;      // Surface normals for lighting
    glm::vec2 texCoord;    // Texture coordinates
    glm::vec3 tangent;     // For normal mapping
};
```

**When adding new attributes:**
- Update `getAttributeDescriptions()` with new location, format, and offset
- Add corresponding `layout(location = N) in` declarations in vertex shader
- Use `offsetof(Vertex, fieldName)` for correct byte offsets
- Consider struct alignment and padding

### 2. Index Buffer Optimizations

**Current implementation:** Uses `VK_INDEX_TYPE_UINT32` for all models

**Future optimization:** Dynamic index type selection
```cpp
// Choose index type based on vertex count
VkIndexType indexType = vertexCount > 65535 ? VK_INDEX_TYPE_UINT32 : VK_INDEX_TYPE_UINT16;
```

**Benefits:**
- **Memory savings:** 16-bit indices use half the memory for small models
- **Cache efficiency:** More indices fit in GPU cache
- **Requires refactoring:** Model class would need to support both uint16_t and uint32_t index vectors

### 3. Vertex Buffer Updates

For animated or procedural geometry:
```cpp
void updateVertexBuffer(const std::vector<Vertex>& vertices) {
    // Map device memory
    // Copy new vertex data
    // Unmap memory
}
```

**Note:** Would require switching back to host-visible memory, or implementing a double-buffered approach with staging buffers per frame.

### 4. Model Loading from Files

Future file format support:
```cpp
class Model {
    static std::unique_ptr<Model> createFromFile(Device& device, const std::string& filepath);
};
```

**Supported formats:**
- OBJ: Simple text-based format
- GLTF: Modern, feature-rich format with PBR support
- Binary formats for faster loading

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
5. Mismatch between number of attributes in `getAttributeDescriptions()` and shader inputs

### Color Not Showing / Wrong Colors

**Possible causes:**
1. Color values outside valid range (should be 0.0 to 1.0)
2. Fragment shader not using the interpolated color input
3. Forgot to pass color from vertex shader to fragment shader (`out`/`in` variables)
4. Swapchain format doesn't support color correctly (should be SRGB format)

---

## Component Dependencies

**Model depends on:**
- **Device** - for buffer creation and memory allocation
- **GLM** - for vec3 type (position and color vectors)

**Components that depend on Model:**
- **Pipeline** - uses vertex descriptors during creation
- **FirstApp** - creates and manages Model instances

---

## Related Documentation

- **[Device Component](DEVICE.md)** - Buffer creation and memory management
- **[Utils Component](UTILS.md)** - Hash combining utility for vertex deduplication
- **[Pipeline Component](PIPELINE.md)** - How vertex input state is configured with vertex attributes
- **[SwapChain Component](SWAPCHAIN.md)** - SRGB format for accurate color display
- **[Architecture Overview](ARCHITECTURE.md)** - How Model fits into the rendering loop and OBJ loading system
- **[Configuration](CONFIGURATION.md)** - MODELS_DIR macro and tinyobjloader dependency

---

**Next Component:** [Shader Documentation](SHADERS.md) _(when created)_
