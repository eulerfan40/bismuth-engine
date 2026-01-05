# Utils Component

The Utils component provides common utility functions used throughout the Bismuth Engine. Currently, it focuses on hash combining functionality for creating composite hash values from multiple data fields.

## Overview

**Purpose:** Provide reusable utility functions that don't belong to any specific component.

**Key Responsibilities:**
- Hash combination for composite data structures
- Support for custom hash functions in STL containers

**Location:** `engine/src/Utils.hpp`

---

## Architecture

### Header Structure

```cpp
#pragma once
#include <functional>

namespace engine {
  template<typename T, typename... Rest>
  void hashCombine(std::size_t &seed, const T &v, const Rest &... rest);
}
```

**Design:** Header-only template utilities for maximum flexibility and zero runtime overhead.

---

## Hash Combining

### hashCombine()

```cpp
template<typename T, typename... Rest>
void hashCombine(std::size_t &seed, const T &v, const Rest &... rest) {
  seed ^= std::hash<T>{}(v) + 0x9e3779b9 + (seed << 6) + (seed >> 2);
  (hashCombine(seed, rest), ...);
}
```

**Purpose:** Combine multiple hash values into a single composite hash.

**Algorithm:** Uses the golden ratio constant (0x9e3779b9) to mix hash values, reducing collisions.

