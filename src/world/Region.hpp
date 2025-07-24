#pragma once

#include <string>
#include <filesystem>
#include <vector>
#include <unordered_map>
#include <utility>

#include "RoomAttractiveness.hpp"

class Region {
	public:
		std::string acronym;

		std::vector<std::pair<std::string, std::unordered_map<std::string, RoomAttractiveness>>> roomAttractiveness;

		std::string extraProperties;
		std::string extraWorld;
		std::string complicatedCreatures;
		std::string roomsDirectory;
		std::filesystem::path exportDirectory;
		
		void reset();
};