# Setup Notes

Personal reference for building and configuring this project.

## Quick Setup

**Windows:**
1. Install Git
2. Install Vulkan SDK
3. Open project in IDE
4. Build:
```bash
cmake --build .
```
5. Compile shaders:
```bash
cd engine/scripts
./compile.bat
```

## What Gets Downloaded

CMake FetchContent automatically grabs:
- Vulkan Headers (latest)
- volk (latest) - Vulkan meta-loader
- GLFW (latest) - windowing
- GLM (latest) - math library

First build: ~3 minutes
Later builds: ~5 seconds

## Vulkan SDK (Required for Development)

**Purpose:** Shader compilation, validation layers, and debugging tools

**Installation:**
- Windows: https://vulkan.lunarg.com/sdk/home#windows
- Linux: `sudo apt install vulkan-sdk`
- macOS: Download from Vulkan website

## Shader Compilation

**Location:** `engine/scripts/compile.bat`

**Compile shaders:**
```bash
cd engine/scripts
./compile.bat    
```

**Structure:**
- Source: `engine/shaders/src/*.vert`, `*.frag`
- Compiled: `engine/shaders/bin/*.spv` (SPIR-V bytecode)

**Note:** `.spv` files are git-ignored (generated files)

## Why These Choices

**FetchContent vs vcpkg/conan/manual:**
- No extra tools to install
- Everything builds from source with my compiler

**volk vs Vulkan SDK for runtime:**
- Don't need SDK installed to run the engine
- Dynamically loads Vulkan at runtime
- SDK only needed for development tools (glslc, validation layers)

## Troubleshooting

**Git not found:**
- Install Git, restart CLion

**glslc not found:**
- Install Vulkan SDK
- Add SDK to PATH
- Restart terminal/CLion

**volk initialization fails:**
- Update GPU drivers

**Build errors:**
- Delete `cmake-build-debug/` and rebuild

## Platform Notes

**Windows:**
- Using MinGW by default (CLion bundled)
- MSVC also works
- Vulkan SDK: Make sure "Add to PATH" is checked during install