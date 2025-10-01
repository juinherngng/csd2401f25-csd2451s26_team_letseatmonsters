#version 330 core

out vec4 FragColor;

in vec2 TexCoord;

uniform sampler2D u_Texture;
uniform vec4 u_Color = vec4(1.0, 1.0, 1.0, 1.0);

void main() {
    FragColor = texture(u_Texture, TexCoord) * u_Color;
}
