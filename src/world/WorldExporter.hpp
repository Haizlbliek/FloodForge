#pragma once

#include <fstream>

#include "../math/Vector.hpp"
#include "../Logger.hpp"

#include "Globals.hpp"

class WorldExporter {
	public:
		static void exportMapFile();

		static void exportWorldFile();

		static void exportImageFile(std::filesystem::path outputPath, std::filesystem::path otherPath);

		static void exportPropertiesFile(std::filesystem::path outputPath);
};