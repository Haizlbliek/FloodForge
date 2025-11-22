#pragma once

#include <string>
#include <filesystem>
#include <vector>
#include <unordered_map>
#include <utility>
#include <map>

#include "../math/Color.hpp"
#include "RoomAttractiveness.hpp"

class Region {
	public:
		std::string acronym;

		std::string extraProperties;
		std::string extraWorld;
		std::string extraMap;
		std::string complicatedCreatures;

		std::filesystem::path roomsDirectory;
		std::filesystem::path exportDirectory;

		std::unordered_map<std::string, RoomAttractiveness> defaultAttractiveness;
		std::map<int, Color> overrideSubregionColors;

		void reset();
};