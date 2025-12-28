# Bismuth Engine

Bismuth Engine is a cross-platform 3D game engine written in C++ and Vulkan.

## Status

🚧 **Early Development** - Working towards 3D rendering.

**Version:** 0.1.0

## Features

- ✅ Window creation (GLFW)
- ✅ Vulkan initialization (volk)
- ✅ Read simple vertex and fragment shader files

## Building

**Prerequisites:**
- Git
- CMake 3.27+
- C++20 compiler (MinGW/MSVC/GCC/Clang)
- Vulkan-capable GPU drivers
- Vulkan SDK

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

## Notes

- [Setup Guide](docs/SETUP.md) - Build instructions and troubleshooting
- [Configuration](docs/CONFIGURATION.md) - Technical decisions and versions