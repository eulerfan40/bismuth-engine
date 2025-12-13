# Game Engine

Personal game engine project built with Vulkan and C++20.

## Cloning
**Command Line:**
```bash
git clone https://www.github.com/eulerfan40/vulkan-engine
mkdir build && cd build
cmake ..
cmake --build .
```

## Building

**Prerequisites:**
- Git
- CMake 3.27+
- C++20 compiler (MinGW/MSVC/GCC/Clang)
- Vulkan-capable GPU drivers

## Dependencies

All managed via CMake FetchContent (automatic):
- Vulkan Headers
- volk (Vulkan loader)
- GLFW (windowing)
- GLM (math)

## Notes

- Using C++20 for modern features
- volk means no Vulkan SDK installation needed
- FetchContent keeps everything self-contained
- See `docs/SETUP.md` for detailed build info