#pragma once

#include <unordered_map>
#include <vector>
#include <string>
#include <filesystem>
#include "../gl.h"

#include "../Utils.hpp"

#define CREATURE_ROWS 7

namespace CreatureTextures {
	extern std::unordered_map<std::string, GLuint> creatureTextures;
	extern std::unordered_map<std::string, GLuint> creatureTagTextures;
	extern std::vector<std::string> creatures;
	extern std::vector<std::string> creatureTags;
	extern std::unordered_map<std::string, std::string> parseMap;
	extern std::vector<std::string> creatureOrder;
	
	extern GLuint UNKNOWN;

	void loadCreaturesFromFolder(std::filesystem::path path, bool include);
	void loadCreaturesFromFolder(std::filesystem::path path, std::string prefix, bool include);

	GLuint getTexture(std::string type, bool lowercase = true);

	void init();

	std::string parse(std::string originalName);

	bool known(std::string type);
};