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
		Texture(const char *filepath, int filter) {
			int nrChannels;

			unsigned char* data = stbi_load(filepath, &width, &height, &nrChannels, 0);
			if (!data) {
				Logger::error("Failed to load texture: ", filepath);
				return;
			}

			glGenTextures(1, &textureID);
			glBindTexture(GL_TEXTURE_2D, textureID);

			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, filter);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, filter);

			GLenum format = nrChannels == 4 ? GL_RGBA : GL_RGB;
			glTexImage2D(GL_TEXTURE_2D, 0, format, width, height, 0, format, GL_UNSIGNED_BYTE, data);

			stbi_image_free(data);
		}

		Texture(std::filesystem::path filepath) : Texture(filepath.generic_u8string(), GL_NEAREST) {}

		Texture(std::filesystem::path filepath, int filter) : Texture(filepath.generic_u8string(), filter) {}

		Texture(const char *filepath) : Texture(filepath, GL_NEAREST) {}

		Texture(std::string filepath, int filter) : Texture(filepath.c_str(), filter) {}

		Texture(std::string filepath) : Texture(filepath.c_str(), GL_NEAREST) {}

		~Texture() {
			glDeleteTextures(1, &textureID);
		}

		GLuint ID() { return textureID; }

		unsigned int Width() { return width; }
		unsigned int Height() { return height; }

	private:
		GLuint textureID;

		int width;
		int height;
};