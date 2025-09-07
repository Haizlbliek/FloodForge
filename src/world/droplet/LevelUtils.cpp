#include "LevelUtils.hpp"

#include <fstream>

void LevelUtils::createLevelFiles(std::filesystem::path directory, std::string roomName, int width, int height, bool fillLayer1, bool fillLayer2) {
	std::ofstream geo(directory / (roomName + ".txt"));
	geo << roomName << "\n";
	geo << width << "*" << height << "|-1|0\n";
	geo << "0.0000*1.0000|0|0\n";
	geo << "-220,-50\n";
	geo << "Border: Passable\n";
	geo << "\n";
	geo << "\n";
	geo << "\n";
	geo << "\n";
	geo << "\n";
	geo << "\n";
	for (int y = 0; y < height; y++) {
		for (int x = 0; x < width; x++) {
			geo << (fillLayer1 ? "1|" : "0|");
		}
	}
	geo << "\n";
	geo << "A\n";
	for (int y = 0; y < height; y++) {
		for (int x = 0; x < width; x++) {
			geo << (fillLayer2 ? "1|" : "0|");
		}
	}
	geo << "\n";
	geo.close();
}