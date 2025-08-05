#include "Utils.hpp"
#include "Draw.hpp"
#include "Logger.hpp"

#include "stb_image.h"
#include "stb_image_write.h"
#include "utf8.h"

#include <iostream>
#include <fstream>
#include <sstream>
#include <cmath>
#include <algorithm>
#include <cctype>
#include <filesystem>
#include <cstring>

#ifndef M_PI
#define M_PI   3.141592653589
#endif
#ifndef M_PI_2
#define M_PI_2 1.570796326795
#endif

void fillRect(float x0, float y0, float x1, float y1) {
	Draw::begin(Draw::QUADS);
	Draw::vertex(x0, y0);
	Draw::vertex(x1, y0);
	Draw::vertex(x1, y1);
	Draw::vertex(x0, y1);
	Draw::end();
}

void textureRect(float x0, float y0, float x1, float y1) {
	Draw::begin(Draw::QUADS);
	Draw::texCoord(0, 0); Draw::vertex(x0, y0);
	Draw::texCoord(1, 0); Draw::vertex(x1, y0);
	Draw::texCoord(1, 1); Draw::vertex(x1, y1);
	Draw::texCoord(0, 1); Draw::vertex(x0, y1);
	Draw::end();
}

void strokeRect(float x0, float y0, float x1, float y1) {
	Draw::begin(Draw::LINE_LOOP);
	Draw::vertex(x0, y0);
	Draw::vertex(x1, y0);
	Draw::vertex(x1, y1);
	Draw::vertex(x0, y1);
	Draw::end();
}

void strokeRect(float x0, float y0, float x1, float y1, double thickness) {
	drawLine(x0, y0, x1, y0, thickness);
	drawLine(x1, y0, x1, y1, thickness);
	drawLine(x1, y1, x0, y1, thickness);
	drawLine(x0, y1, x0, y0, thickness);
}


void nineSlice(double x0, double y0, double x1, double y1, double thickness) {
	double t = 1.0 / 3.0;
	double f = 2.0 / 3.0;

	double xm0 = x0 + thickness;
	double xm1 = x1 - thickness;
	double ym0 = y0 - thickness;
	double ym1 = y1 + thickness;

	Draw::begin(Draw::QUADS);

	Draw::texCoord(0, 0); Draw::vertex(x0, y0);
	Draw::texCoord(t, 0); Draw::vertex(xm0, y0);
	Draw::texCoord(t, t); Draw::vertex(xm0, ym0);
	Draw::texCoord(0, t); Draw::vertex(x0, ym0);

	Draw::texCoord(1, 0); Draw::vertex(x1, y0);
	Draw::texCoord(f, 0); Draw::vertex(xm1, y0);
	Draw::texCoord(f, t); Draw::vertex(xm1, ym0);
	Draw::texCoord(1, t); Draw::vertex(x1, ym0);

	Draw::texCoord(0, 1); Draw::vertex(x0, y1);
	Draw::texCoord(t, 1); Draw::vertex(xm0, y1);
	Draw::texCoord(t, f); Draw::vertex(xm0, ym1);
	Draw::texCoord(0, f); Draw::vertex(x0, ym1);

	Draw::texCoord(1, 1); Draw::vertex(x1, y1);
	Draw::texCoord(f, 1); Draw::vertex(xm1, y1);
	Draw::texCoord(f, f); Draw::vertex(xm1, ym1);
	Draw::texCoord(1, f); Draw::vertex(x1, ym1);

	
	Draw::texCoord(t, 0); Draw::vertex(xm0, y0);
	Draw::texCoord(f, 0); Draw::vertex(xm1, y0);
	Draw::texCoord(f, t); Draw::vertex(xm1, ym0);
	Draw::texCoord(t, t); Draw::vertex(xm0, ym0);

	Draw::texCoord(t, 1); Draw::vertex(xm0, y1);
	Draw::texCoord(f, 1); Draw::vertex(xm1, y1);
	Draw::texCoord(f, f); Draw::vertex(xm1, ym1);
	Draw::texCoord(t, f); Draw::vertex(xm0, ym1);

	Draw::texCoord(0, t); Draw::vertex(x0, ym0);
	Draw::texCoord(t, t); Draw::vertex(xm0, ym0);
	Draw::texCoord(t, f); Draw::vertex(xm0, ym1);
	Draw::texCoord(0, f); Draw::vertex(x0, ym1);

	Draw::texCoord(f, t); Draw::vertex(xm1, ym0);
	Draw::texCoord(1, t); Draw::vertex(x1, ym0);
	Draw::texCoord(1, f); Draw::vertex(x1, ym1);
	Draw::texCoord(f, f); Draw::vertex(xm1, ym1);


	Draw::texCoord(t, t); Draw::vertex(xm0, ym0);
	Draw::texCoord(f, t); Draw::vertex(xm1, ym0);
	Draw::texCoord(f, f); Draw::vertex(xm1, ym1);
	Draw::texCoord(t, f); Draw::vertex(xm0, ym1);

	Draw::end();
}

