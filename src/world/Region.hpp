#pragma once

#include <string>
#include <filesystem>
#include <vector>
#include <unordered_map>
#include <utility>
#include <map>

#include "../math/Colour.hpp"
#include "RoomAttractiveness.hpp"

class Region {
	public:
		std::string acronym;

		std::vector<std::pair<std::string, std::unordered_map<std::string, RoomAttractiveness>>> roomAttractiveness;

		std::string extraProperties;
		std::string extraWorld;
		std::string extraMap;
		std::string complicatedCreatures;

		std::filesystem::path roomsDirectory;
		std::filesystem::path exportDirectory;

		std::map<int, Colour> overrideSubregionColors;

		void reset();
};