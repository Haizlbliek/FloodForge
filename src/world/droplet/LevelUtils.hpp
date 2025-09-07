#pragma once

#include <filesystem>
#include <string>

namespace LevelUtils {
	void createLevelFiles(std::filesystem::path directory, std::string roomName, int width, int height, bool fillLayer1, bool fillLayer2);
};