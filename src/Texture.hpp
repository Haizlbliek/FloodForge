#pragma once

#include <iostream>
#include <string>
#include <filesystem>

#include "gl.h"
#include "stb_image.h"
#include "stb_image_write.h"
#include "Logger.hpp"

class Texture {
	public:
		Texture(const char *filepath, int filter);

		Texture(std::filesystem::path filepath) : Texture(filepath.generic_u8string(), GL_NEAREST) {}

		Texture(std::filesystem::path filepath, int filter) : Texture(filepath.generic_u8string(), filter) {}

		Texture(const char *filepath) : Texture(filepath, GL_NEAREST) {}

		Texture(std::string filepath, int filter) : Texture(filepath.c_str(), filter) {}

		Texture(std::string filepath) : Texture(filepath.c_str(), GL_NEAREST) {}

		~Texture();

		GLuint ID() { return textureID; }

		unsigned int Width() { return width; }
		unsigned int Height() { return height; }

	private:
		GLuint textureID;

		int width;
		int height;
};