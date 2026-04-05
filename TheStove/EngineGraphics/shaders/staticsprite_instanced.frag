#version 330 core

out vec4 FragColor;

in vec2 TexCoord;

uniform sampler2D u_Texture;
uniform vec4 u_Color = vec4(1.0, 1.0, 1.0, 1.0);

void main() {
    vec4 texColor = texture(u_Texture, TexCoord) * u_Color;

    // Discard nearly transparent pixels (alpha cutoff threshold can be adjusted)
    if(texColor.a < 0.1)
        discard;

    FragColor = texColor;
}

