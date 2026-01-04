# Bismuth Engine

Bismuth Engine is a cross-platform 3D game engine written in C++ and Vulkan.

## Status

🚧 **Early Development** - Working towards 3D rendering.

**Version:** 0.1.0

## Features

- ✅ Window creation (GLFW)
- ✅ Vulkan initialization (volk)
- ✅ Graphics pipeline with shader support
- ✅ Vertex buffer management with color attributes
- ✅ Per-vertex color interpolation
- ✅ SRGB color space for accurate color reproduction
- ✅ Dynamic window resizing with automatic swapchain recreation
- ✅ Render a colored triangle with smooth gradients

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
- **[Pipeline](docs/PIPELINE.md)** - Graphics pipeline configuration
- **[Model](docs/MODEL.md)** - Vertex data and buffer management