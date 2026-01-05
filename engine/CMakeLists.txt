cmake_minimum_required(VERSION 3.27)

include(FetchContent)

# Fetch Vulkan Headers FIRST
FetchContent_Declare(
        vulkan_headers
        GIT_REPOSITORY https://github.com/KhronosGroup/Vulkan-Headers.git
        GIT_SHALLOW TRUE
)

# Make Vulkan headers available before volk
FetchContent_MakeAvailable(vulkan_headers)

# Fetch volk (Vulkan meta-loader)
FetchContent_Declare(
        volk
        GIT_REPOSITORY https://github.com/zeux/volk.git
        GIT_SHALLOW TRUE
)

# Fetch GLFW
FetchContent_Declare(
        glfw
        GIT_REPOSITORY https://github.com/glfw/glfw.git
        GIT_SHALLOW TRUE
)

# Fetch GLM
FetchContent_Declare(
        glm
        GIT_REPOSITORY https://github.com/g-truc/glm.git
        GIT_SHALLOW TRUE
)

# Configure GLFW
set(GLFW_BUILD_DOCS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(GLFW_INSTALL OFF CACHE BOOL "" FORCE)

# Download and make available volk separately so we can configure it
FetchContent_MakeAvailable(volk)

# Tell volk where Vulkan headers are AFTER it's populated but BEFORE it builds
target_include_directories(volk PUBLIC ${vulkan_headers_SOURCE_DIR}/include)

# Download and make available the rest
FetchContent_MakeAvailable(glfw glm)

# Set a path to the compiled shader directory to avoid IDE-specific CWD relative path issues
set(COMPILED_SHADERS_DIR "${CMAKE_SOURCE_DIR}/engine/shaders/bin/")

# Engine executable
add_executable(bismuth_engine
        src/main.cpp
        src/FirstApp.hpp
        src/FirstApp.cpp
        src/Window.hpp
        src/Window.cpp
        src/Device.hpp
        src/Device.cpp
        src/SwapChain.hpp
        src/SwapChain.cpp
        src/Pipeline.hpp
        src/Pipeline.cpp
        src/Model.hpp
        src/Model.cpp
        src/GameObject.hpp
        src/Renderer.hpp
        src/Renderer.cpp
        src/SimpleRenderSystem.hpp
        src/SimpleRenderSystem.cpp
        src/Camera.cpp
        src/Camera.hpp
        src/KeyboardMovementController.cpp
        src/KeyboardMovementController.hpp
)

# Set compiler-specific warning flags
if(MSVC)
    target_compile_options(bismuth_engine PRIVATE /W4)
else()
    target_compile_options(bismuth_engine PRIVATE -Wall -Wextra -pedantic)
endif()

# Expose the compiled shaders directory to C++ as a compile-time constant string macro
target_compile_definitions(bismuth_engine PRIVATE COMPILED_SHADERS_DIR="${COMPILED_SHADERS_DIR}")

# Link libraries
target_link_libraries(bismuth_engine PRIVATE
        volk
        glfw
        glm::glm
)