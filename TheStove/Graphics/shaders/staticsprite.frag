#version 330 core

out vec4 FragColor;
in vec2 TexCoord;

uniform sampler2D u_Texture;
uniform vec4 u_Color; 		// RGBA tint from GameObject

void main() {
    vec4 base = texture(u_Texture, TexCoord);

    // Discard nearly transparent pixels (alpha cutoff threshold can be adjusted)
    if(base.a < 0.1)
        discard;

    FragColor = base * u_Color;
}