void drawLine(float x0, float y0, float x1, float y1, double thickness) {
	thickness /= 64.0;

	double angle = atan2(y1 - y0, x1 - x0);

	float a0x = x0 + cos(angle - M_PI_2) * thickness;
	float a0y = y0 + sin(angle - M_PI_2) * thickness;
	float b0x = x0 + cos(angle + M_PI_2) * thickness;
	float b0y = y0 + sin(angle + M_PI_2) * thickness;
	float a1x = x1 + cos(angle - M_PI_2) * thickness;
	float a1y = y1 + sin(angle - M_PI_2) * thickness;
	float b1x = x1 + cos(angle + M_PI_2) * thickness;
	float b1y = y1 + sin(angle + M_PI_2) * thickness;

	float c0x = x0 + cos(angle + M_PI) * thickness;
	float c0y = y0 + sin(angle + M_PI) * thickness;
	float c1x = x1 + cos(angle) * thickness;
	float c1y = y1 + sin(angle) * thickness;

	Draw::begin(Draw::TRIANGLES);

	Draw::vertex(a0x, a0y);
	Draw::vertex(a1x, a1y);
	Draw::vertex(b0x, b0y);

	Draw::vertex(a1x, a1y);
	Draw::vertex(b1x, b1y);
	Draw::vertex(b0x, b0y);

	Draw::vertex(a0x, a0y);
	Draw::vertex(b0x, b0y);
	Draw::vertex(c0x, c0y);

	Draw::vertex(a1x, a1y);
	Draw::vertex(b1x, b1y);
	Draw::vertex(c1x, c1y);

	Draw::end();
}

GLuint loadTexture(std::filesystem::path filepath) {
	return loadTexture(filepath.string().c_str(), GL_NEAREST);
}

GLuint loadTexture(std::filesystem::path filepath, int filter) {
	return loadTexture(filepath.string().c_str(), filter);
}

GLuint loadTexture(std::string filepath) {
	return loadTexture(filepath.c_str(), GL_NEAREST);
}

GLuint loadTexture(std::string filepath, int filter) {
	return loadTexture(filepath.c_str(), filter);
}

GLuint loadTexture(const char *filepath) {
	return loadTexture(filepath, GL_NEAREST);
}

GLuint loadTexture(const char *filepath, int filter) {
	int width, height, nrChannels;

	unsigned char* data = stbi_load(filepath, &width, &height, &nrChannels, 0);
	if (!data) {
		Logger::logError("Failed to load texture: ", filepath);
		return 0;
	}

	GLuint textureID;
	glGenTextures(1, &textureID);
	glBindTexture(GL_TEXTURE_2D, textureID);

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, filter);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, filter);

	GLenum format = nrChannels == 4 ? GL_RGBA : GL_RGB;
	glTexImage2D(GL_TEXTURE_2D, 0, format, width, height, 0, format, GL_UNSIGNED_BYTE, data);
	
	glBindTexture(GL_TEXTURE_2D, 0);

	stbi_image_free(data);

	return textureID;
}

GLFWimage loadIcon(const char* filepath) {
	int width, height, nrChannels;
	unsigned char* data = stbi_load(filepath, &width, &height, &nrChannels, 0);
	if (!data) {
		Logger::logError("Failed to load texture: ", filepath);
		return GLFWimage();
	}

	GLFWimage icon;

	icon.width = width;
	icon.height = height;
	icon.pixels = data;

	return icon;
}

