#pragma once

#include "../gl.h"

#include <string>
#include <iostream>
#include <fstream>
#include <map>

#include "../Constants.hpp"
#define FONT_PATH (BASE_PATH + "assets/fonts/")

#include "../Utils.hpp"
#include "../Draw.hpp"
#include "../Logger.hpp"

// American English
#define CENTER_X  1
#define CENTER_Y  2
#define CENTER_XY 3
#define CENTER_YX 3

// British English
#define CENTRE_X  1
#define CENTRE_Y  2
#define CENTRE_XY 3
#define CENTRE_YX 3

// Font Types
#define FONT_SMOOTH 0
#define FONT_SHARP  1

class Font {
	public:
		Font(std::string name, int fontType) : name(name) {
			Logger::log("Loading ", name);
			loadData(FONT_PATH + name + ".txt");
			loadTexture(FONT_PATH + name + ".png", fontType);
			Logger::log();
		}

		~Font() {
			for (auto &pair : characters)
				delete pair.second;

			characters.clear();
		}

		void write(std::string text, float startX, float startY, float fontSize) {
			Draw::useTexture(texture);

			bool reenableBlend = glIsEnabled(GL_BLEND);

			glEnable(GL_BLEND);
			glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

			Draw::begin(Draw::QUADS);

			double scale = (1.0 / base) * fontSize;

			double currentX = startX;
			double currentY = startY;

			double x, y;
			double width, height;

			float u0, v0, u1, v1;

			for (char character : text) {
				if (characters.count(character) == 0) {
					if (hasInvalidCharacter) character = char(33);
					else continue;
				}

				FontCharacter *fontCharacter = characters[character];

				width = fontCharacter->width * scale;
				height = fontCharacter->height * scale;

				x = currentX + fontCharacter->xOffset *  scale;
				y = currentY + fontCharacter->yOffset * -scale;

				u0 = fontCharacter->x / (double) textureWidth;
				v0 = fontCharacter->y / (double) textureHeight;
				u1 = u0 + fontCharacter->width / (double) textureWidth;
				v1 = v0 + fontCharacter->height / (double) textureHeight;

				Draw::texCoord(u0, v0); Draw::vertex(x, y);
				Draw::texCoord(u1, v0); Draw::vertex(x + width, y);
				Draw::texCoord(u1, v1); Draw::vertex(x + width, y - height);
				Draw::texCoord(u0, v1); Draw::vertex(x, y - height);

				currentX += fontCharacter->xAdvance * scale;
			}

			Draw::end();

			Draw::useTexture(0);
			if (!reenableBlend) glDisable(GL_BLEND);
		}

		double getTextWidth(std::string text, float fontSize) {
			const double scale = (1.0 / base) * fontSize;

			float textWidth = 0.0f;

			int characterIndex = 0;
			for (char character : text) {
				if (characters.count(character) == 0) {
					if (hasInvalidCharacter) character = char(33);
					else continue;
				}

				FontCharacter *fontCharacter = characters[character];

				if (characterIndex == text.length() - 1) {
					textWidth += fontCharacter->width * scale;
				} else {
					textWidth += fontCharacter->xAdvance * scale;
				}

				characterIndex++;
			}

			return textWidth;
		}

		void writeCentred(std::string text, float startX, float startY, float fontSize, unsigned int centreFlags) {
			const double scale = (1.0 / base) * fontSize;

			float textWidth = 0.0f;
			float textHeight = 0.0f;

			int characterIndex = 0;
			for (char character : text) {
				if (characters.count(character) == 0) {
					if (hasInvalidCharacter) character = char(33);
					else continue;
				}

				FontCharacter *fontCharacter = characters[character];

				if (characterIndex == text.length() - 1) {
					textWidth += fontCharacter->width * scale;
				} else {
					textWidth += fontCharacter->xAdvance * scale;
				}

				double charHeight = fontCharacter->height * scale;
				if (charHeight > textHeight) textHeight = charHeight;

				characterIndex++;
			}

			if (centreFlags & CENTRE_X) startX -= textWidth * 0.5;
			if (centreFlags & CENTRE_Y) startY += textHeight * 0.5;

			write(text, startX, startY, fontSize);
		}

		void writeCentered(std::string text, float startX, float startY, float fontSize, unsigned int centerFlags) {
			writeCentred(text, startX, startY, fontSize, centerFlags);
		}

	private:
		struct FontCharacter {
			// unsigned int id;
			unsigned int x;
			unsigned int y;
			unsigned int width;
			unsigned int height;
			int xOffset;
			int yOffset;
			int xAdvance;
			// int page;
			// int chnl;
		};

		void loadData(std::string path) {
			std::ifstream file;
			file.open(path);

			if (!file.is_open()) {
				Logger::logError("Failed to load font data for ", name);
				return;
			}

			std::string temp;
			std::getline(file, temp); // Info

			// Common
			std::getline(file, temp, ' ');
			std::getline(file, temp, ' ');
			std::getline(file, temp, ' ');
			base = std::stoi(temp.substr(5));
			std::getline(file, temp, 'W');
			std::getline(file, temp, ' ');
			textureWidth = std::stoi(temp.substr(1));
			std::getline(file, temp, 'H');
			std::getline(file, temp, ' ');
			textureHeight = std::stoi(temp.substr(1));
			std::getline(file, temp);

			std::getline(file, temp); // Page
			std::getline(file, temp); // Char count

			FontCharacter *current;

			int progress = 0;
			while ((progress == 10) ? std::getline(file, temp, '\n') : std::getline(file, temp, ' ')) {
				if (temp.length() == 0) continue;

				switch (progress) {
					case 0: // char
						if (temp == "kernings") {
							progress = -99;
							break;
						}

						current = new FontCharacter();
						break;
					case 1: // id=####
						characters[std::stoi(temp.substr(3))] = current;
						break;
					case 2: // x=###
						current->x = std::stoi(temp.substr(2));
						break;
					case 3: // y=###
						current->y = std::stoi(temp.substr(2));
						break;
					case 4: // width=##
						current->width = std::stoi(temp.substr(6));
						break;
					case 5: // height=###
						current->height = std::stoi(temp.substr(7));
						break;
					case 6: // xoffset=###
						current->xOffset = std::stoi(temp.substr(8));
						break;
					case 7: // yoffset=###
						current->yOffset = std::stoi(temp.substr(8));
						break;
					case 8: // xadvance=###
						current->xAdvance = std::stoi(temp.substr(9));
						break;
					case 9: // page=#
						break;
					case 10: // chnl=##
						progress = -1;
				}

				if (progress == -99) break;

				progress++;
			}

			hasInvalidCharacter = (characters.count(145) > 0);

			Logger::log(characters.size(), " characters loaded");
		}

		void loadTexture(std::string path, int fontType) {
			texture = ::loadTexture(path.c_str(), (fontType == FONT_SMOOTH) ? GL_LINEAR : GL_NEAREST);

			Logger::log("Texture loaded");
		}

		std::string name;
		GLuint texture;

		unsigned int textureWidth;
		unsigned int textureHeight;
		unsigned int base;
		std::map< unsigned int, FontCharacter* > characters;

		bool hasInvalidCharacter;
};