#version 330 core

layout(location = 0) in vec3 aPos;
layout(location = 1) in vec2 aTexCoord;
layout(location = 2) in vec4 aColor;

out vec2 uv;
out vec4 fragColor;

uniform mat4 model;
uniform mat4 projection;

void main() {
	gl_Position = projection * model * vec4(aPos, 1.0);
	uv = aTexCoord;
	fragColor = aColor;
}