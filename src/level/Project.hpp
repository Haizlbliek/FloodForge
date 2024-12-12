#ifndef PROJECT_HPP
#define PROJECT_HPP

#include "../stb_image.h"
#include "../stb_image_write.h"

#include <string>
#include <fstream>
#include <sstream>

#include "LevelUtils.hpp"
#include "Constants.hpp"
#include "../Utils.hpp"
#include "Grid.hpp"
#include "../Window.hpp"

class Project {
	public:
		Project(std::string name, unsigned int width, unsigned int height)
		 : name(name),
		   width(width),
		   height(height) {
		   	filePath = "";
			layer1 = new Grid(width, height);
			layer2 = new Grid(width, height);
			layer3 = new Grid(width, height);
		}

		~Project() {
			delete layer1;
			delete layer2;
			delete layer3;
		}

		void save();

		void exportDrizzle();

		void render();

		static Project *load(std::string name);
		static Project *loadFromPath(std::string path);

		Grid *GetLayer(unsigned int layer) {
			if (layer == 1) return layer1;
			if (layer == 2) return layer2;
			if (layer == 3) return layer3;

			return nullptr;
		}

		std::string Name() {
			return name;
		}

		bool validPath() {
			return filePath != "";
		}

		void resize(unsigned int width, unsigned int height) {
			this->width = width;
			this->height = height;
			layer1->resize(width, height);
			layer2->resize(width, height);
			layer3->resize(width, height);
		}

		void WindowC(Window *newWindow) { window = newWindow; }
		const Window *WindowC() { return window; }

	private:
		void renderCollision() {
			std::string levelData = "";
			levelData += name + "\n";
			levelData += std::to_string(layer1->Width()) + "*" + std::to_string(layer1->Height());
			if (false) {
				levelData += "|WATER_HEIGHT|IN_FRONT ? 1 : 0\n";
			} else {
				levelData += "\n";
			}
			levelData += "0.0000*1.0000|0|0\n"; // Unknown
			levelData += "0,0\n"; // Camera Data
			levelData += "Border: Passable\n"; // Border Type
			levelData += getObjectData() + "\n";
			levelData += "\n\n\n0\n\n"; // Unknown
			levelData += getTileData() + "\n";

			std::ofstream file;
			file.open(OUTPUT_PATH + name + ".txt");
			file << levelData;
			file.close();
		}

		std::string parseTile(unsigned int tile) {
			if (tile == 0) return "0"; // Air
			if (tile == 1) return "1"; // Solid
			if (tile == 2) return "3"; // One-way
			if (tile <= 6) return "2"; // Slopes
			// 7 and 8 never happen (poles)
			if (tile == 9) return "0"; // Spear
			if (tile == 10) return "0"; // Rock
			if (tile == 11) return "1"; // Border

			return "";
		}

		std::string getObjectData() {
			std::stringstream objectData;

			for (unsigned int x = 0; x < width; x++) {
				for (unsigned int y = 0; y < height; y++) {
					if (layer1->hasData(x, y, DATA_SPEAR)) objectData << "1," << x << "," << y << "|";
					if (layer1->hasData(x, y, DATA_ROCK)) objectData << "0," << x << "," << y << "|";
				}
			}

			return objectData.str();
		}

		std::string getTileData() {
			std::stringstream tileData;

			for (unsigned int x = 0; x < width; x++) {
				for (unsigned int y = 0; y < height; y++) {
					unsigned int tile = layer1->getTile(x, y);

					if (layer1->hasData(x, y, DATA_SHORTCUT)) {
						if (getShortcutTile(layer1, x, y) <= 3) {
							// Shortcut Enterance
							tileData << "4,3|";

							continue;
						}
					}

					tileData << parseTile(tile);

					for (const uint8_t &item : layer1->getData(x, y)) {
						if (item == DATA_SPEAR || item == DATA_ROCK) continue;

						tileData << "," << std::to_string(item);
					}

					tileData << "|";
				}
			}

			return tileData.str();
		}

		std::string name;

		unsigned int width;
		unsigned int height;

		Grid *layer1;
		Grid *layer2;
		Grid *layer3;

		std::string filePath;

		Window *window;
};

#endif