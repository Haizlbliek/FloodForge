#pragma once

#include <filesystem>
#include <fstream>
#include <map>
#include <string>

#include "../Utils.hpp"
#include "../math/Quadruple.hpp"

#include "ExtraRoomData.hpp"
#include "Globals.hpp"

class WorldParser {
	public:
		static void importWorldFile(std::filesystem::path path);

		static void parseMap(std::filesystem::path mapFilePath, std::filesystem::path directory);

		static std::tuple<std::string, std::vector<std::string>, std::vector<std::string>> parseRoomString(const std::string &input);

		static void parseWorldRoom(std::string line, std::filesystem::path directory, std::vector<Quadruple<Room*, int, std::string, int>> &connectionsToAdd);

		static void parseWorldCreature(std::string line);

		static void parseWorld(std::filesystem::path worldFilePath, std::filesystem::path directory);

		static RoomAttractiveness parseRoomAttractiveness(std::string value);

		static void parseProperties(std::string propertiesFilePath);

		static void loadExtraRoomData(std::string roomPath, ExtraRoomData &data);
};
