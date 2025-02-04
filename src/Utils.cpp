#include "Utils.hpp"
#include "Draw.hpp"

#include "stb_image.h"
#include "stb_image_write.h"

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
		std::cerr << "Failed to load texture: " << filepath << std::endl;
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
	// glGenerateMipmap(GL_TEXTURE_2D);
	
	glBindTexture(GL_TEXTURE_2D, 0);

	stbi_image_free(data);

	return textureID;
}

GLFWimage loadIcon(const char* filepath) {
	int width, height, nrChannels;
	unsigned char* data = stbi_load(filepath, &width, &height, &nrChannels, 0);
	if (!data) {
		std::cerr << "Failed to load texture: " << filepath << std::endl;
		return GLFWimage();
	}

	GLFWimage icon;

	icon.width = width;
	icon.height = height;
	icon.pixels = data;

	// Generate the texture
	// GLenum format = nrChannels == 4 ? GL_RGBA : GL_RGB;
	// glTexImage2D(GL_TEXTURE_2D, 0, format, width, height, 0, format, GL_UNSIGNED_BYTE, data);
	// glGenerateMipmap(GL_TEXTURE_2D);

	// Free the image data after uploading it to the GPU
	// stbi_image_free(data);

	return icon;
}

void saveImage(GLFWwindow *window, const char *fileName) {
	// Get the window size
	int width, height;
	glfwGetFramebufferSize(window, &width, &height); // Get the current framebuffer size

	glPixelStorei(GL_PACK_ALIGNMENT, 1);

	// Allocate memory for the pixel data
	unsigned char* pixels = new unsigned char[3 * width * height]; // 3 channels for RGB
	glReadPixels(0, 0, width, height, GL_RGB, GL_UNSIGNED_BYTE, pixels); // Read pixel data from the framebuffer

	// Flip the image because OpenGL reads it upside down
	unsigned char* flippedPixels = new unsigned char[3 * width * height];
	for (int i = 0; i < height; ++i) {
		memcpy(flippedPixels + i * 3 * width, pixels + (height - i - 1) * 3 * width, 3 * width);
	}

	// Write the flipped image data to a PNG file
	stbi_write_png(fileName, width, height, 3, flippedPixels, width * 3);

	// Clean up allocated memory
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
	std::string output = str;
	std::transform(output.begin(), output.end(), output.begin(), ::tolower);

	return output;
}

std::string toUpper(const std::string &str) {
	std::string output = str;
	std::transform(output.begin(), output.end(), output.begin(), ::toupper);

	return output;
}

std::string findFileCaseInsensitive(const std::string &directory, const std::string &fileName) {
	for (const auto &entry : std::filesystem::directory_iterator(directory)) {
		if (entry.is_regular_file()) {
			const std::string entryFileName = entry.path().filename().string();
			if (toLower(entryFileName) == toLower(fileName)) {
				return entry.path().string();
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




std::string loadShaderSource(const char* filePath) {
    std::ifstream shaderFile(filePath);
    if (!shaderFile.is_open()) {
        std::cerr << "Failed to open shader file: " << filePath << std::endl;
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
        std::cerr << "Shader compilation failed: " << log << std::endl;
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
        std::cerr << "Program linking failed: " << log << std::endl;
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
