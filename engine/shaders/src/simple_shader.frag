#version 460

layout(location = 0) in vec3 fragColor;
// Variable stores and RGBA output color that should be written to color attachment 0
layout (location = 0) out vec4 outColor;

void main() {
  outColor = vec4(fragColor, 1.0);
}
