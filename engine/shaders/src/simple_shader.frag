#version 460

// Variable stores and RGBA output color that should be written to color attachment 0
layout (location = 0) out vec4 outColor;

layout(push_constant) uniform Push {
  mat2 transform;
  vec2 offset;
  vec3 color;
} push;

void main() {
  outColor = vec4(push.color, 1.0);
}