void saveImage(GLFWwindow *window, const char *fileName) {
	int width, height;
	glfwGetFramebufferSize(window, &width, &height);

	glPixelStorei(GL_PACK_ALIGNMENT, 1);

	unsigned char* pixels = new unsigned char[3 * width * height];
	glReadPixels(0, 0, width, height, GL_RGB, GL_UNSIGNED_BYTE, pixels);

	unsigned char* flippedPixels = new unsigned char[3 * width * height];
	for (int i = 0; i < height; ++i) {
		memcpy(flippedPixels + i * 3 * width, pixels + (height - i - 1) * 3 * width, 3 * width);
	}

	stbi_write_png(fileName, width, height, 3, flippedPixels, width * 3);

	delete[] pixels;
	delete[] flippedPixels;
}

bool startsWith(const std::string &str, const std::string &prefix) {
	if (prefix.size() > str.size()) {
		return false;
	}
	return str.compare(0, prefix.size(), prefix) == 0;
}

bool endsWith(const std::string &str, const std::string &suffix) {
	if (suffix.size() > str.size()) {
		return false;
	}
	return str.compare(str.size() - suffix.size(), suffix.size(), suffix) == 0;
}

std::string toLower(const std::string &str) {
	std::string output;
	utf8::iterator<std::string::const_iterator> it(str.begin(), str.begin(), str.end());
	utf8::iterator<std::string::const_iterator> end(str.end(), str.begin(), str.end());

	for (; it != end; ++it) {
		char32_t cp = *it;
		
		if (cp <= 0x7F) {
			cp = std::tolower(static_cast<unsigned char>(cp));
		}
		
		utf8::append(cp, std::back_inserter(output));
	}

	return output;
}

std::string toUpper(const std::string &str) {
	std::string output = str;
	std::transform(output.begin(), output.end(), output.begin(), ::toupper);

	return output;
}

std::filesystem::path findDirectoryCaseInsensitive(const std::string &directory, const std::string &fileName) {
	for (const auto &entry : std::filesystem::directory_iterator(directory)) {
		if (entry.is_directory()) {
			const std::string entryFileName = entry.path().filename().generic_u8string();

			if (toLower(entryFileName) == toLower(fileName)) {
				return entry.path();
			}
		}
	}

	return "";
}

std::string findFileCaseInsensitive(const std::string &directory, const std::string &fileName) {
	for (const auto &entry : std::filesystem::directory_iterator(directory)) {
		if (entry.is_regular_file()) {
			const std::string entryFileName = entry.path().filename().generic_u8string();
			if (toLower(entryFileName) == toLower(fileName)) {
				return entry.path().generic_u8string();
			}
		}
	}
	return "";
}

void applyFrustumToOrthographic(Vector2 position, float rotation, Vector2 scale, float left, float right, float bottom, float top, float nearVal, float farVal) {
	left *= scale.x;
	right *= scale.x;
	bottom *= scale.y;
	top *= scale.y;

	left += position.x;
	right += position.x;
	bottom += position.y;
	top += position.y;

	float cosRot = std::cos(rotation);
	float sinRot = std::sin(rotation);

	GLfloat rotationMatrix[16] = {
		cosRot,  sinRot, 0, 0,
		-sinRot, cosRot, 0, 0,
		0,       0,      1, 0,
		0,       0,      0, 1
	};

	Draw::matrixMode(Draw::PROJECTION);
	Draw::loadIdentity();
	Draw::ortho(left, right, bottom, top, nearVal, farVal);

	Draw::multMatrix(Matrix4(rotationMatrix));
}

void applyFrustumToOrthographic(Vector2 position, float rotation, Vector2 scale) {
	applyFrustumToOrthographic(position, rotation, scale, -1.0f, 1.0f, -1.0f, 1.0f, 0.000f, 100.0f);
}

Vector2 bezierCubic(double t, Vector2 p0, Vector2 p1, Vector2 p2, Vector2 p3) {
	double u = 1 - t;
	double tt = t * t;
	double uu = u * u;
	double uuu = uu * u;
	double ttt = tt * t;

	Vector2 p;
	p = uuu * p0;         // (1 - t)^3 * P0
	p += 3 * uu * t * p1; // 3(1 - t)^2 * t * P1
	p += 3 * u * tt * p2; // 3(1 - t) * t^2 * P2
	p += ttt * p3;        // t^3 * P3

	return p;
}

