# Window Component

Complete technical documentation for the Window system.

**File:** `engine/src/Window.hpp`, `engine/src/Window.cpp`  
**Purpose:** Window creation, Vulkan surface management, and initialization of volk  
**Dependencies:** GLFW, volk 

---

## Table of Contents

- [Overview](#overview)
- [Class Interface](#class-interface)
- [Initialization Process](#initialization-process)
- [Technical Deep Dive](#technical-deep-dive)

---

## Overview

The `Window` class is responsible for three critical tasks:

1. **Initialize volk** - Load the Vulkan function loader
2. **Create an OS window** - Using GLFW for cross-platform support
3. **Provide a Vulkan surface** - Connect Vulkan to the window for rendering

### Why This Design?

The Window class is intentionally minimal. It handles only windowing concerns and delegates Vulkan device/rendering setup to other components. This separation allows the window to be platform-agnostic while keeping Vulkan-specific logic isolated.

---

## Class Interface

### Header Definition

```cpp
namespace engine {
  class Window {
  public:
    Window(int w, int h, std::string name);
    ~Window();

    // Prevent copying (Vulkan objects are non-copyable)
    Window(const Window &) = delete;
    Window &operator=(const Window &) = delete;

    // Query methods
    bool shouldClose() { return glfwWindowShouldClose(window); }
    bool wasWindowResized() { return framebufferResized; }
    void resetWindowResizedFlag() { framebufferResized = false; }
    VkExtent2D getExtent() { 
      return {static_cast<uint32_t>(width), static_cast<uint32_t>(height)}; 
    }
    GLFWwindow* getGLFWwindow() const { return window; }

    // Vulkan integration
    void createWindowSurface(VkInstance instance, VkSurfaceKHR* surface);

  private:
    static void frameBufferResizeCallback(GLFWwindow* window, int width, int height);
    void initWindow();

    int width;
    int height;
    bool framebufferResized;
    std::string windowName;
    GLFWwindow *window;
  };
}
```

### Member Variables

| Variable | Type | Purpose |
|----------|------|---------|
| `width` | `int` | Window width in pixels (updated on resize) |
| `height` | `int` | Window height in pixels (updated on resize) |
| `framebufferResized` | `bool` | Flag indicating if window was resized since last check |
| `windowName` | `std::string` | Title displayed in window title bar |
| `window` | `GLFWwindow*` | GLFW window handle (raw pointer managed by GLFW) |

### Public Methods

| Method | Return Type | Purpose |
|--------|-------------|---------|
| `shouldClose()` | `bool` | Returns true if window close requested (X button, Alt+F4, etc.) |
| `wasWindowResized()` | `bool` | Returns true if window was resized since last reset |
| `resetWindowResizedFlag()` | `void` | Clears the resize flag after handling resize |
| `getExtent()` | `VkExtent2D` | Returns current window dimensions as Vulkan extent |
| `getGLFWwindow()` | `GLFWwindow*` | Returns raw GLFW window handle for direct access |
| `createWindowSurface()` | `void` | Creates Vulkan surface for rendering to window |

**Window Resizing Support:**
- Width and height are now mutable to support dynamic window resizing
- `framebufferResized` flag is set by GLFW callback when window is resized
- Application can query resize state with `wasWindowResized()` and reset with `resetWindowResizedFlag()`
- Resizing triggers swapchain recreation to match new window dimensions

**Direct GLFW Access:**
- `getGLFWwindow()` provides access to underlying GLFW window handle
- Required for input handling (`glfwGetKey()`, `glfwGetMouseButton()`, etc.)
- Used by `KeyboardMovementController` to poll keyboard state
- Enables integration with GLFW-based input systems

**Usage Example:**
```cpp
// In game loop - checking for keyboard input
GLFWwindow* windowHandle = window.getGLFWwindow();

// Poll key states
if (glfwGetKey(windowHandle, GLFW_KEY_ESCAPE) == GLFW_PRESS) {
    glfwSetWindowShouldClose(windowHandle, GLFW_TRUE);
}

// Used by KeyboardMovementController
KeyboardMovementController controller{};
controller.moveInPlaneXZ(window.getGLFWwindow(), deltaTime, viewerObject);
```

**Why raw pointer for window?**
- GLFW manages window lifecycle internally
- Using `std::unique_ptr` would require custom deleter
- Current approach is simpler and idiomatic for GLFW

---

## Initialization Process

### Constructor Flow

```cpp
Window::Window(int w, int h, std::string name) 
    : width{w}, height{h}, windowName{name} {
    initWindow();
}
```

**Member Initializer List:**
- Initializes const members before constructor body
- More efficient than assignment in constructor body
- Required for const members (can't assign after construction)

### initWindow() - Step by Step

```cpp
void Window::initWindow() {
    // 1. Initialize volk - MUST BE FIRST
    if (volkInitialize() != VK_SUCCESS) {
        throw std::runtime_error("Failed to initialize Vulkan!");
    }

    // 2. Initialize GLFW library
    glfwInit();
    
    // 3. Configure window hints
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE);

    // 4. Create the window
    window = glfwCreateWindow(width, height, windowName.c_str(), nullptr, nullptr);
    
    // 5. Set up window resize callback
    glfwSetWindowUserPointer(window, this);
    glfwSetWindowSizeCallback(window, frameBufferResizeCallback);
}
```

---

## Technical Deep Dive

### Stage 1: volk Initialization

```cpp
if (volkInitialize() != VK_SUCCESS) {
    throw std::runtime_error("Failed to initialize Vulkan!");
}
```

**What volkInitialize() Does:**

1. **Loads Vulkan Loader DLL/SO:**
    - Windows: `vulkan-1.dll`
    - Linux: `libvulkan.so.1`
    - macOS: `libvulkan.1.dylib` (via MoltenVK)

2. **Loads Base Functions:**
    - `vkCreateInstance`
    - `vkEnumerateInstanceExtensionProperties`
    - `vkEnumerateInstanceLayerProperties`

3. **Returns:**
    - `VK_SUCCESS` - Vulkan loader found and loaded
    - `VK_ERROR_INITIALIZATION_FAILED` - Vulkan not available on system

**Why This Can Fail:**
- ❌ Vulkan driver not installed (update GPU drivers)
- ❌ Outdated drivers (pre-Vulkan support)
- ❌ System without Vulkan-capable GPU

**Critical: This Must Happen First**

volk uses function pointers that are loaded dynamically. If you call any Vulkan function before `volkInitialize()`, you'll get a segfault (null function pointer).

```cpp
// WRONG - Will crash!
glfwInit();
vkCreateInstance(...);  // ← CRASH! volkInitialize() not called yet

// CORRECT
volkInitialize();       // ← Load function pointers first
vkCreateInstance(...);  // ← Now safe to call
```

### Stage 2: GLFW Initialization

```cpp
glfwInit();
```

**What glfwInit() Does:**

1. **Platform-Specific Setup:**
    - Windows: Initialize Win32 API
    - Linux: Initialize X11 or Wayland
    - macOS: Initialize Cocoa

2. **Input System:**
    - Keyboard state tracking
    - Mouse position/button tracking
    - Joystick enumeration

3. **Monitor Enumeration:**
    - Detect connected displays
    - Query display modes/resolutions

**Return Value:**
- `GLFW_TRUE` (1) on success
- `GLFW_FALSE` (0) on failure

### Stage 3: Window Configuration

```cpp
glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE);
```

**Window Hints:**

Window hints configure the window **before** creation. They must be set before calling `glfwCreateWindow()`.

#### GLFW_CLIENT_API = GLFW_NO_API

**Purpose:** Tell GLFW we're using Vulkan, not OpenGL.

**What This Prevents:**
- ❌ GLFW creating an OpenGL context
- ❌ Including OpenGL headers
- ❌ Conflicts between Vulkan and OpenGL

**Without this:**
```cpp
// Without GLFW_NO_API
glfwCreateWindow(...);  // Creates OpenGL context automatically
// Now you have both OpenGL AND Vulkan contexts (conflict!)
```

**With this:**
```cpp
// With GLFW_NO_API
glfwCreateWindow(...);  // No graphics context created
// We'll create Vulkan surface manually
```

#### GLFW_RESIZABLE = GLFW_TRUE

**Purpose:** Enable window resizing.

**How It Works:**
- Users can resize the window by dragging edges/corners
- GLFW triggers a callback when the window size changes
- The application handles swapchain recreation to match new dimensions
- Essential for responsive desktop applications

### Stage 5: Resize Callback Setup

```cpp
glfwSetWindowUserPointer(window, this);
glfwSetWindowSizeCallback(window, frameBufferResizeCallback);
```

**Window User Pointer:**
- `glfwSetWindowUserPointer()` stores a pointer to our Window object inside the GLFW window
- This allows the static callback function to access the Window instance
- Necessary because GLFW callbacks are C-style functions (no `this` pointer)

**Resize Callback Registration:**
- `glfwSetWindowSizeCallback()` registers a function to be called when window is resized
- Callback signature: `void callback(GLFWwindow* window, int width, int height)`
- Must be a static function or free function (GLFW is C-based)

**The Callback Implementation:**

```cpp
void Window::frameBufferResizeCallback(GLFWwindow* window, int width, int height) {
    auto pWindow = reinterpret_cast<Window*>(glfwGetWindowUserPointer(window));
    pWindow->framebufferResized = true;
    pWindow->width = width;
    pWindow->height = height;
}
```

**Callback Flow:**
1. User resizes window → GLFW detects size change
2. GLFW calls `frameBufferResizeCallback()` with new dimensions
3. Retrieve Window object pointer using `glfwGetWindowUserPointer()`
4. Update width/height to new values
5. Set `framebufferResized` flag to notify application
6. Application queries flag with `wasWindowResized()` and recreates swapchain

### Stage 4: Window Creation

```cpp
window = glfwCreateWindow(width, height, windowName.c_str(), nullptr, nullptr);
```

**Parameters:**

| Parameter | Value | Purpose |
|-----------|-------|---------|
| `width` | 800 | Window width in pixels |
| `height` | 600 | Window height in pixels |
| `windowName.c_str()` | "Bismuth Engine" | Title bar text |
| `monitor` (4th) | `nullptr` | Windowed mode (not fullscreen) |
| `share` (5th) | `nullptr` | No context sharing (not using OpenGL) |

**Returns:**
- `GLFWwindow*` - Window handle on success
- `nullptr` - Creation failed

**What Gets Created:**

1. **OS Window:**
    - Windows: HWND (Win32 window)
    - Linux: X11 Window or Wayland Surface
    - macOS: NSWindow (Cocoa window)

2. **Window Properties:**
    - Title bar with name
    - Minimize/maximize/close buttons
    - Fixed size (non-resizable)
    - Default position (OS decides)

3. **Event Queue:**
    - Keyboard events
    - Mouse events
    - Window events (focus, minimize, etc.)

**Not Created:**
- ❌ OpenGL context (we set GLFW_NO_API)
- ❌ Vulkan surface (created later via `createWindowSurface()`)

---

## Vulkan Surface Creation

### createWindowSurface() Method

```cpp
void Window::createWindowSurface(VkInstance instance, VkSurfaceKHR *surface) {
    if (glfwCreateWindowSurface(instance, window, nullptr, surface) != VK_SUCCESS) {
        throw std::runtime_error("Failed to create window surface!");
    }
}
```

**Called By:** `Device::createSurface()` (after VkInstance is created)

**What Is a VkSurfaceKHR?**

A `VkSurfaceKHR` is Vulkan's abstraction for a "drawable area". It connects Vulkan to the windowing system.

```
┌──────────────────────────────────┐
│      Operating System            │
│  ┌────────────────────────────┐  │
│  │      OS Window (GLFW)      │  │
│  │  ┌──────────────────────┐  │  │
│  │  │   VkSurfaceKHR       │  │  │ ← Vulkan renders here
│  │  │  (rendering target)  │  │  │
│  │  └──────────────────────┘  │  │
│  └────────────────────────────┘  │
└──────────────────────────────────┘
```

**Platform-Specific Implementation:**

GLFW abstracts the platform differences:

| Platform | Native Surface | Vulkan Extension |
|----------|---------------|------------------|
| Windows | HWND | VK_KHR_win32_surface |
| Linux (X11) | X11 Window | VK_KHR_xlib_surface |
| Linux (Wayland) | wl_surface | VK_KHR_wayland_surface |
| macOS | CAMetalLayer | VK_EXT_metal_surface |

**Under the Hood:**

```cpp
// What glfwCreateWindowSurface() does (simplified)
#ifdef _WIN32
    VkWin32SurfaceCreateInfoKHR createInfo = {};
    createInfo.hwnd = glfwGetWin32Window(window);
    vkCreateWin32SurfaceKHR(instance, &createInfo, nullptr, surface);
#elif __linux__
    VkXlibSurfaceCreateInfoKHR createInfo = {};
    createInfo.window = glfwGetX11Window(window);
    vkCreateXlibSurfaceKHR(instance, &createInfo, nullptr, surface);
#endif
```

**Why We Use GLFW's Helper:**
- ✅ Cross-platform (one line works everywhere)
- ✅ Handles all platform-specific details
- ✅ Automatically selects correct Vulkan extension

---

## Destructor and Cleanup

```cpp
Window::~Window() {
    glfwDestroyWindow(window);
    glfwTerminate();
}
```

### Destruction Order

**1. glfwDestroyWindow(window)**

Destroys the window and frees associated resources:
- OS window handle
- Input event queue
- Window state

**2. glfwTerminate()**

Shuts down GLFW library:
- Destroys all remaining windows (safety check)
- Releases platform-specific resources
- Unloads input devices

**Critical: VkSurfaceKHR Not Destroyed Here**

The `VkSurfaceKHR` is owned by `Device`, not `Window`. It must be destroyed by the Device destructor:

```cpp
// Device::~Device()
vkDestroySurfaceKHR(instance, surface_, nullptr);  // ← Device destroys surface
```

**Destruction Order in Application:**

```cpp
FirstApp app;  // Constructor order: Window → Device → SwapChain → Pipeline
// ...
// Destructor order (reverse): Pipeline → SwapChain → Device → Window
```

Vulkan objects must be destroyed **before** the window:
```
~Pipeline()    → Destroys pipeline objects
~SwapChain()   → Destroys framebuffers, images, semaphores
~Device()      → Destroys VkSurfaceKHR ← Surface destroyed here
~Window()      → Destroys GLFW window  ← Window destroyed last
```

---

## Copy Prevention

```cpp
Window(const Window &) = delete;
Window &operator=(const Window &) = delete;
```

**Why Delete Copy Operations?**

1. **GLFW Windows Are Not Copyable:**
    - `GLFWwindow*` is a unique resource
    - Multiple copies would point to same window
    - Destructor would destroy window twice → crash

2. **Vulkan Surfaces Are Not Copyable:**
    - `VkSurfaceKHR` is a Vulkan handle
    - Copying would create multiple owners
    - Double-free on destruction

**Example of Problem Without Deletion:**

```cpp
// If copying was allowed...
Window window1(800, 600, "Engine");
Window window2 = window1;  // Copy!

// Now both window1 and window2 point to same GLFWwindow*

window1.~Window();  // Destroys GLFW window
window2.~Window();  // Tries to destroy already-destroyed window → CRASH!
```

**Correct Usage:**

```cpp
// Pass by reference (no copy)
void useWindow(Window& window) { ... }

// Pass by pointer (no copy)
void useWindow(Window* window) { ... }

// Move semantics (future improvement)
Window window = std::move(createWindow());
```

---

## Related Documentation

- **[Device Component](DEVICE.md)** - Uses Window to create VkSurfaceKHR
- **[Architecture Overview](ARCHITECTURE.md)** - Window's role in initialization
- **[GLFW Documentation](https://www.glfw.org/docs/latest/)** - Official GLFW reference
- **[volk Documentation](https://github.com/zeux/volk)** - volk function loader

---

**Related Documentation:**
- **[Device Component](DEVICE.md)** - Uses Window to create VkSurfaceKHR
- **[Architecture Overview](ARCHITECTURE.md)** - Window's role in initialization
- **[Setup Guide](SETUP.md)** - Platform-specific window requirements