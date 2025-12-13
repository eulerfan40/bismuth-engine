# Configuration Reference

Quick reference for technical decisions and versions.

## Tech Stack

- **Language:** C++20
- **Graphics:** Vulkan 1.3+
- **Build:** CMake 3.27+
- **Libraries:** Vulkan Headers, volk, GLFW, GLM

## Dependencies

| Library | Version | Purpose |
|---------|---------|---------|
| Vulkan Headers | Latest | Vulkan API |
| volk | Latest | Vulkan loader |
| GLFW | Latest | Windows/Input |
| GLM | Latest | Math |

Using `GIT_SHALLOW TRUE` to grab latest from each repo without previous version history.

**Latest dependencies:**
- All are stable, mature libraries
- Gets bug fixes automatically
- Can pin versions later if needed

## Build Configuration

**Dependencies via FetchContent:**
```cmake
FetchContent_Declare(library GIT_REPOSITORY ... GIT_SHALLOW TRUE)
FetchContent_MakeAvailable(library)
```

**Why this approach:**
- Self-contained (no vcpkg/conan needed)
- Compiler agnostic (builds from source)
- Transparent (see exact versions in CMakeLists.txt)

**volk configuration:**
- Vulkan headers available first
- Then volk builds against them
- No system-wide installed Vulkan SDK needed for development

## Project Structure

```
Game-Engine/
├── CMakeLists.txt      # Root (minimal, adds subdirs)
└── engine/
    ├── CMakeLists.txt  # Main config (FetchContent here)
    └── src/
```

Can add `editor/`, `examples/`, `tests/` later as separate modules.

## Compiler Support

- **Windows:** MinGW (primary), MSVC
- **Linux:** GCC 10+, Clang 10+
- **macOS:** Apple Clang 12+

---
*Notes to self - Dec 2025*