# Setup Notes

Personal reference for building and configuring this project.

## Quick Setup

**Windows (CLion):**
1. Install Git
2. Open project in CLion
3. Build (Ctrl+F9)

**Command Line:**
```bash
mkdir build && cd build
cmake ..
cmake --build .
```

## What Gets Downloaded

CMake FetchContent automatically grabs:
- Vulkan Headers (latest)
- volk (latest) - Vulkan meta-loader
- GLFW (latest) - windowing
- GLM (latest) - math library

First build: ~3 minutes
Later builds: ~5 seconds

## Why These Choices

**FetchContent vs vcpkg/conan/manual:**
- No extra tools to install
- Everything builds from source with my compiler

**volk vs Vulkan SDK:**
- Don't need SDK installed to develop
- Dynamically loads Vulkan at runtime
- Lower barrier when switching machines

## Troubleshooting

**Git not found:**
- Install Git, restart CLion

**volk initialization fails:**
- Update GPU drivers

**Build errors:**
- Delete `build/` or `cmake-build-*/` and rebuild

## Platform Notes

**Windows:**
- Using MinGW by default (CLion bundled)
- MSVC also works

**Linux:**
- Need: `build-essential cmake libxrandr-dev libxinerama-dev libxcursor-dev libxi-dev`

**macOS:**
- Need: Xcode command line tools
- `brew install cmake`

---

Last updated: Dec 2025