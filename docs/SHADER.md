# Shader Documentation

Complete technical documentation for the shader system.

**Files:** `engine/shaders/src/simple_shader.vert`, `engine/shaders/src/simple_shader.frag`  
**Purpose:** Vertex transformation and fragment coloring with push constants  
**Language:** GLSL 4.6  
**Compilation:** glslc → SPIR-V bytecode

---

## Table of Contents

- [Overview](#overview)
- [Vertex Shader](#vertex-shader)
- [Fragment Shader](#fragment-shader)
- [Push Constants](#push-constants)
- [Shader Communication](#shader-communication)
- [Compilation](#compilation)
- [Common Issues](#common-issues)

---

## Overview

### What Are Shaders?

Shaders are programs that run on the GPU to transform vertices and compute pixel colors:

```
CPU → Vertex Data → [Vertex Shader] → Transformed Positions
                                              ↓
                              [Rasterizer] → Fragments
                                              ↓
                          [Fragment Shader] → Final Colors → Screen
```

**Vertex Shader:**
- Runs once per vertex
- Transforms positions (model → world → clip space)
- Outputs position for rasterization

**Fragment Shader:**
- Runs once per pixel
- Computes final color
- Outputs RGBA color value

---

## Vertex Shader

**File:** `engine/shaders/src/simple_shader.vert`

### Complete Source

```glsl
#version 460

layout(location = 0) in vec3 position;
layout(location = 1) in vec3 color;

layout(location = 0) out vec3 fragColor;

layout(push_constant) uniform Push {
  mat4 transform;
  vec3 color;
} push;

// Executed once per vertex we have
void main () {
  //gl_Position is the output position in clip coordinates (x: -1 (left) - (right) 1, y: -1 (up) - (down) 1)
  gl_Position = push.transform * vec4(position, 1.0);
  fragColor = color;
}
```

### Input Attributes

```glsl
layout(location = 0) in vec3 position;
layout(location = 1) in vec3 color;
```

| Location | Type | Source | Purpose |
|----------|------|--------|---------|
| 0 | `vec3` | Vertex buffer | 3D position (x, y, z) |
| 1 | `vec3` | Vertex buffer | RGB color (per-vertex) |

**Matching CPU Side:**

```cpp
// In Model.hpp
static std::vector<VkVertexInputAttributeDescription> Vertex::getAttributeDescriptions() {
    std::vector<VkVertexInputAttributeDescription> attributeDescriptions(2);
    
    attributeDescriptions[0].binding = 0;
    attributeDescriptions[0].location = 0;  // Matches shader location 0
    attributeDescriptions[0].format = VK_FORMAT_R32G32B32_SFLOAT;  // vec3
    attributeDescriptions[0].offset = offsetof(Vertex, position);
    
    attributeDescriptions[1].binding = 0;
    attributeDescriptions[1].location = 1;  // Matches shader location 1
    attributeDescriptions[1].format = VK_FORMAT_R32G32B32_SFLOAT;  // vec3
    attributeDescriptions[1].offset = offsetof(Vertex, color);
    
    return attributeDescriptions;
}
```

**Usage:** The `color` input attribute provides per-vertex colors, enabling smooth color gradients across faces.

### Push Constants

```glsl
layout(push_constant) uniform Push {
  mat4 transform;
  vec3 color;
} push;
```

**Purpose:** Fast, per-draw-call data from CPU to GPU.

**Access:** `push.transform`, `push.color`

**Note:** The `color` field in push constants is currently unused in the vertex shader - per-vertex colors from the vertex buffer are passed through instead.

**CPU Side Declaration:**

```cpp
struct SimplePushConstantData {
  glm::mat4 transform{1.f};     // Identity matrix by default
  alignas(16) glm::vec3 color;  // Alignment requirement (unused in current implementation)
};
```

**Member Breakdown:**

| Field | Type | Size | Purpose |
|-------|------|------|---------|
| `transform` | `mat4` | 64 bytes | 4×4 transformation matrix (scale, rotation, translation) |
| `color` | `vec3` | 12 bytes (aligned to 16) | RGB color (unused - vertex colors used instead) |

**Why `alignas(16)` for vec3?**
- GLSL std140 layout rules require vec3 to be aligned to 16 bytes
- Without alignment: GPU reads wrong memory location
- Causes visual glitches or validation errors

**Total Size:** 80 bytes (well within 128-byte minimum limit)

See [Push Constants](#push-constants) section for complete details.

### Output

```glsl
gl_Position = push.transform * vec4(position, 1.0);
fragColor = color;
```

**`gl_Position`:** Built-in output variable (vec4) - final vertex position in clip space

**`fragColor`:** User-defined output variable (vec3) - passed to fragment shader

**Clip Space Coordinates:**
- x: -1.0 (left) to +1.0 (right)
- y: -1.0 (top) to +1.0 (bottom)
- z: 0.0 (near) to 1.0 (far) - Vulkan depth range with `GLM_FORCE_DEPTH_ZERO_TO_ONE`
- w: 1.0 (no perspective division, orthographic projection)

**Transformation Breakdown:**

```glsl
push.transform * vec4(position, 1.0)
^^^^^^^^^^^^^^   ^^^^^^^^^^^^^^^^^^
4×4 Matrix       Vertex promoted to vec4
```

The 4×4 transformation matrix includes:
1. **Scale** (upper-left 3×3, diagonal components)
2. **Rotation** (upper-left 3×3, off-diagonal components)
3. **Translation** (bottom row: `{tx, ty, tz, 1}`)

**Matrix Multiplication:**

```glsl
mat4 T = push.transform;
vec4 p = vec4(position, 1.0);

gl_Position.x = T[0][0]*p.x + T[1][0]*p.y + T[2][0]*p.z + T[3][0]*p.w
gl_Position.y = T[0][1]*p.x + T[1][1]*p.y + T[2][1]*p.z + T[3][1]*p.w
gl_Position.z = T[0][2]*p.x + T[1][2]*p.y + T[2][2]*p.z + T[3][2]*p.w
gl_Position.w = T[0][3]*p.x + T[1][3]*p.y + T[2][3]*p.z + T[3][3]*p.w
```

Since `p.w = 1.0`, the translation values `T[3][0]`, `T[3][1]`, `T[3][2]` are added directly.

**Example:**

```glsl
// Cube vertex at one corner
position = vec3(0.5, 0.5, 0.5)

// Transform matrix (from GameObject)
// Includes: scale(0.5), rotation.y(0.1 rad), translation(0, 0, 0.5)
push.transform = mat4(...computed values...)

// Apply transformation
gl_Position = push.transform * vec4(0.5, 0.5, 0.5, 1.0)
            = vec4(x', y', z', 1.0)  // Transformed position

// Result: vertex is scaled, rotated, translated to final position in clip space
```

**Why This Approach?**
- Single matrix multiplication applies all transformations
- Efficient: GPU hardware optimized for matrix ops
- Standard 3D graphics transformation pipeline
- Translation included in matrix (homogeneous coordinates)

### Key Features

1. **3D Transformations:** `push.transform` applies scale, rotation, and translation in one 4×4 matrix
2. **Per-Vertex Colors:** Color attribute from vertex buffer passed to fragment shader
3. **3D Rendering:** Full XYZ positioning with depth testing
4. **Per-Vertex Execution:** Runs independently for each vertex
5. **Efficient Animation:** Change transform matrix without modifying vertex buffers
6. **Clip Space Rendering:** Without projection matrix, coordinates map directly to clip space [0, 1]

---

## Fragment Shader

**File:** `engine/shaders/src/simple_shader.frag`

### Complete Source

```glsl
#version 460

layout(location = 0) in vec3 fragColor;
// Variable stores and RGBA output color that should be written to color attachment 0
layout(location = 0) out vec4 outColor;

layout(push_constant) uniform Push {
  mat4 transform;
  vec3 color;
} push;

void main() {
  outColor = vec4(fragColor, 1.0);
}
```

### Input from Vertex Shader

```glsl
layout(location = 0) in vec3 fragColor;
```

**Purpose:** Receives interpolated color from vertex shader.

**How Interpolation Works:**
- Vertex shader outputs per-vertex colors
- Rasterizer interpolates colors across the triangle face
- Fragment shader receives interpolated color for each pixel

**Example:**
```
Vertex 1: color = (1, 0, 0) RED
Vertex 2: color = (0, 1, 0) GREEN  
Vertex 3: color = (0, 0, 1) BLUE

Fragment in center: fragColor ≈ (0.33, 0.33, 0.33) GRAY
Fragment near Vertex 1: fragColor ≈ (0.8, 0.1, 0.1) REDDISH
```

### Push Constants

```glsl
layout(push_constant) uniform Push {
  mat4 transform;
  vec3 color;
} push;
```

**Same declaration as vertex shader** - push constants are visible to all shader stages specified in `VkPushConstantRange::stageFlags`.

**Note:** `push.color` is currently **unused** in the fragment shader. The fragment shader uses the interpolated `fragColor` from vertex attributes instead. The push constant color is kept in the structure for potential future use but doesn't affect current rendering.

### Output

```glsl
layout (location = 0) out vec4 outColor;
```

**`outColor`:** User-defined output variable

**Location 0:** Writes to color attachment 0 (the swapchain image)

**vec4 Format:** RGBA (Red, Green, Blue, Alpha)

### Color Output

```glsl
outColor = vec4(fragColor, 1.0);
                ^^^^^^^^^^  ^^^
                Interpolated Alpha
                  Color    (opaque)
```

**`outColor`:** Final RGBA color written to framebuffer (color attachment 0)

**Example:**

For a cube face with vertices colored:
- Corner 1: White (0.9, 0.9, 0.9)
- Corner 2: White (0.9, 0.9, 0.9)  
- Corner 3: White (0.9, 0.9, 0.9)
- Corner 4: White (0.9, 0.9, 0.9)

All fragments get white: `outColor = vec4(0.9, 0.9, 0.9, 1.0)`

For gradient faces (different corner colors), fragments get interpolated values between the vertex colors.

### Rendering Model

**Per-Vertex Color with Interpolation (Current):**
- Each vertex has its own color from vertex buffer
- GPU rasterizer interpolates colors across triangle faces
- Creates smooth color gradients
- Each cube face can have uniform color or gradients

**Vertex Shader → Fragment Shader Flow:**
```glsl
// Vertex shader (runs per vertex)
fragColor = color;  // color from vertex buffer

// Rasterizer (automatic interpolation)
// Generates fragments, interpolates fragColor

// Fragment shader (runs per pixel)
outColor = vec4(fragColor, 1.0);  // Use interpolated color
```

**Why This Approach?**
- Enables per-face colors (each face can be different color)
- Smooth color transitions if adjacent vertices have different colors
- Standard approach for colored 3D geometry
- No need to change push constants per face

---

## Push Constants

### What Are Push Constants?

Push constants provide **fast, small amounts of data** from CPU to GPU:

```
CPU                          GPU
SimplePushConstantData  →   Push uniform block
{                           {
  offset: (0.2, 0.1)          offset: vec2
  color:  (1.0, 0.5, 0.3)     color:  vec3
}                           }
```

### Characteristics

| Property | Value |
|----------|-------|
| **Size Limit** | 128 bytes minimum (some GPUs support more) |
| **Speed** | Fastest CPU→GPU transfer method |
| **Frequency** | Can change per draw call |
| **Visibility** | Any shader stage (vertex, fragment, etc.) |

**Use Cases:**
- Transformation matrices (small models)
- Animation offsets
- Per-object colors
- Time values
- Dynamic parameters

**Not Suitable For:**
- Large datasets (>128 bytes)
- Texture data
- Geometry data

### Declaration (GLSL)

```glsl
layout(push_constant) uniform Push {
  vec2 offset;
  vec3 color;
} push;
```

**Must be identical in all shader stages that use it:**
- Same member names
- Same types
- Same order
- Can use only subset of fields per stage

### Declaration (C++)

```cpp
struct SimplePushConstantData {
  glm::mat2 transform{1.f};     // Identity matrix by default
  glm::vec2 offset;
  alignas(16) glm::vec3 color;  // CRITICAL: 16-byte alignment
};
```

**Alignment Rules (std140):**

| GLSL Type | Size | Alignment |
|-----------|------|-----------|
| `float` | 4 bytes | 4 bytes |
| `vec2` | 8 bytes | 8 bytes |
| `vec3` | 12 bytes | **16 bytes** ⚠️ |
| `vec4` | 16 bytes | 16 bytes |
| `mat2` | 16 bytes | 8 bytes (vec2 columns) |
| `mat3` | 36 bytes | 16 bytes (vec3 columns) |
| `mat4` | 64 bytes | 16 bytes (vec4 columns) |

**Why vec3 needs 16-byte alignment:**
- GPU memory access is optimized for 16-byte chunks
- GLSL std140 layout pads vec3 to 16 bytes
- C++ struct must match exactly

**Example Memory Layout:**

```
Correct Layout (with alignas):
Offset  Data
0x00    mat2 transform (16 bytes) - 4 floats in column-major order
0x10    vec2 offset (8 bytes)
0x18    [padding] (8 bytes)
0x20    vec3 color (12 bytes, aligned to 16)
Total: 40 bytes
```

**Column-Major mat2:**
```
mat2(a, b, c, d) stored as:
0x00: a (col0.x)
0x04: b (col0.y)
0x08: c (col1.x)
0x0C: d (col1.y)
```

### Pipeline Layout Configuration

**In FirstApp::createPipelineLayout():**

```cpp
VkPushConstantRange pushConstantRange{};
pushConstantRange.stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;
pushConstantRange.offset = 0;
pushConstantRange.size = sizeof(SimplePushConstantData);

VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
pipelineLayoutInfo.pushConstantRangeCount = 1;
pipelineLayoutInfo.pPushConstantRanges = &pushConstantRange;
```

**`stageFlags`:** Which shader stages can access push constants
- `VK_SHADER_STAGE_VERTEX_BIT` - Vertex shader
- `VK_SHADER_STAGE_FRAGMENT_BIT` - Fragment shader
- Bitwise OR to enable multiple stages

**`offset`:** Starting byte offset (0 for first range)

**`size`:** Total bytes (must be ≤ 128 bytes minimum)

### Usage in Rendering

**Setting Push Constants:**

```cpp
SimplePushConstantData push{};
push.offset = {-0.05f, -0.4f};
push.color = {0.0f, 0.0f, 0.8f};  // Blue

vkCmdPushConstants(
    commandBuffer,
    pipelineLayout,
    VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT,
    0,  // offset
    sizeof(SimplePushConstantData),
    &push
);

model->draw(commandBuffer);  // Uses push constants
```

**Multiple Draw Calls:**

```cpp
for (int i = 0; i < 4; i++) {
    SimplePushConstantData push{};
    push.offset = {0.0f, i * 0.25f};  // Stack vertically
    push.color = {0.0f, 0.0f, 0.2f + 0.2f * i};  // Increasing blue
    
    vkCmdPushConstants(..., &push);
    model->draw(commandBuffer);  // Draw with different color/position
}
```

**Result:** Same triangle geometry drawn 4 times with different colors and positions.

### Performance Considerations

**Fast:**
- Stored in GPU registers or cache
- No memory allocation
- Immediate updates

**Limitations:**
- Small size limit (128 bytes minimum)
- Frequent updates okay but not every nanosecond

**Comparison:**

| Method | Size | Speed | Update Frequency |
|--------|------|-------|------------------|
| Push Constants | <128 bytes | Fastest | Per draw call |
| Uniform Buffers | Unlimited | Fast | Per frame |
| Vertex Buffers | Unlimited | Medium | Rarely |
| Textures | Unlimited | Varies | Rarely |

---

## Shader Communication

### Data Flow

```
CPU (C++)                          GPU (GLSL)

SimplePushConstantData            layout(push_constant) uniform Push
  ↓                                  ↓
vkCmdPushConstants()              push.offset, push.color
  ↓                                  ↓
Vertex Buffer                     in vec2 position, in vec3 color
  ↓                                  ↓
              [Vertex Shader]
                    ↓
              gl_Position (built-in output)
                    ↓
              [Rasterizer]
                    ↓
              Fragments
                    ↓
              [Fragment Shader]
                    ↓
              out vec4 outColor
                    ↓
              Framebuffer
```

### No Interpolation (Current)

**Vertex Shader → Fragment Shader:**

Previously, data was passed between shaders:
```glsl
// Vertex shader
layout(location = 0) out vec3 fragColor;
fragColor = color;

// Fragment shader
layout(location = 0) in vec3 fragColor;  // Interpolated!
```

**Current Implementation:**

Both shaders read from push constants directly - no inter-shader communication.

**Why No Interpolation?**
- Push constants are constant per draw call
- All fragments of a primitive get same color
- Simpler for solid-color rendering

**When to Use Interpolation:**
- Per-vertex colors with gradients
- Texture coordinates
- Normals for lighting
- Any value that varies across primitive

---

## Compilation

### Manual Compilation

**Windows (compile.bat):**
```batch
cd engine\scripts
compile.bat
```

**Linux/Mac (compile.sh):**
```bash
cd engine/scripts
chmod +x compile.sh
./compile.sh
```

### Compilation Process

```bash
glslc simple_shader.vert -o ../shaders/bin/simple_shader.vert.spv
glslc simple_shader.frag -o ../shaders/bin/simple_shader.frag.spv
```

**glslc:** Shader compiler (part of Vulkan SDK)

**Input:** GLSL source (.vert, .frag)

**Output:** SPIR-V bytecode (.spv)

### SPIR-V

**What Is SPIR-V?**
- Standard Portable Intermediate Representation
- Binary format understood by all Vulkan drivers
- Cross-platform (write once, run anywhere)
- Optimized by driver for specific GPU

**Why Not Compile at Runtime?**
- Faster application startup
- Catches shader errors during development
- Consistent behavior across platforms
- Smaller distribution size (binary vs. source)

### Compilation Errors

**Syntax Error:**
```
simple_shader.vert:10: error: expected ';' at end of statement
```

**Type Mismatch:**
```
simple_shader.frag:5: error: cannot convert from 'vec3' to 'vec4'
```

**Undefined Variable:**
```
simple_shader.vert:8: error: 'pushh' : undeclared identifier
```

**Fix → Recompile → Rebuild Application**

---

## Common Issues

### Issue 1: Shader Compilation Failed

**Error in terminal:**
```
simple_shader.vert:10: error: 'push' : undeclared identifier
```

**Solution:**
1. Fix GLSL syntax error
2. Recompile shaders: `cd engine/scripts && ./compile.bat`
3. Verify .spv files created: `ls engine/shaders/bin/`

### Issue 2: Nothing Renders (Black Screen)

**Possible Causes:**

1. **Push constants not set:**
```cpp
// Missing vkCmdPushConstants() before draw!
model->draw(commandBuffer);
```

2. **Wrong pipeline layout:**
```cpp
// Pipeline layout must declare push constant range
pushConstantRange.size = sizeof(SimplePushConstantData);
```

3. **Color is black:**
```cpp
push.color = {0.0f, 0.0f, 0.0f};  // Invisible on black background!
```

### Issue 3: Validation Errors

**Error:**
```
vkCmdPushConstants(): Offset + size exceeds maxPushConstantsSize
```

**Cause:** Push constant data too large

**Solution:**
```cpp
// Check size
sizeof(SimplePushConstantData) <= 128  // Must be true

// If larger, use uniform buffers instead
```

### Issue 4: Wrong Colors or Positions

**Error:** Visual artifacts, wrong geometry placement

**Cause:** Alignment mismatch

**Solution:**
```cpp
struct SimplePushConstantData {
  glm::vec2 offset;
  alignas(16) glm::vec3 color;  // MUST align vec3 to 16 bytes
};
```

**Verify alignment:**
```cpp
static_assert(offsetof(SimplePushConstantData, color) == 16, "vec3 not aligned!");
```

### Issue 5: Shader Changes Not Applied

**Problem:** Modified shader but no visual change

**Causes:**
1. Forgot to recompile shaders
2. Application using old .spv files
3. Wrong shader path

**Solution:**
```bash
# 1. Recompile
cd engine/scripts
./compile.bat

# 2. Verify .spv timestamp
ls -l engine/shaders/bin/*.spv

# 3. Rebuild application
cmake --build build
```

### Issue 6: Push Constant Data Incorrect in Shader

**Problem:** `push.offset` or `push.color` have wrong values in shader

**Debug Steps:**

1. **Check struct alignment:**
```cpp
std::cout << "offset offset: " << offsetof(SimplePushConstantData, offset) << std::endl;
std::cout << "color offset: " << offsetof(SimplePushConstantData, color) << std::endl;
// Should print: 0, 16
```

2. **Check push constant call:**
```cpp
vkCmdPushConstants(
    commandBuffer,
    pipelineLayout,
    VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT,  // Both stages!
    0,
    sizeof(SimplePushConstantData),
    &push  // Address of actual data
);
```

3. **Verify shader declaration matches:**
```glsl
// GLSL layout must match C++ struct order
layout(push_constant) uniform Push {
  vec2 offset;  // First (offset 0)
  vec3 color;   // Second (offset 16)
} push;
```

---

## Future Enhancements

### 1. Per-Vertex Color Interpolation

**Return to gradient rendering:**

```glsl
// Vertex shader
layout(location = 0) out vec3 fragColor;
void main() {
    gl_Position = vec4(position + push.offset, 0.0, 1.0);
    fragColor = color;  // Pass vertex color to fragment shader
}

// Fragment shader
layout(location = 0) in vec3 fragColor;
void main() {
    outColor = vec4(fragColor, 1.0);  // Use interpolated color
}
```

**Use Case:** Smooth gradients, per-vertex lighting

### 2. Transformation Matrices

**Add MVP matrix to push constants:**

```glsl
layout(push_constant) uniform Push {
  mat4 modelViewProj;  // 64 bytes
  vec3 color;          // 12 bytes (aligned to 16)
} push;                // Total: 80 bytes (within limit)

void main() {
    gl_Position = push.modelViewProj * vec4(position, 0.0, 1.0);
}
```

**Use Case:** 3D transformations, camera movement

### 3. Texture Coordinates

**Add UV coordinates:**

```glsl
// Vertex shader
layout(location = 2) in vec2 texCoord;
layout(location = 0) out vec2 fragTexCoord;

void main() {
    gl_Position = vec4(position + push.offset, 0.0, 1.0);
    fragTexCoord = texCoord;
}

// Fragment shader
layout(location = 0) in vec2 fragTexCoord;
layout(binding = 0) uniform sampler2D texSampler;

void main() {
    outColor = texture(texSampler, fragTexCoord);
}
```

**Use Case:** Textured rendering, sprites

### 4. Time-Based Animation

**Add time to push constants:**

```cpp
struct SimplePushConstantData {
  glm::vec2 offset;
  alignas(16) glm::vec3 color;
  float time;
};
```

```glsl
void main() {
    vec2 animatedOffset = push.offset + vec2(sin(push.time), cos(push.time)) * 0.1;
    gl_Position = vec4(position + animatedOffset, 0.0, 1.0);
}
```

**Use Case:** Procedural animation, wave effects

---

## Related Documentation

- [PIPELINE.md](PIPELINE.md) - Pipeline configuration and shader loading
- [MODEL.md](MODEL.md) - Vertex buffer and attribute descriptions
- [ARCHITECTURE.md](ARCHITECTURE.md) - Overall rendering flow
- [SETUP.md](SETUP.md) - Shader compilation setup

---

