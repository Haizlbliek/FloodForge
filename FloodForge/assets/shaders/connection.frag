#version 330 core

in vec4 fragColour;
out vec4 color;

uniform vec4 tintColor;
uniform vec4 tintColorB;
uniform float widthClip;
uniform float fadeMiddle;

vec3 lerp(vec3 a, vec3 b, float t) {
	return (b - a) * t + a;
}

float lerpf(float a, float b, float t) {
	return (b - a) * t + a;
}

void main() {
	color.rgb = lerp(tintColor.rgb, tintColorB.rgb, fragColour.r);

	float centerDist = abs((fragColour.g * 2) - 1);
	if (centerDist > widthClip) {
		color.r = 1.0; // so it's visible that something's wrong with alpha
		color.a = 0.0;
	}
	else {
		float middleFader = abs((fragColour.r * 2) - 1);
		if (fadeMiddle == 0)
			middleFader = 1.0;
		color.a = lerpf(tintColor.a, tintColorB.a, fragColour.r) * middleFader;
	}
}
