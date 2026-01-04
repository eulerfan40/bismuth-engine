#version 460

layout(location = 0) in vec2 position;

// Executed once per vertex we have
void main () {
  //gl_Position is the output position in clip coordinates (x: -1 (left) - (right) 1, y: -1 (up) - (down) 1)
  gl_Position = vec4(position, 0.0, 1.0);
}