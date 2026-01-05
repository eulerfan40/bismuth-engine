# Bismuth Engine

Bismuth Engine is a cross-platform 3D game engine written in C++ and Vulkan.

## Status

🚧 **Early Development** - Working towards 3D rendering.

**Version:** 0.1.0

## Features

- ✅ Window creation (GLFW)
- ✅ Vulkan initialization (volk)
- ✅ **Renderer abstraction** - Clean API for frame lifecycle management
- ✅ **Render system pattern** - Modular, extensible rendering architecture
- ✅ Graphics pipeline with shader support
- ✅ **3D rendering** - Full 3D geometry with depth testing
- ✅ **OBJ model loading** - Load 3D models from OBJ files with automatic vertex deduplication (40-60% memory savings)
- ✅ **Camera system** - Projection matrices (perspective/orthographic) and view transformations
- ✅ **Camera view control** - Position and orient camera with setViewTarget, setViewDirection, and setViewYXZ
- ✅ **Interactive camera control** - Keyboard-based first-person camera with WASD movement and arrow key look controls
- ✅ **Vertex buffer management** - Device-local memory with staging buffers for optimal GPU performance
- ✅ **Index buffer support** - Efficient indexed rendering with shared vertices (33% memory reduction for cubes)
- ✅ GameObject system with component-based architecture
- ✅ **3D transformations** - mat4 with scale, rotation (Euler angles), and translation
- ✅ Push constants for dynamic per-draw-call transformations
- ✅ Per-vertex colors with GPU interpolation
- ✅ SRGB color space for accurate color reproduction
- ✅ Dynamic window resizing with automatic swapchain recreation
- ✅ Automatic aspect ratio handling for correct projection
- ✅ Multiple instance rendering with shared geometry
- ✅ Per-frame animation support

## Building

**Prerequisites:**
- Git
- CMake 3.27+
- C++20 compiler (MinGW/MSVC/GCC/Clang)
- Vulkan-capable GPU drivers
- Vulkan SDK

Note: On Linux, certain X11/Wayland libraries are required to build. See [SETUP](docs/SETUP.md) for more info.

**Clone and Build:**
```bash
git clone https://github.com/eulerfan40/bismuth-engine.git
cd bismuth-engine
mkdir build && cd build
cmake ..
cmake --build .
```

**Run:**
- **Windows:** `engine\bismuth_engine.exe`
- **Linux/macOS:** `./engine/bismuth_engine`

**Compile Shaders:**
Before running, you must compile the shaders.
- **Windows:** Run `engine\scripts\compile.bat`
- **Linux/macOS:** Run `chmod +x engine/scripts/compile.sh && ./engine/scripts/compile.sh`

**First build** takes 2-3 minutes as dependencies download automatically.

## Dependencies

The below dependencies are downloaded automatically using CMake FetchContent:
- Vulkan Headers
- volk (Vulkan loader)
- GLFW (windowing)
- GLM (math)
- tinyobjloader (OBJ file loading)

In addition, you must manually download and install the Vulkan SDK. It comes with 
glslc, which is used to compile shader code. It also provides validation and debugging tools.

## Documentation

- **[Setup Guide](docs/SETUP.md)** - Build instructions and troubleshooting
- **[Architecture Overview](docs/ARCHITECTURE.md)** - System design and component interactions
- **[Configuration](docs/CONFIGURATION.md)** - Technical decisions and versions

### Component Documentation

- **[Window](docs/WINDOW.md)** - Window creation and surface management
- **[Device](docs/DEVICE.md)** - Vulkan device initialization
- **[SwapChain](docs/SWAPCHAIN.md)** - Frame management and synchronization
- **[Renderer](docs/RENDERER.md)** - Frame lifecycle and command buffer management
- **[Render System](docs/RENDERSYSTEM.md)** - Modular rendering architecture
- **[Pipeline](docs/PIPELINE.md)** - Graphics pipeline configuration
- **[Shader](docs/SHADER.md)** - Vertex and fragment shader details
- **[Model](docs/MODEL.md)** - Vertex data, buffer management, and OBJ file loading
- **[Utils](docs/UTILS.md)** - Common utility functions (hash combining)
- **[GameObject](docs/GAMEOBJECT.md)** - Entity system with transform components
- **[Camera](docs/CAMERA.md)** - Projection matrices and view transformations
- **[KeyboardMovementController](docs/KEYBOARDMOVEMENTCONTROLLER.md)** - First-person keyboard camera controls