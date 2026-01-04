# Pipeline Component

Complete technical documentation for the Graphics Pipeline system.

**File:** `engine/src/Pipeline.hpp`, `engine/src/Pipeline.cpp`  
**Purpose:** Shader management, graphics pipeline configuration  
**Dependencies:** Device 

---

## Table of Contents

- [Overview](#overview)
- [Graphics Pipeline Explained](#graphics-pipeline-explained)
- [Class Interface](#class-interface)
- [Shader Management](#shader-management)
- [Pipeline Configuration](#pipeline-configuration)
- [Pipeline Stages](#pipeline-stages)
- [Default Configuration](#default-configuration)
- [Common Issues](#common-issues)

---

## Overview

The `Pipeline` class encapsulates the Vulkan graphics pipeline - the sequence of operations that transforms vertices into pixels on screen.

### What Is a Graphics Pipeline?

A graphics pipeline is a **state machine** that defines how rendering happens:

```
Vertices → Pipeline → Pixels

Pipeline contains:
├─ Shaders (vertex, fragment)
├─ Vertex input format
├─ Viewport/scissor
├─ Rasterization settings
├─ Depth/stencil testing
└─ Color blending
```

**Key Concept:** Pipelines are **immutable** - once created, cannot be changed. Must create a new pipeline for different settings.

---

## Graphics Pipeline Explained

### Simplified Pipeline Flow

```
Input Vertices
    ↓
[Vertex Shader]          ← Transforms vertices
    ↓
[Input Assembly]         ← Groups vertices into primitives
    ↓
[Rasterization]          ← Converts triangles to fragments
    ↓
[Fragment Shader]        ← Colors each fragment
    ↓
[Depth Testing]          ← Discards hidden fragments
    ↓
[Color Blending]         ← Blends with framebuffer
    ↓
Framebuffer
```

### Complete Vulkan Pipeline

```
┌─────────────────────────────────────────┐
│         Input Assembler                 │
│  (groups vertices into primitives)      │
└──────────────┬──────────────────────────┘
               ↓
┌─────────────────────────────────────────┐
│         Vertex Shader                   │
│  (transforms vertices, outputs position)│
└──────────────┬──────────────────────────┘
               ↓
┌─────────────────────────────────────────┐
│      Tessellation (optional)            │
└──────────────┬──────────────────────────┘
               ↓
┌─────────────────────────────────────────┐
│      Geometry Shader (optional)         │
└──────────────┬──────────────────────────┘
               ↓
┌─────────────────────────────────────────┐
│         Rasterization                   │
│  (converts triangles to fragments)      │
└──────────────┬──────────────────────────┘
               ↓
┌─────────────────────────────────────────┐
│        Fragment Shader                  │
│  (computes color for each fragment)     │
└──────────────┬──────────────────────────┘
               ↓
┌─────────────────────────────────────────┐
│      Depth/Stencil Testing              │
│  (discards occluded fragments)          │
└──────────────┬──────────────────────────┘
               ↓
┌─────────────────────────────────────────┐
│        Color Blending                   │
│  (blends with existing framebuffer)     │
└──────────────┬──────────────────────────┘
               ↓
        Framebuffer
```

---

## Class Interface

```cpp
struct PipelineConfigInfo {
    PipelineConfigInfo() = default;
    PipelineConfigInfo(const PipelineConfigInfo&) = delete;
    PipelineConfigInfo& operator=(const PipelineConfigInfo&) = delete;

    VkPipelineViewportStateCreateInfo viewportInfo;
    VkPipelineInputAssemblyStateCreateInfo inputAssemblyInfo;
    VkPipelineRasterizationStateCreateInfo rasterizationInfo;
    VkPipelineMultisampleStateCreateInfo multisampleInfo;
    VkPipelineColorBlendAttachmentState colorBlendAttachment;
    VkPipelineColorBlendStateCreateInfo colorBlendInfo;
    VkPipelineDepthStencilStateCreateInfo depthStencilInfo;
    std::vector<VkDynamicState> dynamicStateEnables;
    VkPipelineDynamicStateCreateInfo dynamicStateInfo;
    VkPipelineLayout pipelineLayout = nullptr;
    VkRenderPass renderPass = nullptr;
    uint32_t subpass = 0;
};

class Pipeline {
public:
    Pipeline(Device &device,
             const std::string &vertPath,
             const std::string &fragPath,
             const PipelineConfigInfo &configInfo);
    ~Pipeline();

    // Non-copyable
    Pipeline(const Pipeline &) = delete;
    void operator=(const Pipeline &) = delete;

    // Bind pipeline for rendering
    void bind(VkCommandBuffer commandBuffer);

    // Get default configuration
    static void defaultPipelineConfigInfo(PipelineConfigInfo& configInfo);

private:
    static std::vector<char> readFile(const std::string &path);
    void createGraphicsPipeline(const std::string &vertPath,
                                const std::string &fragPath,
                                const PipelineConfigInfo &configInfo);
    void createShaderModule(const std::vector<char> &code, VkShaderModule *shaderModule);

    Device &device;
    VkPipeline graphicsPipeline;
    VkShaderModule vertShaderModule;
    VkShaderModule fragShaderModule;
};
```

---

## Shader Management

### Reading Shader Files

```cpp
std::vector<char> Pipeline::readFile(const std::string &path) {
    // Open file at end (ate) in binary mode
    std::ifstream file{path, std::ios::ate | std::ios::binary};

    if (!file.is_open()) {
        throw std::runtime_error{"Failed to open file \"" + path + "\"!"};
    }

    // Get file size (we're at end)
    size_t fileSize = static_cast<size_t>(file.tellg());
    std::vector<char> buffer(fileSize);

    // Go back to beginning
    file.seekg(0);
    
    // Read entire file
    file.read(buffer.data(), fileSize);
    
    file.close();
    return buffer;
}
```

**Why `ios::ate`?**
- Opens at end of file
- `tellg()` gives file size immediately
- More efficient than seeking to end after opening

**Binary mode (`ios::binary`):**
- Prevents text conversion (newline handling)
- SPIR-V is binary data, not text
- Critical for correct shader loading

### Creating Shader Modules

```cpp
void Pipeline::createShaderModule(const std::vector<char> &code, VkShaderModule *shaderModule) {
    VkShaderModuleCreateInfo createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    createInfo.codeSize = code.size();
    createInfo.pCode = reinterpret_cast<const uint32_t*>(code.data());

    if (vkCreateShaderModule(device.device(), &createInfo, nullptr, shaderModule) != VK_SUCCESS) {
        throw std::runtime_error("Failed to create shader module!");
    }
}
```

**What Is a Shader Module?**
- GPU object containing compiled SPIR-V bytecode
- Intermediate representation before pipeline creation
- Can be destroyed after pipeline is created

**Why reinterpret_cast?**
- SPIR-V interprets data as 32-bit words
- File data is bytes (`char*`)
- Must reinterpret as `uint32_t*`

### Shader Compilation Pipeline

```
GLSL Source (.vert, .frag)
    ↓ [glslc compiler]
SPIR-V Bytecode (.spv)
    ↓ [readFile()]
std::vector<char>
    ↓ [createShaderModule()]
VkShaderModule
    ↓ [vkCreateGraphicsPipelines()]
VkPipeline
```

**GLSL → SPIR-V:**
```bash
# In engine/scripts/compile.bat
glslc simple_shader.vert -o simple_shader.vert.spv
glslc simple_shader.frag -o simple_shader.frag.spv
```

---

## Pipeline Configuration

### createGraphicsPipeline() - Overview

```cpp
void Pipeline::createGraphicsPipeline(
    const std::string &vertPath,
    const std::string &fragPath,
    const PipelineConfigInfo &configInfo) {
    
    // 1. Validate configuration
    assert(configInfo.pipelineLayout != VK_NULL_HANDLE);
    assert(configInfo.renderPass != VK_NULL_HANDLE);

    // 2. Load and create shader modules
    auto vertCode = readFile(vertPath);
    auto fragCode = readFile(fragPath);
    createShaderModule(vertCode, &vertShaderModule);
    createShaderModule(fragCode, &fragShaderModule);

    // 3. Define shader stages
    VkPipelineShaderStageCreateInfo shaderStages[2];
    // ... configure vertex and fragment stages

    // 4. Configure fixed-function stages
    VkPipelineVertexInputStateCreateInfo vertexInputInfo{};
    // ... all the pipeline configuration

    // 5. Create graphics pipeline
    VkGraphicsPipelineCreateInfo pipelineInfo{};
    // ... assemble all the pieces
    vkCreateGraphicsPipelines(device.device(), VK_NULL_HANDLE, 1, 
                             &pipelineInfo, nullptr, &graphicsPipeline);
}
```

### Shader Stage Configuration

```cpp
// Vertex Shader Stage
shaderStages[0].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
shaderStages[0].stage = VK_SHADER_STAGE_VERTEX_BIT;
shaderStages[0].module = vertShaderModule;
shaderStages[0].pName = "main";  // Entry point function name
shaderStages[0].flags = 0;
shaderStages[0].pNext = nullptr;
shaderStages[0].pSpecializationInfo = nullptr;

// Fragment Shader Stage
shaderStages[1].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
shaderStages[1].stage = VK_SHADER_STAGE_FRAGMENT_BIT;
shaderStages[1].module = fragShaderModule;
shaderStages[1].pName = "main";
// ... same as vertex
```

**`pName` - Entry Point:**
- Specifies which function in shader to execute
- Usually `"main"` by convention
- Can have multiple entry points in one shader

**`pSpecializationInfo` - Shader Constants:**
- Allows setting constants at pipeline creation
- Example: Array sizes, loop counts
- Not used in current implementation

---

## Pipeline Stages

### 1. Input Assembly

```cpp
configInfo.inputAssemblyInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
configInfo.inputAssemblyInfo.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
configInfo.inputAssemblyInfo.primitiveRestartEnable = VK_FALSE;
```

**Topology Types:**

| Topology | Description | Use Case |
|----------|-------------|----------|
| `TRIANGLE_LIST` | Every 3 vertices = 1 triangle | Most common |
| `TRIANGLE_STRIP` | Shared vertices between triangles | Terrain meshes |
| `TRIANGLE_FAN` | All triangles share first vertex | Circles |
| `LINE_LIST` | Every 2 vertices = 1 line | Wireframe |
| `POINT_LIST` | Each vertex = 1 point | Particle systems |

**Primitive Restart:**
- Allows using special index value to break strips/fans
- Disabled for triangle lists (not needed)

### 2. Viewport and Scissor

```cpp
// Viewport: Transformation from NDC to screen space
configInfo.viewport.x = 0.0f;
configInfo.viewport.y = 0.0f;
configInfo.viewport.width = static_cast<float>(width);
configInfo.viewport.height = static_cast<float>(height);
configInfo.viewport.minDepth = 0.0f;  // Near plane
configInfo.viewport.maxDepth = 1.0f;  // Far plane

// Scissor: Clip region (pixels outside are discarded)
configInfo.scissor.offset = {0, 0};
configInfo.scissor.extent = {width, height};
```

**Viewport vs Scissor:**

| Viewport | Scissor |
|----------|---------|
| Transforms coordinates | Clips pixels |
| Can scale/stretch | Hard boundary |
| Affects depth | Only affects XY |

**Coordinate Systems:**

```
Vertex Shader Output (NDC):      Viewport Transform:      Scissor Clip:
   (-1, -1) ┌───┐ (1, 1)          (0,0) ┌────┐ (W,H)      No clip
            │   │           →           │    │         →   
            └───┘                       └────┘              
     Normalized                       Pixels                 Final
```

### 3. Rasterization

```cpp
configInfo.rasterizationInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
configInfo.rasterizationInfo.depthClampEnable = VK_FALSE;
configInfo.rasterizationInfo.rasterizerDiscardEnable = VK_FALSE;
configInfo.rasterizationInfo.polygonMode = VK_POLYGON_MODE_FILL;
configInfo.rasterizationInfo.lineWidth = 1.0f;
configInfo.rasterizationInfo.cullMode = VK_CULL_MODE_NONE;
configInfo.rasterizationInfo.frontFace = VK_FRONT_FACE_CLOCKWISE;
configInfo.rasterizationInfo.depthBiasEnable = VK_FALSE;
```

**Polygon Modes:**

| Mode | Effect | Use Case |
|------|--------|----------|
| `FILL` | Solid triangles | Normal rendering |
| `LINE` | Wireframe | Debugging |
| `POINT` | Vertices only | Debugging |

**Culling:**
- `CULL_MODE_NONE` - Render both sides (current)
- `CULL_MODE_BACK` - Cull back-facing triangles (optimize)
- `CULL_MODE_FRONT` - Cull front-facing (rarely used)

**Front Face:**
- `CLOCKWISE` - Clockwise-wound triangles face camera
- `COUNTER_CLOCKWISE` - Counter-clockwise-wound face camera

```
Clockwise:           Counter-Clockwise:
  v0                      v0
   |\                     |
   | \                    | \
   |  \                   |  \
  v1--v2                 v2--v1
```

### 4. Multisampling (MSAA)

```cpp
configInfo.multisampleInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
configInfo.multisampleInfo.sampleShadingEnable = VK_FALSE;
configInfo.multisampleInfo.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
configInfo.multisampleInfo.minSampleShading = 1.0f;
configInfo.multisampleInfo.pSampleMask = nullptr;
configInfo.multisampleInfo.alphaToCoverageEnable = VK_FALSE;
configInfo.multisampleInfo.alphaToOneEnable = VK_FALSE;
```

**Sample Counts:**
- `1_BIT` - No MSAA (current)
- `2_BIT` - 2x MSAA
- `4_BIT` - 4x MSAA (common)
- `8_BIT` - 8x MSAA (high quality)

**Why Disabled?**
- MSAA adds performance cost
- Requires additional configuration
- Future improvement

### 5. Depth and Stencil Testing

```cpp
configInfo.depthStencilInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
configInfo.depthStencilInfo.depthTestEnable = VK_TRUE;
configInfo.depthStencilInfo.depthWriteEnable = VK_TRUE;
configInfo.depthStencilInfo.depthCompareOp = VK_COMPARE_OP_LESS;
configInfo.depthStencilInfo.depthBoundsTestEnable = VK_FALSE;
configInfo.depthStencilInfo.stencilTestEnable = VK_FALSE;
```

**Depth Test:**
- Compares fragment depth with depth buffer
- `LESS` = Keep fragment if closer to camera
- Prevents rendering hidden surfaces

**Depth Comparison Operators:**

| Operator | Passes When |
|----------|-------------|
| `LESS` | Fragment closer than buffer (common) |
| `LESS_OR_EQUAL` | Fragment closer or equal |
| `GREATER` | Fragment farther (inverted depth) |
| `ALWAYS` | Always pass (disable depth test) |
| `NEVER` | Never pass |

### 6. Color Blending

```cpp
// Per-attachment blending
configInfo.colorBlendAttachment.colorWriteMask = 
    VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT |
    VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
configInfo.colorBlendAttachment.blendEnable = VK_FALSE;
configInfo.colorBlendAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_ONE;
configInfo.colorBlendAttachment.dstColorBlendFactor = VK_BLEND_FACTOR_ZERO;
configInfo.colorBlendAttachment.colorBlendOp = VK_BLEND_OP_ADD;

// Global blending
configInfo.colorBlendInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
configInfo.colorBlendInfo.logicOpEnable = VK_FALSE;
configInfo.colorBlendInfo.attachmentCount = 1;
configInfo.colorBlendInfo.pAttachments = &configInfo.colorBlendAttachment;
```

**Blending Disabled (Current):**
```
Output = Fragment Color
// Overwrites framebuffer completely
```

**Alpha Blending (Future):**
```cpp
colorBlendAttachment.blendEnable = VK_TRUE;
colorBlendAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
colorBlendAttachment.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
colorBlendAttachment.colorBlendOp = VK_BLEND_OP_ADD;
```

**Blending Formula:**
```
finalColor = (srcColor * srcFactor) OP (dstColor * dstFactor)

With alpha blending:
= (fragColor * fragAlpha) + (framebufferColor * (1 - fragAlpha))
```

---

## Default Configuration

### defaultPipelineConfigInfo()

```cpp
static void Pipeline::defaultPipelineConfigInfo(PipelineConfigInfo& configInfo) {
    // Configure all stages with sensible defaults
    // ... (see full implementation in source)
}
```

**Purpose:**
- Provides working configuration for basic rendering
- Configures dynamic viewport and scissor for window resizing
- No textures or complex features
- Good starting point for customization

**Signature Change:**
- **Old:** Returns `PipelineConfigInfo` by value, required width/height parameters
- **New:** Takes `PipelineConfigInfo&` by reference, no width/height needed
- **Reason:** Viewport and scissor are now dynamic state, set at draw time

**Usage:**
```cpp
PipelineConfigInfo pipelineConfig{};
Pipeline::defaultPipelineConfigInfo(pipelineConfig);
pipelineConfig.renderPass = swapChain->getRenderPass();
pipelineConfig.pipelineLayout = pipelineLayout;

Pipeline pipeline(device, "shaders/vert.spv", "shaders/frag.spv", pipelineConfig);
```

---

## Dynamic State

### What Is Dynamic State?

Most pipeline state is **baked in** at creation time - changing it requires creating a new pipeline. However, certain states can be marked as **dynamic**, allowing them to be changed at draw time.

**Static State (Default):**
```cpp
// Create pipeline with fixed viewport
VkViewport viewport{0.0f, 0.0f, 800.0f, 600.0f, 0.0f, 1.0f};
// Viewport is now fixed - can't change without new pipeline
```

**Dynamic State (Current Implementation):**
```cpp
// Create pipeline with dynamic viewport
configInfo.dynamicStateEnables = {VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR};
// Viewport can be set per-frame in command buffer
```

### Why Use Dynamic State?

**Window Resizing Support:**
- Window dimensions can change at runtime
- Without dynamic state: Must recreate pipeline for each resize
- With dynamic state: Keep same pipeline, update viewport/scissor in command buffer

**Performance:**
- Pipeline creation is expensive (~milliseconds)
- Setting dynamic state is cheap (~microseconds)
- Essential for responsive window resizing

### Dynamic State Configuration

**In defaultPipelineConfigInfo():**

```cpp
// Enable viewport and scissor as dynamic state
configInfo.dynamicStateEnables = {VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR};
configInfo.dynamicStateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
configInfo.dynamicStateInfo.pDynamicStates = configInfo.dynamicStateEnables.data();
configInfo.dynamicStateInfo.dynamicStateCount = 
    static_cast<uint32_t>(configInfo.dynamicStateEnables.size());
configInfo.dynamicStateInfo.flags = 0;

// Viewport state now references nullptr (set at draw time)
configInfo.viewportInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
configInfo.viewportInfo.viewportCount = 1;
configInfo.viewportInfo.pViewports = nullptr;  // Dynamic!
configInfo.viewportInfo.scissorCount = 1;
configInfo.viewportInfo.pScissors = nullptr;   // Dynamic!
```

### Setting Dynamic State at Draw Time

**In command buffer recording:**

```cpp
void recordCommandBuffer(int imageIndex) {
    vkBeginCommandBuffer(commandBuffers[imageIndex], &beginInfo);
    vkCmdBeginRenderPass(commandBuffers[imageIndex], &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);

    // Set viewport dynamically
    VkViewport viewport{};
    viewport.x = 0.0f;
    viewport.y = 0.0f;
    viewport.width = static_cast<float>(swapChain->getSwapChainExtent().width);
    viewport.height = static_cast<float>(swapChain->getSwapChainExtent().height);
    viewport.minDepth = 0.0f;
    viewport.maxDepth = 1.0f;
    vkCmdSetViewport(commandBuffers[imageIndex], 0, 1, &viewport);

    // Set scissor dynamically
    VkRect2D scissor{{0, 0}, swapChain->getSwapChainExtent()};
    vkCmdSetScissor(commandBuffers[imageIndex], 0, 1, &scissor);

    // Now bind pipeline and draw
    pipeline->bind(commandBuffers[imageIndex]);
    model->bind(commandBuffers[imageIndex]);
    model->draw(commandBuffers[imageIndex]);

    vkCmdEndRenderPass(commandBuffers[imageIndex]);
    vkEndCommandBuffer(commandBuffers[imageIndex]);
}
```

**Key Points:**
- Dynamic state must be set **before** draw commands
- Must be set **after** binding render pass
- Values can change every frame (e.g., after window resize)
- Validation layers will error if dynamic state not set

### Other Common Dynamic States

While we currently only use viewport and scissor, Vulkan supports many dynamic states:

| Dynamic State | Use Case |
|---------------|----------|
| `VK_DYNAMIC_STATE_VIEWPORT` | Window resizing, split-screen |
| `VK_DYNAMIC_STATE_SCISSOR` | Window resizing, UI clipping |
| `VK_DYNAMIC_STATE_LINE_WIDTH` | Debug rendering, wireframe |
| `VK_DYNAMIC_STATE_BLEND_CONSTANTS` | Fade effects, UI transparency |
| `VK_DYNAMIC_STATE_DEPTH_BIAS` | Shadow mapping |
| `VK_DYNAMIC_STATE_STENCIL_REFERENCE` | Stencil masking |

**Trade-off:**
- More dynamic states = more flexibility
- Fewer dynamic states = better driver optimization
- Use dynamic state only when needed

---

## Pipeline Binding

### bind() Method

```cpp
void Pipeline::bind(VkCommandBuffer commandBuffer) {
    vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, graphicsPipeline);
}
```

**What This Does:**
- Sets the active graphics pipeline for rendering
- All subsequent draw commands use this pipeline
- Must rebind to switch pipelines

**Usage Pattern:**
```cpp
vkCmdBeginRenderPass(commandBuffer, ...);
pipeline.bind(commandBuffer);
vkCmdDraw(commandBuffer, 3, 1, 0, 0);  // Uses this pipeline
vkCmdEndRenderPass(commandBuffer);
```

**Pipeline Switching:**
```cpp
pipelineA.bind(commandBuffer);
vkCmdDraw(...);  // Uses pipeline A

pipelineB.bind(commandBuffer);
vkCmdDraw(...);  // Uses pipeline B
```

---

## Common Issues

### Issue 1: Shader Module Creation Failed

**Error:**
```
Failed to create shader module!
```

**Causes:**
1. Shader file not found
2. Shader not compiled (.spv missing)
3. Corrupted SPIR-V bytecode

**Solutions:**
```bash
# Compile shaders
cd engine/scripts
./compile.bat

# Verify .spv files exist
ls ../shaders/bin/*.spv
```

### Issue 2: Pipeline Creation Failed

**Error:**
```
vkCreateGraphicsPipelines returned error
```

**Common causes:**
- Missing render pass
- Missing pipeline layout
- Incompatible shader stages
- Invalid configuration values

**Debug:**
```cpp
assert(configInfo.pipelineLayout != VK_NULL_HANDLE);
assert(configInfo.renderPass != VK_NULL_HANDLE);
```

### Issue 3: Nothing Renders

**Check:**
1. Pipeline bound before draw call?
2. Correct vertex count?
3. Viewport/scissor correct?
4. Culling removing triangles?

**Debug culling:**
```cpp
config.rasterizationInfo.cullMode = VK_CULL_MODE_NONE;  // Disable
```

### Issue 4: Validation Errors About Vertex Input

**Error:**
```
Vertex shader expects input at location 0 but no vertex input binding provided
```

**Cause:** Mismatch between shader and pipeline vertex input

**Solution (Current):**
- We have NO vertex input (data in shader)
- Vertex input state is empty (correct for our setup)

```cpp
vertexInputInfo.vertexBindingDescriptionCount = 0;     // No bindings
vertexInputInfo.vertexAttributeDescriptionCount = 0;   // No attributes
```

---

## Shader Examples

### Current Vertex Shader

```glsl
#version 460

// Hardcoded triangle vertices
vec2 positions[3] = vec2[] (
    vec2(0.0, -0.5),
    vec2(0.5, 0.5),
    vec2(-0.5, 0.5)
);

void main () {
    gl_Position = vec4(positions[gl_VertexIndex], 0.0, 1.0);
}
```

**Why hardcoded?**
- Simplest possible setup
- No vertex buffers needed
- Good for initial testing

### Current Fragment Shader

```glsl
#version 460

layout (location = 0) out vec4 outColor;

void main() {
    outColor = vec4(1.0, 1.0, 0.0, 1.0);  // Yellow
}
```

**Future: Vertex Buffers**

```glsl
// Future vertex shader with input
layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inColor;

layout(location = 0) out vec3 fragColor;

void main() {
    gl_Position = vec4(inPosition, 1.0);
    fragColor = inColor;
}
```

---

## Performance Considerations

### Pipeline State Objects (PSOs)

**Vulkan pipelines are expensive to create:**
- Compile shaders
- Validate configuration
- Generate GPU code

**Best Practices:**
1. Create pipelines at startup, not per-frame
2. Reuse pipelines when possible
3. Consider pipeline caching (future)

### Pipeline Caching

```cpp
// Future improvement
VkPipelineCache pipelineCache;
vkCreatePipelineCache(device, &cacheInfo, nullptr, &pipelineCache);
vkCreateGraphicsPipelines(device, pipelineCache, 1, &pipelineInfo, ...);
```

**Benefits:**
- Faster creation on subsequent runs
- Can save cache to disk
- Significantly reduces startup time

---

## Destruction

```cpp
Pipeline::~Pipeline() {
    vkDestroyShaderModule(device.device(), vertShaderModule, nullptr);
    vkDestroyShaderModule(device.device(), fragShaderModule, nullptr);
    vkDestroyPipeline(device.device(), graphicsPipeline, nullptr);
}
```

**Note:** Shader modules can be destroyed immediately after pipeline creation!

```cpp
// This is also valid:
createGraphicsPipeline(...);
vkDestroyShaderModule(device, vertShaderModule, nullptr);  // Safe!
vkDestroyShaderModule(device, fragShaderModule, nullptr);  // Safe!
// Pipeline still works - modules are compiled into pipeline
```

---

## Related Documentation

- **[Model Component](MODEL.md)** - Provides vertex input descriptors (position, color attributes)
- **[SwapChain Component](SWAPCHAIN.md)** - Provides render pass and SRGB framebuffer format
- **[Device Component](DEVICE.md)** - Creates and manages pipeline objects
- **[Architecture Overview](ARCHITECTURE.md)** - Pipeline's role in rendering system and color interpolation

---