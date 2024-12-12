#version 330 core

in vec4 fragColour;
out vec4 color;

uniform vec4 tintColour;

void main() {
    color = (fragColour + fragColour * tintColour) * 0.5;
    color.a = tintColour.a;
}
