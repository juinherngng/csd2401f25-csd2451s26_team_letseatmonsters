#version 330 core

layout (location = 0) in vec3 aPos;
layout (location = 1) in vec2 aTexCoord;

// Per-instance model matrix (locations 2-5)
layout (location = 2) in mat4 instanceModel;

uniform mat4 u_View;
uniform mat4 u_Projection;

out vec2 TexCoord;

void main() {
    gl_Position = u_Projection * u_View * instanceModel * vec4(aPos, 1.0);
    TexCoord = aTexCoord;
}
