# Configuration Reference

Quick reference for technical decisions and versions.

## Tech Stack

- **Language:** C++20
- **Graphics:** Vulkan 1.3+
- **Build:** CMake 3.27+
- **Libraries:** Vulkan Headers, volk, GLFW, GLM
- **Dev Tools** Vulkan SDK

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

## Build Configuration

**Dependencies via FetchContent:**
```cmake
FetchContent_Declare(library GIT_REPOSITORY ... GIT_SHALLOW TRUE)
FetchContent_MakeAvailable(library)
```

**Why this approach:**
- Self-contained: no vcpkg/conan needed, no global downloads needed
- Compiler agnostic: builds from source, important for future cross-platform support

**volk configuration:**
- Vulkan headers available first
- Then volk builds against them
- The Vulkan SDK does not provide the headers

## Development Tools

**Vulkan SDK:**
- **Provides:** Shader compilation (`glslc`), validation layers, debugging tools
- The Vulkan SDK must be installed and path added to environment variables
- Comes bundled with glslc by default which is used to compile shader source code

**Shader Compilation:**
- Compiler: `glslc` (from Vulkan SDK)
- Source: `engine/shaders/src/*.vert`, `*.frag`
- Output: `engine/shaders/bin/*.spv` (SPIR-V bytecode)
- Script: `engine/scripts/compile.bat` (Windows)

## Project Structure

```
bismuth-engine/
├── CMakeLists.txt           # Root (orchestrates subdirs)
├── docs/                    # Documentation
│   ├── SETUP.md
│   └── CONFIGURATION.md
└── engine/
    ├── CMakeLists.txt       # Engine build config
    ├── scripts/             # Build/utility scripts
    │   └── compile.bat      # Shader compiler (Windows)
    ├── shaders/             # Shader files
    │   ├── src/            # Shader source (.vert, .frag)
    │   └── bin/            # Compiled shaders (.spv, git-ignored)
    └── src/                 # C++ source files
        └── main.cpp
```

## Naming Conventions

**Files:** PascalCase for classes (`Window.cpp`, `FirstApp.hpp`)
**Shaders:** snake_case (`simple_shader.vert`)
**Executable:** lowercase (`bismuth_engine`)
**Project:** PascalCase (`BismuthEngine`)
**Namespaces:** lowercase (`namespace engine`)

## Compiler Support

- **Windows:** MinGW (primary), MSVC
- **Linux:** GCC 10+, Clang 10+
- **macOS:** Apple Clang 12+