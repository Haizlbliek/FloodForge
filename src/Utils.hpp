#pragma once

#include "gl.h"

#include <string>
#include <filesystem>
#include <vector>

#include "math/Color.hpp"
#include "math/Vector.hpp"
#include "math/Rect.hpp"

#define LINE_NONE 0
#define LINE_START 1
#define LINE_END 2
#define LINE_BOTH 4

void fillRect(Rect rect);
void fillRect(UVRect rect);
void fillRect(float x0, float y0, float x1, float y1);

void strokeRect(Rect rect);
void strokeRect(Rect rect, double thickness);
void strokeRect(float x0, float y0, float x1, float y1);
void strokeRect(float x0, float y0, float x1, float y1, double thickness);

void drawLine(float x0, float y0, float x1, float y1);
void drawLine(float x0, float y0, float x1, float y1, double thickness, int fancyEnds = LINE_BOTH);

void fillCircle(float x, float y, float radius, int resolution);
void strokeCircle(float x, float y, float radius, int resolution);

void nineSlice(double x0, double y0, double x1, double y1, double thickness);

GLuint loadTexture(std::filesystem::path filepath);

GLuint loadTexture(std::filesystem::path filepath, int filter);

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

bool compareInsensitive(const std::string &a, const std::string &b);

std::filesystem::path findDirectoryCaseInsensitive(const std::filesystem::path &directory, const std::string &fileName);

std::filesystem::path findFileCaseInsensitive(const std::filesystem::path &directory, const std::string &fileName);

void applyFrustumToOrthographic(Vector2 position, float rotation, Vector2 scale, float left, float right, float bottom, float top, float nearVal, float farVal);

void applyFrustumToOrthographic(Vector2 position, float rotation, Vector2 scale);

Vector2 bezierCubic(double t, Vector2 p0, Vector2 p1, Vector2 p2, Vector2 p3);

double lineDistance(Vector2 vector, Vector2 pointA, Vector2 pointB);

std::vector<std::string> split(const std::string &text, std::string delimiter);

std::vector<std::string> split(const std::string &text, char delimiter);

void openURL(std::string url);

GLuint loadShaders(std::filesystem::path vertexPath, std::filesystem::path fragmentPath);

void replaceLastInstance(std::string& str, const std::string& old_sub, const std::string& new_sub);

char parseCharacter(char character, bool shiftPressed, bool capsPressed);

std::string toFixed(double x, int decimals);

Color stringToColor(const std::string &hex);

std::string colorToString(const Color &color);

double safeStod(const std::string &str, const std::string message);