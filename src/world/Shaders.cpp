#include "Shaders.hpp"

#include <iostream>

#include "../Constants.hpp"
#include "../Utils.hpp"
#include "../Logger.hpp"

GLuint Shaders::roomShader = 0;
GLuint Shaders::hueSliderShader = 0;
GLuint Shaders::colorSquareShader = 0;

void Shaders::init() {
	Shaders::roomShader = loadShaders(BASE_PATH / "assets" / "shaders" / "room.vert", BASE_PATH / "assets" / "shaders" / "room.frag");
	Shaders::hueSliderShader = loadShaders(BASE_PATH / "assets" / "shaders" / "default.vert", BASE_PATH / "assets" / "shaders" / "hue_slider.frag");
	Shaders::colorSquareShader = loadShaders(BASE_PATH / "assets" / "shaders" / "default.vert", BASE_PATH / "assets" / "shaders" / "color_square.frag");

	Logger::info("Shaders initialized");
}

void Shaders::cleanup() {
	glDeleteProgram(Shaders::roomShader);
}