#version 330 core
out vec4 FragColor;

uniform vec4 u_ColorTint;

void main()
{
    FragColor = u_ColorTint;   // no textures, no mixing — just the tint you pass in
}