double lineDistance(Vector2 vector, Vector2 pointA, Vector2 pointB) {
	Vector2 AB = pointB - pointA;
	Vector2 AP = vector - pointA;
	double lengthSqrAB = AB.x * AB.x + AB.y * AB.y;
	double t = (AP.x * AB.x + AP.y * AB.y) / lengthSqrAB;

	if (t < 0.0) t = 0.0;
	if (t > 1.0) t = 1.0;

	Vector2 closestPoint = pointA + t * AB;

	return closestPoint.distanceTo(vector);
}

std::vector<std::string> split(const std::string &text, char delimiter) {
	std::vector<std::string> tokens;
	std::string token;
	std::istringstream tokenStream(text);

	while (std::getline(tokenStream, token, delimiter)) {
		token.erase(0, token.find_first_not_of(" \t\n"));
		token.erase(token.find_last_not_of(" \t\n") + 1);
		tokens.push_back(token);
	}

	return tokens;
}

#ifdef _WIN32
#include <windows.h>
#include <shellapi.h>
void openURL(std::string url) {
	ShellExecuteA(nullptr, nullptr, url.c_str(), nullptr, nullptr, SW_SHOWNORMAL);
}
#endif
#ifdef __linux__
#include <cstdlib>
void openURL(std::string url) {
	std::string command = "xdg-open " + url + " &";
	int __ = std::system(command.c_str());
}
#endif


std::string loadShaderSource(const char* filePath) {
	std::ifstream shaderFile(filePath);
	if (!shaderFile.is_open()) {
		Logger::logError("Failed to open shader file: ", filePath);
		return "";
	}

	std::stringstream buffer;
	buffer << shaderFile.rdbuf();
	return buffer.str();
}

GLuint compileShader(const std::string& source, GLenum shaderType) {
	GLuint shader = glCreateShader(shaderType);
	const char* src = source.c_str();
	glShaderSource(shader, 1, &src, nullptr);
	glCompileShader(shader);

	GLint success;
	glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
	if (!success) {
		GLint logLength;
		glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &logLength);
		char* log = new char[logLength];
		glGetShaderInfoLog(shader, logLength, &logLength, log);
		Logger::logError("Shader compilation failed: ", log);
		delete[] log;
	}

	return shader;
}

GLuint linkShaders(GLuint vertexShader, GLuint fragmentShader) {
	GLuint program = glCreateProgram();
	glAttachShader(program, vertexShader);
	glAttachShader(program, fragmentShader);
	glLinkProgram(program);

	GLint success;
	glGetProgramiv(program, GL_LINK_STATUS, &success);
	if (!success) {
		GLint logLength;
		glGetProgramiv(program, GL_INFO_LOG_LENGTH, &logLength);
		char* log = new char[logLength];
		glGetProgramInfoLog(program, logLength, &logLength, log);
		Logger::logError("Program linking failed: ", log);
		delete[] log;
	}

	return program;
}

GLuint loadShaders(const char* vertexPath, const char* fragmentPath) {
	std::string vertexSource = loadShaderSource(vertexPath);
	std::string fragmentSource = loadShaderSource(fragmentPath);

	GLuint vertexShader = compileShader(vertexSource, GL_VERTEX_SHADER);
	GLuint fragmentShader = compileShader(fragmentSource, GL_FRAGMENT_SHADER);

	GLuint shaderProgram = linkShaders(vertexShader, fragmentShader);

	glDeleteShader(vertexShader);
	glDeleteShader(fragmentShader);

	return shaderProgram;
}

void replaceLastInstance(std::string& str, const std::string& old_sub, const std::string& new_sub) {
	size_t pos = str.rfind(old_sub);
	
	if (pos != std::string::npos) {
		str.replace(pos, old_sub.length(), new_sub);
	}
}

char parseCharacter(char character, bool shiftPressed) {
	if (!shiftPressed) return std::tolower(character);
	
	switch (character) {
		case '1': return '!';
		case '2': return '@';
		case '3': return '#';
		case '4': return '$';
		case '5': return '%';
		case '6': return '^';
		case '7': return '&';
		case '8': return '*';
		case '9': return '(';
		case '0': return ')';
		case '`': return '~';
		case '-': return '_';
		case '=': return '+';
		case '[': return '{';
		case ']': return '}';
		case ';': return ':';
		case '\'': return '"';
		case '\\': return '|';
		case ',': return '<';
		case '.': return '>';
		case '/': return '?';
	}

	return std::toupper(character);
}