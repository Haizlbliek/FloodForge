#version 330 core

in vec4 fragColor;
out vec4 color;

void main() {
    color = fragColor;
    color.a = 1.0;
}
