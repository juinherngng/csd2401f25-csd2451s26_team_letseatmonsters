#version 330 core

layout (location = 0) in vec3 aPos;
layout (location = 1) in vec2 aTexCoord;

out vec2 vUV;

void main() {
    // Fullscreen quad is defined in object space [-0.5 .. 0.5], expand to NDC [-1 .. 1]
    gl_Position = vec4(aPos.xy * 2.0, 0.0, 1.0);
    vUV = aTexCoord;
}