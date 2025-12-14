# Vulkan Engine

Personal game engine project built with Vulkan and C++20.

## Status

🚧 **Early Development** - Working towards rendering a triangle

**Version:** 0.1.0

## Features

- ✅ Window creation (GLFW)
- ✅ Vulkan initialization (volk)

## Building

**Prerequisites:**
- Git
- CMake 3.27+
- C++20 compiler (MinGW/MSVC/GCC/Clang)
- Vulkan-capable GPU drivers

**Clone and Build:**
```bash
git clone https://github.com/eulerfan40/vulkan-engine.git
cd vulkan-engine
mkdir build && cd build
cmake ..
cmake --build .
./engine/engine  # or engine\engine.exe on Windows
```

**First build** takes 2-3 minutes as dependencies download automatically.

## Dependencies

All managed via CMake FetchContent (automatic):
- Vulkan Headers
- volk (Vulkan loader)
- GLFW (windowing)
- GLM (math)

## Notes

- [Setup Guide](docs/SETUP.md) - Build instructions and troubleshooting
- [Configuration](docs/CONFIGURATION.md) - Technical decisions and versions