**Source:** Adapted from [Stack Overflow](https://stackoverflow.com/a/57595105) - a well-tested hash combining algorithm.

**Parameters:**
- `seed`: Reference to accumulator for the combined hash value (modified in-place)
- `v`: First value to hash and combine
- `rest`: Variadic template pack - additional values to hash and combine (optional)

**Usage Pattern:**
1. Initialize seed to 0
2. Call `hashCombine()` with seed and all fields
3. Use resulting seed as the hash value

**Example:**
```cpp
struct MyStruct {
    int a;
    float b;
    std::string c;
};

template <>
struct std::hash<MyStruct> {
    size_t operator()(const MyStruct& obj) const {
        size_t seed = 0;
        engine::hashCombine(seed, obj.a, obj.b, obj.c);
        return seed;
    }
};
```

**Why it works:**
- XOR (`^`) combines the new hash with the existing seed
- Magic constant `0x9e3779b9` ensures good bit distribution
- Bit shifts `(seed << 6)` and `(seed >> 2)` avalanche changes across all bits
- Variadic fold expression `(hashCombine(seed, rest), ...)` recursively combines remaining values

---

## Integration with STL

### Custom Hash Specialization

The `hashCombine()` function enables custom hash implementations for STL containers like `std::unordered_map` and `std::unordered_set`.

**Pattern:**
```cpp
// Define your struct
struct Vertex {
    glm::vec3 position;
    glm::vec3 color;
    glm::vec3 normal;
    glm::vec2 uv;
    
    bool operator==(const Vertex& other) const {
        return position == other.position && 
               color == other.color && 
               normal == other.normal && 
               uv == other.uv;
    }
};

// Specialize std::hash in the std namespace
namespace std {
    template <>
    struct hash<Vertex> {
        size_t operator()(const Vertex& vertex) const {
            size_t seed = 0;
            engine::hashCombine(seed, vertex.position, vertex.color, vertex.normal, vertex.uv);
            return seed;
        }
    };
}

// Now you can use Vertex as a key
std::unordered_map<Vertex, uint32_t> uniqueVertices;
```

**Requirements:**
1. The struct must implement `operator==()` for equality comparison
2. All fields used in the hash must be hashable (have `std::hash<T>` specializations)
3. Hash specialization must be defined in the `std` namespace

**Performance:** Hash combining is compile-time optimized for inline execution with zero function call overhead.

---

## Use Cases

### 1. Vertex Deduplication in Model Loading

When loading 3D models from OBJ files, vertices are often duplicated. Using `hashCombine()`, we can efficiently identify and merge duplicate vertices:

```cpp
std::unordered_map<Vertex, uint32_t> uniqueVertices{};

for (const auto& vertex : loadedVertices) {
    if (uniqueVertices.count(vertex) == 0) {
        uniqueVertices[vertex] = static_cast<uint32_t>(vertices.size());
        vertices.push_back(vertex);
    }
    indices.push_back(uniqueVertices[vertex]);
}
```

**Benefits:**
- Reduces memory usage (fewer unique vertices)
- Improves GPU cache efficiency
- Faster rendering with smaller buffers

**Typical savings:** 40-60% reduction in vertex count for models with shared vertices.

---

## Design Decisions

### Why Header-Only?

**Advantages:**
- No compilation unit needed
- Template instantiation at call site
- Complete inlining opportunities
- Easy to include anywhere

**Tradeoffs:**
- Increases compilation time slightly (negligible for small utilities)

### Why Variadic Templates?

**Flexibility:** Can hash any number of fields in a single call:
```cpp
hashCombine(seed, a);              // 1 field
hashCombine(seed, a, b);           // 2 fields
hashCombine(seed, a, b, c, d, e);  // 5 fields
```

**Type Safety:** Compile-time verification that all types are hashable.

**Performance:** Zero-overhead abstraction - expands to optimal code at compile time.

### Why This Algorithm?

The chosen hash combining algorithm balances:
- **Speed:** Simple XOR and bit shifts are extremely fast
- **Quality:** Golden ratio constant provides excellent distribution
- **Proven:** Widely used in production code (Boost, Stack Overflow)
- **Simplicity:** Easy to understand and maintain

**Alternatives considered:**
- Boost hash_combine: Same algorithm, but requires Boost dependency
- FNV hash: Slower but higher quality (overkill for this use case)
- Simple XOR: Poor distribution, high collision rate

---

## Common Issues and Troubleshooting

### Hash Specialization Not Found

**Symptom:** Compiler error: "use of deleted function std::hash<MyType>"

**Cause:** Missing `std::hash<MyType>` specialization.

**Solution:** Define the hash specialization in the `std` namespace before using the type as a key.

### GLM Types Not Hashable

**Symptom:** Compiler error when hashing `glm::vec3`, `glm::vec2`, etc.

**Cause:** GLM types don't have default `std::hash` specializations.

**Solution:** Include `<glm/gtx/hash.hpp>` to enable GLM type hashing:
```cpp
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/hash.hpp>
```

**Note:** This is required when using `hashCombine()` with GLM vectors.

### Missing operator==()

**Symptom:** Compiler error when using custom type in `std::unordered_map`

**Cause:** STL containers require both `std::hash<T>` and `operator==()`.

**Solution:** Implement equality comparison:
```cpp
bool operator==(const MyType& other) const {
    return field1 == other.field1 && field2 == other.field2;
}
```

---

## Future Enhancements

Potential additions to the Utils component:

### File I/O Utilities
- Path manipulation helpers
- Binary file reading
- Asset loading abstractions

### Math Utilities
- Common interpolation functions (lerp, slerp)
- Angle/radian conversions
- Random number generation

### String Utilities
- String splitting and parsing
- Format string helpers
- Case conversion

### Memory Utilities
- Alignment helpers
- Buffer management utilities
- Pool allocators

### Performance Utilities
- Timing/profiling macros
- Frame rate monitoring
- Performance counters

**Principle:** Only add utilities when they're used in multiple places - avoid speculative generalization.

---

## Related Documentation

- **[Model Component](MODEL.md)** - Primary user of `hashCombine()` for vertex deduplication
- **[Architecture Overview](ARCHITECTURE.md)** - Component overview
- **[Configuration](CONFIGURATION.md)** - Build settings and dependencies

---

## Summary

The Utils component provides foundational utility functions, starting with hash combining for efficient vertex deduplication in model loading. Its header-only, template-based design ensures zero overhead while maintaining flexibility. As the engine grows, additional common utilities will be added following the same principles: type-safe, performant, and easy to use.
