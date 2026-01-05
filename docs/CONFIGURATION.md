# Configuration Reference

Quick reference for technical decisions and versions.

## Tech Stack

- **Language:** C++20
- **Graphics:** Vulkan 1.3+
- **Build:** CMake 3.27+
- **Libraries:** Vulkan Headers, volk, GLFW, GLM, tinyobjloader
- **Dev Tools** Vulkan SDK

## Dependencies

| Library | Version | Purpose |
|---------|---------|---------|
| Vulkan Headers | Latest | Vulkan API |
| volk | Latest | Vulkan loader |
| GLFW | Latest | Windows/Input |
| GLM | Latest | Math |
| tinyobjloader | Release | OBJ file loading |

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
- Scripts: 
  - Windows: `engine/scripts/compile.bat`
  - Linux/macOS: `engine/scripts/compile.sh`

**Asset Paths:**
- CMake exposes `COMPILED_SHADERS_DIR` and `MODELS_DIR` as compile-time macros
- Avoids relative path issues across different IDEs and build configurations
- Usage: `MODELS_DIR "filename.obj"` in C++ code

## Project Structure

```
bismuth-engine/
├── CMakeLists.txt           # Root (orchestrates subdirs)
├── LICENSE                  # MIT License
├── README.md                # Project overview
├── docs/                    # Documentation
│   ├── ARCHITECTURE.md      # High-level architecture overview
│   ├── CONFIGURATION.md     # Technical decisions and versions
│   ├── DEVICE.md            # Device component documentation
│   ├── MODEL.md             # Model component documentation
│   ├── PIPELINE.md          # Pipeline component documentation
│   ├── SETUP.md             # Build instructions
│   ├── SWAPCHAIN.md         # SwapChain component documentation
│   └── WINDOW.md            # Window component documentation
└── engine/
    ├── CMakeLists.txt       # Engine build config
    ├── scripts/             # Build/utility scripts
    │   ├── compile.bat      # Shader compiler (Windows)
    │   └── compile.sh       # Shader compiler (Linux/macOS)
    ├── shaders/             # Shader files
    │   ├── src/             # Shader source (.vert, .frag)
    │   │   ├── simple_shader.vert
    │   │   └── simple_shader.frag
    │   └── bin/             # Compiled shaders (.spv, git-ignored)
    ├── models/              # 3D model assets (.obj files)
    └── src/                 # C++ source files
        ├── main.cpp         # Entry point
        ├── FirstApp.hpp/cpp # Application orchestration
        ├── Window.hpp/cpp   # Window management
        ├── Device.hpp/cpp   # Vulkan device setup
        ├── SwapChain.hpp/cpp # Frame management
        ├── Pipeline.hpp/cpp # Graphics pipeline
        └── Model.hpp/cpp    # Vertex data management
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