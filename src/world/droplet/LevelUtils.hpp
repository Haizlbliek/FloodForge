#pragma once

#include <filesystem>
#include <string>
#include <vector>

#include "../../math/Vector.hpp"

namespace LevelUtils {
	void createLevelFiles(std::filesystem::path directory, std::string roomName, int width, int height, bool fillLayer1, bool fillLayer2, bool createCameras);

	std::vector<Vector2i> line(int x0, int y0, int x1, int y1);
};