#version 330 core

in vec2 vUV;
out vec4 FragColor;

uniform vec4 u_ColorTint;   // rgb ignored, a = opacity
uniform vec2 u_UVScale;     // reuse existing setter; use (1, sizeY/sizeX) to shape ellipse

void main()
{
	// Move to center and normalize to [-1,1]
	vec2 p = (vUV - vec2(0.5)) / vec2(0.5);

	// Shape into ellipse by scaling Y (X kept at 1)
	p.y /= max(u_UVScale.y, 0.0001);

	// Soft circular falloff (1 at center, 0 at radius >= 1)
	float r = length(p);
	float falloff = smoothstep(1.0, 0.0, r);  // inner = 1, edge = 0
	falloff = falloff * falloff;              // slightly sharper center

	float alpha = clamp(u_ColorTint.a, 0.0, 1.0) * falloff;
	FragColor = vec4(0.0, 0.0, 0.0, alpha);
}