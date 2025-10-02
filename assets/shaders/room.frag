#version 330 core

in vec4 fragColour;
out vec4 color;

uniform vec4 tintColour;
uniform float tintStrength;

vec3 lerp(vec3 a, vec3 b, float t) {
	return (b - a) * t + a;
}

void main() {
	color.rgb = lerp(fragColour.rgb, fragColour.rgb * tintColour.rgb, tintStrength);
	color.a = tintColour.a;
}
