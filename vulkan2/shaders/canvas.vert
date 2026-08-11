#version 450

layout(location = 0) in vec2 inPos;   // pixel-space
layout(location = 1) in vec4 inColor;

layout(push_constant) uniform PushConstants {
   vec2 scale;   // 2 / screenWidth, 2 / screenHeight
   vec2 offset;  // -1, -1
} pc;

layout(location = 0) out vec4 fragColor;

void main() {
   vec2 ndc = inPos * pc.scale + pc.offset;
   gl_Position = vec4(ndc, 0.0, 1.0);
   fragColor = inColor;
}
