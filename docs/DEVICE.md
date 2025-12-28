# Device Component

Complete technical documentation for the Vulkan Device management system.

**File:** `engine/src/Device.hpp`, `engine/src/Device.cpp`  
**Purpose:** Vulkan instance creation, GPU selection, logical device, command pools  
**Dependencies:** Window, volk 

---

## Table of Contents

- [Overview](#overview)
- [Class Interface](#class-interface)
- [Initialization Sequence](#initialization-sequence)
- [Vulkan Instance](#vulkan-instance)
- [Physical Device Selection](#physical-device-selection)
- [Logical Device Creation](#logical-device-creation)
- [Queue Families](#queue-families)
- [Validation Layers](#validation-layers)
- [Command Pools](#command-pools)
- [Helper Functions](#helper-functions)

---

## Overview

The `Device` class is the core of Vulkan initialization. It manages the entire Vulkan setup process from creating an instance to setting up command pools.

### Responsibilities

1. **VkInstance** - Connection to Vulkan API
2. **Physical Device** - GPU selection
3. **Logical Device** - Interface to GPU with specific features
4. **Queue Families** - Command submission queues (graphics, present)
5. **Validation Layers** - Debug/error checking (development only)
6. **Surface Management** - Window surface for rendering
7. **Command Pool** - Command buffer allocation

### Lifecycle

```
Constructor:
  1. createInstance()        → VkInstance
  2. setupDebugMessenger()   → Validation layers
  3. createSurface()         → VkSurfaceKHR (from Window)
  4. pickPhysicalDevice()    → VkPhysicalDevice (GPU)
  5. createLogicalDevice()   → VkDevice
  6. createCommandPool()     → VkCommandPool

Destructor (reverse order):
  6. vkDestroyCommandPool()
  5. vkDestroyDevice()
  2. DestroyDebugUtilsMessengerEXT()
  3. vkDestroySurfaceKHR()
  1. vkDestroyInstance()
```

---

## Class Interface

### Public Interface

```cpp
class Device {
public:
    // Configuration
    #ifdef NDEBUG
        const bool enableValidationLayers = false;  // Release
    #else
        const bool enableValidationLayers = true;   // Debug
    #endif

    // Constructor/Destructor
    Device(Window &window);
    ~Device();

    // Non-copyable, non-movable
    Device(const Device &) = delete;
    Device& operator=(const Device &) = delete;
    Device(Device &&) = delete;
    Device& operator=(Device &&) = delete;

    // Accessors
    VkCommandPool getCommandPool() { return commandPool; }
    VkDevice device() { return device_; }
    VkSurfaceKHR surface() { return surface_; }
    VkQueue graphicsQueue() { return graphicsQueue_; }
    VkQueue presentQueue() { return presentQueue_; }

    // Query methods
    SwapChainSupportDetails getSwapChainSupport();
    QueueFamilyIndices findPhysicalQueueFamilies();
    VkFormat findSupportedFormat(...);
    uint32_t findMemoryType(...);

    // Buffer helpers
    void createBuffer(...);
    void copyBuffer(...);
    void copyBufferToImage(...);
    VkCommandBuffer beginSingleTimeCommands();
    void endSingleTimeCommands(VkCommandBuffer);
    void createImageWithInfo(...);

    VkPhysicalDeviceProperties properties;  // GPU info

private:
    // Initialization methods
    void createInstance();
    void setupDebugMessenger();
    void createSurface();
    void pickPhysicalDevice();
    void createLogicalDevice();
    void createCommandPool();

    // Helper methods
    bool isDeviceSuitable(VkPhysicalDevice);
    bool checkValidationLayerSupport();
    std::vector<const char*> getRequiredExtensions();
    QueueFamilyIndices findQueueFamilies(VkPhysicalDevice);
    // ... more helpers

    // Vulkan objects
    VkInstance instance;
    VkDebugUtilsMessengerEXT debugMessenger;
    VkPhysicalDevice physicalDevice = VK_NULL_HANDLE;
    VkDevice device_;
    VkSurfaceKHR surface_;
    VkQueue graphicsQueue_;
    VkQueue presentQueue_;
    VkCommandPool commandPool;

    Window &window;  // Reference to window

    // Configuration
    const std::vector<const char*> validationLayers = {
        "VK_LAYER_KHRONOS_validation"
    };
    const std::vector<const char*> deviceExtensions = {
        VK_KHR_SWAPCHAIN_EXTENSION_NAME
    };
};
```

---

## Initialization Sequence

### Complete Flow

```cpp
Device::Device(Window &window) : window{window} {
    createInstance();
    setupDebugMessenger();
    createSurface();
    pickPhysicalDevice();
    createLogicalDevice();
    createCommandPool();
}
```

Each method builds on the previous:

| Step | Creates | Required By |
|------|---------|-------------|
| 1. createInstance() | VkInstance | Everything |
| 2. setupDebugMessenger() | Debug callback | Development only |
| 3. createSurface() | VkSurfaceKHR | Physical device selection |
| 4. pickPhysicalDevice() | VkPhysicalDevice | Logical device |
| 5. createLogicalDevice() | VkDevice, VkQueues | All rendering |
| 6. createCommandPool() | VkCommandPool | Command buffers |

---

## Vulkan Instance

### createInstance() - Detailed Breakdown

```cpp
void Device::createInstance() {
    // 1. Check validation layers
    if (enableValidationLayers && !checkValidationLayerSupport()) {
        throw std::runtime_error("validation layers requested, but not available!");
    }

    // 2. Application info
    VkApplicationInfo appInfo = {};
    appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    appInfo.pApplicationName = "Bismuth Engine";
    appInfo.applicationVersion = VK_MAKE_VERSION(0, 1, 0);
    appInfo.pEngineName = "Bismuth Engine";
    appInfo.engineVersion = VK_MAKE_VERSION(0, 1, 0);
    appInfo.apiVersion = VK_API_VERSION_1_3;

    // 3. Instance create info
    VkInstanceCreateInfo createInfo = {};
    createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    createInfo.pApplicationInfo = &appInfo;

    // 4. Extensions
    auto extensions = getRequiredExtensions();
    createInfo.enabledExtensionCount = static_cast<uint32_t>(extensions.size());
    createInfo.ppEnabledExtensionNames = extensions.data();

    // 5. Validation layers (debug only)
    VkDebugUtilsMessengerCreateInfoEXT debugCreateInfo;
    if (enableValidationLayers) {
        createInfo.enabledLayerCount = static_cast<uint32_t>(validationLayers.size());
        createInfo.ppEnabledLayerNames = validationLayers.data();
        populateDebugMessengerCreateInfo(debugCreateInfo);
        createInfo.pNext = &debugCreateInfo;
    } else {
        createInfo.enabledLayerCount = 0;
        createInfo.pNext = nullptr;
    }

    // 6. Create instance
    if (vkCreateInstance(&createInfo, nullptr, &instance) != VK_SUCCESS) {
        throw std::runtime_error("failed to create instance!");
    }

    // 7. CRITICAL: Load instance-level functions
    volkLoadInstance(instance);

    // 8. Verify extensions
    hasGflwRequiredInstanceExtensions();
}
```

### Application Info

```cpp
VkApplicationInfo appInfo = {};
appInfo.pApplicationName = "Bismuth Engine";  // Your app name
appInfo.applicationVersion = VK_MAKE_VERSION(0, 1, 0); // Your app version
appInfo.pEngineName = "Bismuth Engine";                     // Engine name
appInfo.engineVersion = VK_MAKE_VERSION(0, 1, 0);      // Engine version
appInfo.apiVersion = VK_API_VERSION_1_3;               // Vulkan API version
```

**Purpose:**
- Informational (driver can optimize based on engine)
- Debugging (shows in validation layer messages)
- Required by Vulkan specification

**Version Macros:**
```cpp
VK_MAKE_VERSION(major, minor, patch)
// VK_MAKE_VERSION(0, 1, 0) = 0.1.0

VK_API_VERSION_1_0  // Vulkan 1.0
VK_API_VERSION_1_1  // Vulkan 1.1
VK_API_VERSION_1_2  // Vulkan 1.2
VK_API_VERSION_1_3  // Vulkan 1.3 (recommended)
```

**Why VK_API_VERSION_1_3?**
- Modern features
- Good compatibility
- Device can still support newer versions

### Required Extensions

```cpp
std::vector<const char*> Device::getRequiredExtensions() {
    uint32_t glfwExtensionCount = 0;
    const char** glfwExtensions;
    glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);

    std::vector<const char*> extensions(
        glfwExtensions, 
        glfwExtensions + glfwExtensionCount
    );

    if (enableValidationLayers) {
        extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
    }

    return extensions;
}
```

**GLFW Extensions (Platform-Specific):**

| Platform | Extensions |
|----------|-----------|
| Windows | `VK_KHR_surface`, `VK_KHR_win32_surface` |
| Linux (X11) | `VK_KHR_surface`, `VK_KHR_xlib_surface` |
| Linux (Wayland) | `VK_KHR_surface`, `VK_KHR_wayland_surface` |
| macOS | `VK_KHR_surface`, `VK_EXT_metal_surface` |

**Debug Extension:**
- `VK_EXT_debug_utils` - Validation layer messages
- Only added in debug builds
- Allows callback for validation errors/warnings

### volkLoadInstance() - Critical!

```cpp
volkLoadInstance(instance);
```

**What This Does:**
Loads instance-level Vulkan functions:
- `vkEnumeratePhysicalDevices`
- `vkGetPhysicalDeviceProperties`
- `vkCreateDevice`
- All other instance-scope functions

**Why Necessary:**
volk uses function pointers loaded dynamically. After creating the instance, we must load instance-specific functions.

**Three-Stage Loading:**
1. `volkInitialize()` (Window) → Base loader functions
2. `volkLoadInstance(instance)` (Device) → Instance functions
3. `volkLoadDevice(device)` (Device) → Device functions

```cpp
volkInitialize();                    // Stage 1: Can call vkCreateInstance
volkLoadInstance(instance);          // Stage 2: Can call vkEnumeratePhysicalDevices
volkLoadDevice(device);              // Stage 3: Can call vkCreateCommandPool
```

---

## Physical Device Selection

### pickPhysicalDevice()

```cpp
void Device::pickPhysicalDevice() {
    // 1. Enumerate GPUs
    uint32_t deviceCount = 0;
    vkEnumeratePhysicalDevices(instance, &deviceCount, nullptr);
    
    if (deviceCount == 0) {
        throw std::runtime_error("failed to find GPUs with Vulkan support!");
    }

    std::vector<VkPhysicalDevice> devices(deviceCount);
    vkEnumeratePhysicalDevices(instance, &deviceCount, devices.data());

    // 2. Find suitable GPU
    for (const auto &device : devices) {
        if (isDeviceSuitable(device)) {
            physicalDevice = device;
            break;
        }
    }

    if (physicalDevice == VK_NULL_HANDLE) {
        throw std::runtime_error("failed to find a suitable GPU!");
    }

    // 3. Query device properties
    vkGetPhysicalDeviceProperties(physicalDevice, &properties);
    std::cout << "physical device: " << properties.deviceName << std::endl;
}
```

### GPU Suitability Check

```cpp
bool Device::isDeviceSuitable(VkPhysicalDevice device) {
    // 1. Check queue families (graphics + present)
    QueueFamilyIndices indices = findQueueFamilies(device);

    // 2. Check device extensions (swapchain support)
    bool extensionsSupported = checkDeviceExtensionSupport(device);

    // 3. Check swapchain adequacy
    bool swapChainAdequate = false;
    if (extensionsSupported) {
        SwapChainSupportDetails swapChainSupport = querySwapChainSupport(device);
        swapChainAdequate = !swapChainSupport.formats.empty() && 
                           !swapChainSupport.presentModes.empty();
    }

    // 4. Check required features
    VkPhysicalDeviceFeatures supportedFeatures;
    vkGetPhysicalDeviceFeatures(device, &supportedFeatures);

    return indices.isComplete() && 
           extensionsSupported && 
           swapChainAdequate &&
           supportedFeatures.samplerAnisotropy;
}
```

**Requirements for "Suitable" GPU:**

1. **Queue Families:** Must have graphics and present queues
2. **Extensions:** Must support `VK_KHR_swapchain`
3. **Swapchain:** Must have surface formats and present modes
4. **Features:** Must support anisotropic filtering

**Why These Requirements?**
- Graphics queue: Needed for rendering
- Present queue: Needed for displaying to screen
- Swapchain: Needed for frame buffering
- Anisotropic filtering: Better texture quality

### VkPhysicalDevice vs VkDevice

| VkPhysicalDevice | VkDevice |
|------------------|----------|
| GPU hardware handle | Logical interface to GPU |
| Read-only | Can submit commands |
| Query capabilities | Execute operations |
| Select features | Use selected features |
| One per GPU | Can create multiple |

**Analogy:**
- Physical Device = The actual graphics card in your computer
- Logical Device = Your application's connection to that card

---

## Queue Families

### What Are Queue Families?

Queue families are groups of command queues with specific capabilities.

```
GPU
├── Queue Family 0 (Graphics + Compute + Transfer)
│   ├── Queue 0
│   ├── Queue 1
│   └── Queue 2
│
├── Queue Family 1 (Transfer only)
│   └── Queue 0
│
└── Queue Family 2 (Compute only)
    ├── Queue 0
    └── Queue 1
```

### findQueueFamilies()

```cpp
QueueFamilyIndices Device::findQueueFamilies(VkPhysicalDevice device) {
    QueueFamilyIndices indices;

    // 1. Get queue families
    uint32_t queueFamilyCount = 0;
    vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, nullptr);

    std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
    vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, queueFamilies.data());

    // 2. Find graphics and present queues
    int i = 0;
    for (const auto &queueFamily : queueFamilies) {
        // Graphics queue
        if (queueFamily.queueCount > 0 && 
            queueFamily.queueFlags & VK_QUEUE_GRAPHICS_BIT) {
            indices.graphicsFamily = i;
            indices.graphicsFamilyHasValue = true;
        }

        // Present queue
        VkBool32 presentSupport = false;
        vkGetPhysicalDeviceSurfaceSupportKHR(device, i, surface_, &presentSupport);
        if (queueFamily.queueCount > 0 && presentSupport) {
            indices.presentFamily = i;
            indices.presentFamilyHasValue = true;
        }

        if (indices.isComplete()) break;
        i++;
    }

    return indices;
}
```

### QueueFamilyIndices Structure

```cpp
struct QueueFamilyIndices {
    uint32_t graphicsFamily;           // Index of graphics queue family
    uint32_t presentFamily;            // Index of present queue family
    bool graphicsFamilyHasValue = false;
    bool presentFamilyHasValue = false;

    bool isComplete() { 
        return graphicsFamilyHasValue && presentFamilyHasValue; 
    }
};
```

**Why Track "HasValue"?**
- Queue family index could be 0 (valid)
- Can't use 0 to check if found
- Separate bool tracks if we've found it

**Graphics vs Present Queues:**

| Queue | Purpose | Required Capability |
|-------|---------|-------------------|
| Graphics | Rendering commands | `VK_QUEUE_GRAPHICS_BIT` |
| Present | Display to screen | Surface support |

**Often Same Family:**
Most GPUs use the same queue for graphics and present:
```cpp
graphicsFamily = 0
presentFamily = 0  // Same family
```

**Sometimes Different:**
Some systems have separate queues:
```cpp
graphicsFamily = 0  // Discrete GPU
presentFamily = 1   // Integrated GPU (for display)
```

---

## Logical Device Creation

### createLogicalDevice()

```cpp
void Device::createLogicalDevice() {
    QueueFamilyIndices indices = findQueueFamilies(physicalDevice);

    // 1. Create queue create infos (handle potential duplicates)
    std::vector<VkDeviceQueueCreateInfo> queueCreateInfos;
    std::set<uint32_t> uniqueQueueFamilies = {
        indices.graphicsFamily, 
        indices.presentFamily
    };

    float queuePriority = 1.0f;
    for (uint32_t queueFamily : uniqueQueueFamilies) {
        VkDeviceQueueCreateInfo queueCreateInfo = {};
        queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
        queueCreateInfo.queueFamilyIndex = queueFamily;
        queueCreateInfo.queueCount = 1;
        queueCreateInfo.pQueuePriorities = &queuePriority;
        queueCreateInfos.push_back(queueCreateInfo);
    }

    // 2. Specify device features
    VkPhysicalDeviceFeatures deviceFeatures = {};
    deviceFeatures.samplerAnisotropy = VK_TRUE;

    // 3. Create device
    VkDeviceCreateInfo createInfo = {};
    createInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
    createInfo.queueCreateInfoCount = static_cast<uint32_t>(queueCreateInfos.size());
    createInfo.pQueueCreateInfos = queueCreateInfos.data();
    createInfo.pEnabledFeatures = &deviceFeatures;
    createInfo.enabledExtensionCount = static_cast<uint32_t>(deviceExtensions.size());
    createInfo.ppEnabledExtensionNames = deviceExtensions.data();

    if (enableValidationLayers) {
        createInfo.enabledLayerCount = static_cast<uint32_t>(validationLayers.size());
        createInfo.ppEnabledLayerNames = validationLayers.data();
    }

    if (vkCreateDevice(physicalDevice, &createInfo, nullptr, &device_) != VK_SUCCESS) {
        throw std::runtime_error("failed to create logical device!");
    }

    // 4. CRITICAL: Load device-level functions
    volkLoadDevice(device_);

    // 5. Get queue handles
    vkGetDeviceQueue(device_, indices.graphicsFamily, 0, &graphicsQueue_);
    vkGetDeviceQueue(device_, indices.presentFamily, 0, &presentQueue_);
}
```

### Queue Priority

```cpp
float queuePriority = 1.0f;
```

**Purpose:** Hints to driver about queue importance

**Range:** 0.0 to 1.0
- 0.0 = Lowest priority
- 1.0 = Highest priority

**Reality:** Most drivers ignore this
- Single-queue applications: Priority doesn't matter
- Multi-queue applications: Might help scheduling

### Device Extensions

```cpp
const std::vector<const char*> deviceExtensions = {
    VK_KHR_SWAPCHAIN_EXTENSION_NAME  // Required for rendering to screen
};
```

**VK_KHR_swapchain:**
- Enables swapchain creation
- Without this: Can't present to screen
- Required for any windowed application

**Future Extensions:**
```cpp
VK_KHR_ray_tracing_pipeline  // Ray tracing
VK_EXT_mesh_shader          // Mesh shaders
VK_KHR_dynamic_rendering    // Modern rendering
```

### volkLoadDevice() - Critical!

```cpp
volkLoadDevice(device_);
```

**Loads device-level functions:**
- `vkCreateCommandPool`
- `vkAllocateCommandBuffers`
- `vkCreateBuffer`
- `vkCreateImage`
- All rendering functions

**Without this:** Segfault when calling device functions!

---

## Validation Layers

### What Are Validation Layers?

Validation layers are **optional** debugging layers that intercept Vulkan calls and check for errors.

**Without Validation:**
```cpp
vkCreateBuffer(device, &badInfo, ...);  // Silently fails or crashes later
```

**With Validation:**
```
validation layer: vkCreateBuffer(): size cannot be zero
The Vulkan spec states: size must be greater than 0
```

### Debug Callback

```cpp
static VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(
    VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
    VkDebugUtilsMessageTypeFlagsEXT messageType,
    const VkDebugUtilsMessengerCallbackDataEXT *pCallbackData,
    void *pUserData) {
    
    std::cerr << "validation layer: " << pCallbackData->pMessage << std::endl;
    return VK_FALSE;
}
```

**Message Severities:**
- `VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT` - Diagnostic
- `VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT` - Informational
- `VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT` - Potential issues
- `VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT` - Errors

**Return Value:**
- `VK_FALSE` - Continue execution
- `VK_TRUE` - Abort the Vulkan call (rarely used)

### Performance Impact

| Build Type | Validation | Performance Impact |
|------------|-----------|-------------------|
| Debug | ✅ Enabled | ~20-30% slower |
| Release | ❌ Disabled | No impact |

**Why Disable in Release?**
- Significant performance cost
- Only useful for debugging
- End users don't need validation

---

## Command Pools

### createCommandPool()

```cpp
void Device::createCommandPool() {
    QueueFamilyIndices queueFamilyIndices = findPhysicalQueueFamilies();

    VkCommandPoolCreateInfo poolInfo = {};
    poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    poolInfo.queueFamilyIndex = queueFamilyIndices.graphicsFamily;
    poolInfo.flags = VK_COMMAND_POOL_CREATE_TRANSIENT_BIT | 
                     VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;

    if (vkCreateCommandPool(device_, &poolInfo, nullptr, &commandPool) != VK_SUCCESS) {
        throw std::runtime_error("failed to create command pool!");
    }
}
```

### Command Pool Purpose

**What Is It?**
- Memory allocator for command buffers
- One pool can allocate many command buffers
- Tied to specific queue family

**Why Needed?**
- Command buffers can't be created directly
- Must be allocated from a pool
- Pools optimize allocation/deallocation

### Command Pool Flags

**VK_COMMAND_POOL_CREATE_TRANSIENT_BIT:**
- Hint: Command buffers are short-lived
- Optimizes for frequent reset/reallocation
- Good for dynamic command buffers

**VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT:**
- Allows individual command buffer reset
- Without this: Must reset entire pool
- More flexible but slightly more overhead

**Future: Multiple Pools**
```cpp
VkCommandPool graphicsPool;  // For rendering
VkCommandPool transferPool;  // For data uploads
VkCommandPool computePool;   // For compute shaders
```

---

## Helper Functions

### Buffer Creation

```cpp
void Device::createBuffer(
    VkDeviceSize size,
    VkBufferUsageFlags usage,
    VkMemoryPropertyFlags properties,
    VkBuffer &buffer,
    VkDeviceMemory &bufferMemory) {
    
    // 1. Create buffer
    VkBufferCreateInfo bufferInfo{};
    bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    bufferInfo.size = size;
    bufferInfo.usage = usage;
    bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    vkCreateBuffer(device_, &bufferInfo, nullptr, &buffer);

    // 2. Query memory requirements
    VkMemoryRequirements memRequirements;
    vkGetBufferMemoryRequirements(device_, buffer, &memRequirements);

    // 3. Allocate memory
    VkMemoryAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    allocInfo.allocationSize = memRequirements.size;
    allocInfo.memoryTypeIndex = findMemoryType(
        memRequirements.memoryTypeBits, 
        properties
    );

    vkAllocateMemory(device_, &allocInfo, nullptr, &bufferMemory);

    // 4. Bind memory to buffer
    vkBindBufferMemory(device_, buffer, bufferMemory, 0);
}
```

**Usage Example:**
```cpp
VkBuffer vertexBuffer;
VkDeviceMemory vertexBufferMemory;

device.createBuffer(
    sizeof(vertices),
    VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
    VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
    vertexBuffer,
    vertexBufferMemory
);
```

### Single-Time Commands

```cpp
VkCommandBuffer Device::beginSingleTimeCommands() {
    VkCommandBufferAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    allocInfo.commandPool = commandPool;
    allocInfo.commandBufferCount = 1;

    VkCommandBuffer commandBuffer;
    vkAllocateCommandBuffers(device_, &allocInfo, &commandBuffer);

    VkCommandBufferBeginInfo beginInfo{};
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

    vkBeginCommandBuffer(commandBuffer, &beginInfo);
    return commandBuffer;
}

void Device::endSingleTimeCommands(VkCommandBuffer commandBuffer) {
    vkEndCommandBuffer(commandBuffer);

    VkSubmitInfo submitInfo{};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &commandBuffer;

    vkQueueSubmit(graphicsQueue_, 1, &submitInfo, VK_NULL_HANDLE);
    vkQueueWaitIdle(graphicsQueue_);  // Synchronous

    vkFreeCommandBuffers(device_, commandPool, 1, &commandBuffer);
}
```

**Purpose:** One-time operations (data uploads, layout transitions)

**Usage Pattern:**
```cpp
VkCommandBuffer cmd = device.beginSingleTimeCommands();
// Record commands...
vkCmdCopyBuffer(cmd, srcBuffer, dstBuffer, ...);
device.endSingleTimeCommands(cmd);
```

---

## Destruction Order

```cpp
Device::~Device() {
    vkDestroyCommandPool(device_, commandPool, nullptr);         // 6
    vkDestroyDevice(device_, nullptr);                           // 5

    if (enableValidationLayers) {
        DestroyDebugUtilsMessengerEXT(instance, debugMessenger, nullptr);  // 2
    }

    vkDestroySurfaceKHR(instance, surface_, nullptr);            // 3
    vkDestroyInstance(instance, nullptr);                        // 1
}
```

**Critical Order:**
1. Destroy child objects first (command pool, device)
2. Destroy parent objects last (surface, instance)

**Common Mistake:**
```cpp
// WRONG - Will cause validation errors
vkDestroyInstance(instance, nullptr);    // ← Instance destroyed
vkDestroyDevice(device_, nullptr);       // ← Device needs instance!
```

---

## Related Documentation

- [Window Component](WINDOW.md) - Provides surface
- [SwapChain Component](SWAPCHAIN.md) - Uses device and queues
- [Architecture Overview](ARCHITECTURE.md) - Device's role

---

**Next Component:** [SwapChain Component Documentation](SWAPCHAIN.md)