#include "CreatureTextures.hpp"

#include <fstream>
#include <algorithm>

std::unordered_map<std::string, GLuint> CreatureTextures::creatureTextures;
std::unordered_map<std::string, GLuint> CreatureTextures::creatureTagTextures;
std::vector<std::string> CreatureTextures::creatures;
std::vector<std::string> CreatureTextures::creatureTags;
std::unordered_map<std::string, std::string> CreatureTextures::parseMap;
GLuint CreatureTextures::UNKNOWN = 0;

bool validExtension(std::string extension) {
	return extension == ".png";
}

void CreatureTextures::loadCreaturesFromFolder(std::string path, bool include) {
	loadCreaturesFromFolder(path, "", include);
}

void CreatureTextures::loadCreaturesFromFolder(std::string path, std::string prefix, bool include) {
	for (const auto& entry : std::filesystem::directory_iterator(path)) {
		if (std::filesystem::is_regular_file(entry.path()) && validExtension(entry.path().extension().string())) {
			std::string creature = prefix + entry.path().stem().string();
			if (include) creatures.push_back(creature);
			creatureTextures[creature] = loadTexture(entry.path().string());
		}
	}
}

void CreatureTextures::init() {
	std::fstream modsFile("assets/creatures/mods.txt");
	if (!modsFile.is_open()) return;
	
	std::vector<std::string> mods;

	std::string line;
	while (std::getline(modsFile, line)) {
		if (line.empty()) continue;
		
		mods.push_back(line);
	}
	
	modsFile.close();

	loadCreaturesFromFolder("assets/creatures/", true);
	for (std::string mod : mods) {
		loadCreaturesFromFolder("assets/creatures/" + mod, true);
	}
	loadCreaturesFromFolder("assets/creatures/room/", "room-", false);
	
	for (const auto& entry : std::filesystem::directory_iterator("assets/creatures/TAGS/")) {
		if (std::filesystem::is_regular_file(entry.path()) && validExtension(entry.path().extension().string())) {
			std::string tag = entry.path().stem().string();
			creatureTags.push_back(tag);
			creatureTagTextures[tag] = loadTexture(entry.path().string());
		}
	}

	auto CLEAR_it = std::find(creatures.begin(), creatures.end(), "CLEAR");
	if (CLEAR_it != creatures.end()) {
		std::swap(*CLEAR_it, *(creatures.begin()));
	}

	auto UNKNOWN_it = std::find(creatures.begin(), creatures.end(), "UNKNOWN");
	UNKNOWN = creatureTextures["UNKNOWN"];
	if (UNKNOWN_it != creatures.end()) {
		std::swap(*UNKNOWN_it, *(creatures.end() - 1));
	}

	std::fstream parseFile("assets/creatures/parse.txt");
	if (!parseFile.is_open()) return;

	while (std::getline(parseFile, line)) {
		std::string from = line.substr(0, line.find_first_of(">"));
		std::string to = line.substr(line.find_first_of(">") + 1);

		parseMap[from] = to;
	}
	
	parseFile.close();
}

GLuint CreatureTextures::getTexture(std::string type) {
	if (type == "") return 0;

	if (creatureTagTextures.find(type) != creatureTagTextures.end()) {
		return creatureTagTextures[type];
	}

	if (creatureTextures.find(type) == creatureTextures.end()) {
		return creatureTextures["UNKNOWN"];
	}

	return creatureTextures[type];
}

std::string CreatureTextures::parse(std::string originalName) {
	if (parseMap.find(originalName) == parseMap.end()) {
		return originalName;
	}

	return parseMap[originalName];
}

bool CreatureTextures::known(std::string type) {
	if (type == "") return true;

	return creatureTextures.find(parse(type)) != creatureTextures.end();
}