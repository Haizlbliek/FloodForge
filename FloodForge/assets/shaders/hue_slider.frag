#version 330 core

in vec2 uv;

out vec4 color;

uniform float hue;

vec3 hsv2rgb(float hue) {
	vec4 K = vec4(1.0, 2.0 / 3.0, 1.0 / 3.0, 3.0);
	vec3 p = abs(fract(vec3(hue) + K.xyz) * 6.0 - K.www);
	return clamp(p - K.xxx, 0.0, 1.0);
}

void main() {
	color = vec4(hsv2rgb(uv.y), 1.0 + hue * 0.001);
}
