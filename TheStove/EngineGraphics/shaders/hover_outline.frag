#version 330 core

out vec4 FragColor;

in vec2 TexCoord;

uniform sampler2D u_Texture;
uniform vec4 u_Color = vec4(0.0, 1.0, 1.0, 1.0);

void main() {
    vec4 tex = texture(u_Texture, TexCoord);

    // Keep original alpha shape, ignore original RGB completely
    if (tex.a < 0.1)
        discard;

    FragColor = vec4(u_Color.rgb, tex.a * u_Color.a);
}