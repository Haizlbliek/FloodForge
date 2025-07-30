#include "Shaders.hpp"

#include <iostream>

#include "../Constants.hpp"
#include "../Utils.hpp"
#include "../Logger.hpp"

GLuint Shaders::roomShader = 0;

void Shaders::init() {
	Shaders::roomShader = loadShaders((BASE_PATH / "assets" / "shaders" / "room.vertex").string().c_str(), (BASE_PATH / "assets" / "shaders" / "room.frag").string().c_str());

	Logger::log("Shaders initialized");
}

void Shaders::cleanup() {
	glDeleteProgram(Shaders::roomShader);
}