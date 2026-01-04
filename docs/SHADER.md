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

layout(location = 0) in vec2 position;
layout(location = 1) in vec3 color;

layout(push_constant) uniform Push {
  vec2 offset;
  vec3 color;
} push;

// Executed once per vertex we have
void main () {
  //gl_Position is the output position in clip coordinates (x: -1 (left) - (right) 1, y: -1 (up) - (down) 1)
  gl_Position = vec4(position + push.offset, 0.0, 1.0);
}
```

### Input Attributes

```glsl
layout(location = 0) in vec2 position;
layout(location = 1) in vec3 color;
```

| Location | Type | Source | Purpose |
|----------|------|--------|---------|
| 0 | `vec2` | Vertex buffer | 2D position (x, y) |
| 1 | `vec3` | Vertex buffer | RGB color (unused in current implementation) |

**Matching CPU Side:**

```cpp
// In Model.hpp
static std::vector<VkVertexInputAttributeDescription> Vertex::getAttributeDescriptions() {
    std::vector<VkVertexInputAttributeDescription> attributeDescriptions(2);
    
    attributeDescriptions[0].binding = 0;
    attributeDescriptions[0].location = 0;  // Matches shader location 0
    attributeDescriptions[0].format = VK_FORMAT_R32G32_SFLOAT;  // vec2
    attributeDescriptions[0].offset = offsetof(Vertex, position);
    
    attributeDescriptions[1].binding = 0;
    attributeDescriptions[1].location = 1;  // Matches shader location 1
    attributeDescriptions[1].format = VK_FORMAT_R32G32B32_SFLOAT;  // vec3
    attributeDescriptions[1].offset = offsetof(Vertex, color);
    
    return attributeDescriptions;
}
```

**Note:** The `color` input attribute is currently unused - color comes from push constants instead.

### Push Constants

```glsl
layout(push_constant) uniform Push {
  vec2 offset;
  vec3 color;
} push;
```

**Purpose:** Fast, per-draw-call data from CPU to GPU.

**Access:** `push.offset`, `push.color`

**CPU Side Declaration:**

```cpp
struct SimplePushConstantData {
  glm::vec2 offset;
  alignas(16) glm::vec3 color;  // Alignment requirement!
};
```

**Why `alignas(16)`?**
- GLSL std140 layout rules require vec3 to be aligned to 16 bytes
- Without alignment: GPU reads wrong memory location
- Causes visual glitches or validation errors

See [Push Constants](#push-constants) section for complete details.

### Output

```glsl
gl_Position = vec4(position + push.offset, 0.0, 1.0);
```

**`gl_Position`:** Built-in output variable (vec4)

**Clip Space Coordinates:**
- x: -1.0 (left) to +1.0 (right)
- y: -1.0 (top) to +1.0 (bottom)
- z: 0.0 (near) to 1.0 (far)
- w: 1.0 (no perspective division)

**Transformation Breakdown:**

```glsl
vec4(position + push.offset, 0.0, 1.0)
     ^^^^^^^^   ^^^^^^^^^^^^  ^^^  ^^^
        |            |         |    |
     Base 2D      Offset     Z=0  W=1
   (from vertex) (animated) (flat) (ortho)
```

**Example:**
```glsl
position = vec2(0.0, -0.5)
push.offset = vec2(0.2, 0.1)

gl_Position = vec4(0.2, -0.4, 0.0, 1.0)
```

### Key Features

1. **Dynamic Positioning:** `push.offset` allows moving geometry per draw call
2. **2D Rendering:** Z is always 0.0 (flat plane)
3. **No Transformation Matrix:** Direct clip space coordinates
4. **Per-Vertex Execution:** Runs independently for each vertex

---

## Fragment Shader

**File:** `engine/shaders/src/simple_shader.frag`

### Complete Source

```glsl
#version 460

// Variable stores and RGBA output color that should be written to color attachment 0
layout (location = 0) out vec4 outColor;

layout(push_constant) uniform Push {
  vec2 offset;
  vec3 color;
} push;

void main() {
  outColor = vec4(push.color, 1.0);
}
```

### Push Constants

```glsl
layout(push_constant) uniform Push {
  vec2 offset;
  vec3 color;
} push;
```

**Same declaration as vertex shader** - push constants are visible to all shader stages specified in `VkPushConstantRange::stageFlags`.

**Access:** `push.color` - RGB color from CPU

**Note:** `push.offset` is not used in fragment shader (only vertex shader needs position data).

### Output

```glsl
layout (location = 0) out vec4 outColor;
```

**`outColor`:** User-defined output variable

**Location 0:** Writes to color attachment 0 (the swapchain image)

**vec4 Format:** RGBA (Red, Green, Blue, Alpha)

### Color Output

```glsl
outColor = vec4(push.color, 1.0);
                ^^^^^^^^^^^  ^^^
                    RGB       Alpha
               (from CPU)  (opaque)
```

**Example:**
```glsl
push.color = vec3(0.8, 0.2, 0.4)  // Pink-ish

outColor = vec4(0.8, 0.2, 0.4, 1.0)
                ^^^  ^^^  ^^^  ^^^
                Red  Grn  Blu  Alpha=1 (opaque)
```

### Rendering Model

**Flat Color:**
- Entire primitive (triangle) rendered with same color
- No interpolation or gradients
- Color comes entirely from push constants

**Previous Implementation (Per-Vertex Color):**
```glsl
// Old vertex shader
layout(location = 0) out vec3 fragColor;
fragColor = color;  // Pass vertex color to fragment shader

// Old fragment shader
layout(location = 0) in vec3 fragColor;
outColor = vec4(fragColor, 1.0);  // Interpolated color
```

**Why Changed?**
- Push constants enable dynamic per-draw-call colors
- Can draw same geometry multiple times with different colors
- More flexible for rendering multiple instances

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

**Why vec3 needs 16-byte alignment:**
- GPU memory access is optimized for 16-byte chunks
- GLSL std140 layout pads vec3 to 16 bytes
- C++ struct must match exactly

**Example Memory Layout:**

```
Without alignas(16):           With alignas(16):
Offset  Data                   Offset  Data
0x00    vec2 offset (8 bytes)  0x00    vec2 offset (8 bytes)
0x08    vec3 color (12 bytes)  0x10    vec3 color (12 bytes)  ✓
        ^ WRONG OFFSET!               ^ Correct 16-byte aligned
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

