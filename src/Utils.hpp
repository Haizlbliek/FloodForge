#include <GLFW/glfw3.h>

#include <string>
#include <filesystem>

#include "math/Colour.hpp"

#ifndef UTILS_HPP
#define UTILS_HPP

void fillRect(float x0, float y0, float x1, float y1);

void strokeRect(float x0, float y0, float x1, float y1);

void drawLine(float x0, float y0, float x1, float y1, double thickness);

GLuint loadTexture(std::string filepath);

GLuint loadTexture(std::string filepath, int filter);

GLuint loadTexture(const char* filepath);

GLuint loadTexture(const char* filepath, int filter);

GLFWimage loadIcon(const char* filepath);

void saveImage(GLFWwindow* window, const char* fileName);

bool startsWith(const std::string& str, const std::string& prefix);

bool endsWith(const std::string &str, const std::string &suffix);

std::string toLower(const std::string &str);

std::string toUpper(const std::string &str);

std::string findFileCaseInsensitive(const std::string &directory, const std::string &fileName);


// GL functions
void glColour(Colour colour);

void glColor(Colour color);

#endif