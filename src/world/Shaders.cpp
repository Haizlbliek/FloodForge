#include "Shaders.hpp"

#include <iostream>

#include "../Constants.hpp"
#include "../Utils.hpp"
#include "../Logger.hpp"

GLuint Shaders::roomShader = 0;
GLuint Shaders::hueSliderShader = 0;
GLuint Shaders::colorSquareShader = 0;

void Shaders::init() {
	Logger::info("Loaded shaders");
	Shaders::roomShader = loadShaders(ASSETS_PATH / "shaders" / "room.vert", ASSETS_PATH / "shaders" / "room.frag");
	Shaders::hueSliderShader = loadShaders(ASSETS_PATH / "shaders" / "default.vert", ASSETS_PATH / "shaders" / "hue_slider.frag");
	Shaders::colorSquareShader = loadShaders(ASSETS_PATH / "shaders" / "default.vert", ASSETS_PATH / "shaders" / "color_square.frag");
	Logger::info("Shaders loaded");
	Logger::info();
}

void Shaders::cleanup() {
	glDeleteProgram(Shaders::roomShader);
}