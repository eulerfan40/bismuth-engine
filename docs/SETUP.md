# Setup Notes

Quick reference for building and configuring this project.

## Windows

## Linux

### Prerequisites
1. Verify that you are on the latest Vulkan-compatible GPU Drivers. One helpful tool to verify compatibility is 
   vulkan-tools:
```bash
sudo apt install vulkan-tools
vulkaninfo | less
```
2. Install Git:
```bash
sudo apt install git
```
3. Install CMake:
```bash
sudo apt install cmake
```
4. Install a C++ 20 compatible compiler. One example is gcc, which is bundled with build-essential:
```bash
sudo apt install build-essential
```
5. Install required system dependencies, which include pkg-config (required by CMake) and certain X11/Wayland libraries 
   (required by glfw):
```bash
sudo apt install libx11-dev libxcursor-dev libxrandr-dev libxinerama-dev libxi-dev libgl1-mesa-dev libxkbcommon-dev 
libwayland-dev wayland-protocols pkg-config
```
6. Install the [Vulkan SDK](https://vulkan.lunarg.com/sdk/home) Tarball. Extract the .tar.xz file to 
   a convenient location. Within the extracted folder, locate the setup_env.sh file and copy its path. Then add the 
   following line to your .bashrc or .
   zshrc file (located in the home directory):
```bash
# Replace the following with the copied path to the setup_env.sh file
source <path-to-setup_env.sh>
```

### Clone and Build

1. Clone the GitHub repository:
```bash
git clone https://github.com/eulerfan40/bismuth-engine.git
cd bismuth-engine
```
2. Build the source files:
```bash
mkdir build && cd build
cmake ..
cmake --build .
```

During the build process, CMake FetchContent automatically grabs:
- Vulkan Headers (latest)
- volk (latest) - Vulkan meta-loader
- GLFW (latest) - windowing
- GLM (latest) - math library
- tinyobjloader (release) - OBJ file loading

First build: ~3 minutes \
Later builds: ~5 seconds 

### Compile Shaders and Run

1. Before running the engine, and after any changes to the shader source files, you must run the shader compilation 
   script:
```bash
cd ../engine/scripts
chmod +x compile.sh
./compile.sh
```
2. Run the engine executable:
```bash
cd ../../build
./engine/bismuth_engine
```

## MacOS

**Windows:**
1. Install Git
2. Install Vulkan SDK
3. Open project in IDE
4. Build:
```bash
cmake --build .
engine/bismuth_engine.exe
```
5. Compile shaders:
```bash
cd engine/scripts
./compile.bat
```

**Linux / macOS:**
1. Install Git, CMake, Compiler, Vulkan SDK
2. Install system dependencies (see [Platform Notes](#platform-notes))
3. Build:
```bash
mkdir build && cd build
cmake ..
cmake --build .
```
4. Compile shaders:
```bash
cd ../engine/scripts
chmod +x compile.sh
./compile.sh
```
5. Run:
```bash
cd ../../build
./engine/bismuth_engine
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
- Windows: https://vulkan.lunarg.com/sdk/home
- Linux: `sudo apt install vulkan-sdk` (Ubuntu/Debian) or via package manager
- macOS: Download from Vulkan website (LunarG). **Important:** You must source the `setup-env.sh` script included in the SDK to add tools like `glslc` to your PATH. Add `source ~/VulkanSDK/x.x.x.x/macOS/setup-env.sh` to your `.zshrc`.

## Shader Compilation

**Location:** `engine/scripts/`

**Compile shaders:**

You must compile shaders after every change in order to see the effects in the engine.

Linux/MacOS
```bash
cd ../engine/scripts
chmod +x compile.sh
./compile.sh
```
Windows
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
- Everything builds from source with the compiler

**volk vs Vulkan SDK for runtime:**
- The engine does not need to know the path of the SDK to load Vulkan
- Dynamically loads Vulkan at runtime
- SDK only needed for development tools (glslc, validation layers)

## Platform Notes

**Windows:**
- Using MinGW by default
- MSVC also works
- Vulkan SDK: Make sure "Add to PATH" is checked during install

**Linux:**
- **Dependencies:** GLFW requires X11/Wayland libraries to build.
  ```bash
  sudo apt install libx11-dev libxcursor-dev libxrandr-dev libxinerama-dev libxi-dev libgl1-mesa-dev libxkbcommon-dev libwayland-dev wayland-protocols
  ```

**macOS:**
- **Dependencies:** Xcode Command Line Tools (`xcode-select --install`).
- **MoltenVK:** The engine automatically enables the required `VK_KHR_portability_subset` extension.