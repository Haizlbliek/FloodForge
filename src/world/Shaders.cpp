#include "Shaders.hpp"

#include <iostream>

#include "../Constants.hpp"
#include "../Utils.hpp"

GLuint Shaders::roomShader = 0;

void Shaders::init() {
    Shaders::roomShader = loadShaders((BASE_PATH + "assets/shaders/room.vertex").c_str(), (BASE_PATH + "assets/shaders/room.frag").c_str());

    std::cout << "Shaders initialized" << std::endl;
}

void Shaders::cleanup() {
    glDeleteProgram(Shaders::roomShader);
}