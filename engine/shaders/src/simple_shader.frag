#version 460

// Variable stores and RGBA output color that should be written to color attachment 0
layout (location = 0) out vec4 outColor;

void main() {
  outColor = vec4(1.0, 0.0, 0.0, 1.0);
}
