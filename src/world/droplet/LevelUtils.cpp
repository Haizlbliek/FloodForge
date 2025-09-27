#include "LevelUtils.hpp"

#include <fstream>

void LevelUtils::createLevelFiles(std::filesystem::path directory, std::string roomName, int width, int height, bool fillLayer1, bool fillLayer2, bool createCameras) {
	std::ofstream geo(directory / (roomName + ".txt"));
	geo << roomName << "\n";
	geo << width << "*" << height << "|-1|0\n";
	geo << "0.0000*1.0000|0|0\n";
	if (createCameras) {
		int screenWidth = (int) std::round((width + 4.0) / 52.0);
		int screenHeight = (int) std::round((height + 5.0) / 40.0);
		for (int y = 0; y < screenHeight; y++) {
			for (int x = 0; x < screenWidth; x++) {
				if (x != 0 || y != 0) geo << "|";
				geo << std::to_string(-212 + x * 1024) << "," << std::to_string(-34 + 768 * y);
			}
		}

		if (screenHeight <= 0 && screenWidth <= 0) {
			geo << "-212,-34\n";
		} else {
			geo << "\n";
		}
	} else {
		geo << "-220,-50\n";
	}
	geo << "Border: Passable\n";
	geo << "\n";
	geo << "\n";
	geo << "\n";
	geo << "\n";
	geo << "0\n";
	geo << "\n";
	for (int y = 0; y < height; y++) {
		for (int x = 0; x < width; x++) {
			geo << (fillLayer1 ? "1|" : (fillLayer2 ? "0,6|" : "0|"));
		}
	}
	geo << "\n";
	geo.close();
}

std::vector<Vector2i> LevelUtils::line(int x0, int y0, int x1, int y1) {
	std::vector<Vector2i> points;

	int dx = abs(x1 - x0);
	int dy = abs(y1 - y0);
	int sx = (x0 < x1) ? 1 : -1;
	int sy = (y0 < y1) ? 1 : -1;
	int error = dx - dy;

	while (true) {
		points.push_back(Vector2i(x0, y0));

		if (x0 == x1 && y0 == y1) {
			break;
		}

		int e2 = 2 * error;
		if (e2 > -dy) {
			error -= dy;
			x0 += sx;
		}
		if (e2 < dx) {
			error += dx;
			y0 += sy;
		}
	}

	return points;
}