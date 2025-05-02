#pragma once

#include "gl.h"

#include <string>
#include <filesystem>
#include <vector>

#include "math/Colour.hpp"
#include "math/Vector.hpp"

void fillRect(float x0, float y0, float x1, float y1);

void textureRect(float x0, float y0, float x1, float y1);

void strokeRect(float x0, float y0, float x1, float y1);

void strokeRect(float x0, float y0, float x1, float y1, double thickness);

void drawLine(float x0, float y0, float x1, float y1, double thickness);

void nineSlice(double x0, double y0, double x1, double y1, double thickness);

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

void applyFrustumToOrthographic(Vector2 position, float rotation, Vector2 scale, float left, float right, float bottom, float top, float nearVal, float farVal);

void applyFrustumToOrthographic(Vector2 position, float rotation, Vector2 scale);

Vector2 bezierCubic(double t, Vector2 p0, Vector2 p1, Vector2 p2, Vector2 p3);

double lineDistance(Vector2 vector, Vector2 pointA, Vector2 pointB);

std::vector<std::string> split(const std::string &text, char delimiter);

void openURL(std::string url);

// GL functions
void glColour(Colour colour);

void glColor(Colour color);


GLuint loadShaders(const char* vertexPath, const char* fragmentPath);