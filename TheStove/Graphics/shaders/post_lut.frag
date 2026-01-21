#version 330 core
in vec2 vUV;
out vec4 FragColor;

uniform sampler2D u_SceneTex;
uniform sampler2D u_LUT;
uniform int   u_LUTSize    = 16;   // unused in 1D path
uniform float u_Intensity  = 1.0;

vec3 ApplyCurve(vec3 color) {
    // Sample a 1D curve (256x1) along U for each channel independently
    float r = texture(u_LUT, vec2(color.r, 0.5)).r;
    float g = texture(u_LUT, vec2(color.g, 0.5)).g;
    float b = texture(u_LUT, vec2(color.b, 0.5)).b;
    return vec3(r, g, b);
}

void main() {
    vec2 sceneUV = vec2(vUV.x, 1.0 - vUV.y); // flip if your FBO needs it
    vec4 src = texture(u_SceneTex, sceneUV);

    vec3 graded = ApplyCurve(src.rgb);
    vec3 result = mix(src.rgb, graded, clamp(u_Intensity, 0.0, 1.0));
    FragColor = vec4(result, src.a);
}