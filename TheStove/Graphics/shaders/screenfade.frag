#version 330 core

out vec4 FragColor;

uniform vec4 u_ColorTint; // expected to be (0,0,0, alpha)

void main()
{
    FragColor = u_ColorTint;
}