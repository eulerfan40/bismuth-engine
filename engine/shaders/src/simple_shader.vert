#version 460

vec2 positions[3] = vec2[] (
vec2(0.0, -0.5),
vec2(0.5, 0.5),
vec2(-0.5, 0.5)
);

// Executed once per vertex we have
void main () {
  //gl_Position is the output position in clip coordinates (x: -1 (left) - (right) 1, y: -1 (up) - (down) 1)
  gl_Position = vec4(positions[gl_VertexIndex], 0.0, 1.0); // gl_VertexIndex stores the index of the current vertex
}