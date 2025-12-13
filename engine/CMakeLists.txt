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

# Engine executable
add_executable(engine )

# Link libraries
target_link_libraries(engine PRIVATE
        volk
        glfw
        glm::glm
)