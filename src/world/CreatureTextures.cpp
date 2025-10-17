#include "CreatureTextures.hpp"

#include <fstream>
#include <algorithm>

#include "../Constants.hpp"
#include "../Logger.hpp"

std::unordered_map<std::string, GLuint> CreatureTextures::creatureTextures;
std::unordered_map<std::string, GLuint> CreatureTextures::creatureTagTextures;
std::vector<std::string> CreatureTextures::creatures;
std::vector<std::string> CreatureTextures::creatureTags;
std::unordered_map<std::string, std::string> CreatureTextures::parseMap;
std::vector<std::string> CreatureTextures::creatureOrder;
GLuint CreatureTextures::UNKNOWN = 0;

bool validExtension(std::string extension) {
	return extension == ".png";
}

void CreatureTextures::loadCreaturesFromFolder(std::filesystem::path path, bool include) {
	loadCreaturesFromFolder(path, "", include);

	if (!include) return;

	std::ifstream orderFile(path / "order.txt");
	if (!orderFile.is_open()) return;

	std::string line;
	while (std::getline(orderFile, line)) {
		if (line.empty()) continue;
		if (startsWith(line, "//")) continue;

		creatureOrder.push_back(toLower(line));
	}

	orderFile.close();
}

void CreatureTextures::loadCreaturesFromFolder(std::filesystem::path path, std::string prefix, bool include) {
	Logger::info("Loading creatures from: '", path.generic_u8string(), "'");

	for (const auto& entry : std::filesystem::directory_iterator(path)) {
		if (std::filesystem::is_regular_file(entry.path()) && validExtension(entry.path().extension().generic_u8string())) {
			std::string creature = toLower(prefix + entry.path().stem().generic_u8string());
			if (include) creatures.push_back(creature);
			creatureTextures[creature] = loadTexture(entry.path().generic_u8string());
		}
	}
}

void CreatureTextures::init() {
	std::filesystem::path creaturesDirectory = BASE_PATH / "assets" / "creatures";
	std::fstream modsFile(creaturesDirectory / "mods.txt");
	if (!modsFile.is_open()) return;
	
	std::vector<std::string> mods;

	std::string line;
	while (std::getline(modsFile, line)) {
		if (line.empty()) continue;

		if (line.back() == '\r') line.pop_back();
		
		mods.push_back(line);
	}
	
	modsFile.close();

	creatureOrder.push_back("clear");
	loadCreaturesFromFolder(creaturesDirectory, true);
	for (std::string mod : mods) {
		loadCreaturesFromFolder(creaturesDirectory / mod, true);
	}
	loadCreaturesFromFolder(creaturesDirectory / "room", "room-", false);
	
	for (const auto& entry : std::filesystem::directory_iterator(creaturesDirectory / "TAGS")) {
		if (std::filesystem::is_regular_file(entry.path()) && validExtension(entry.path().extension().generic_u8string())) {
			std::string tag = entry.path().stem().generic_u8string();
			creatureTags.push_back(tag);
			creatureTagTextures[tag] = loadTexture(entry.path().generic_u8string());
		}
	}

	auto CLEAR_it = std::find(creatures.begin(), creatures.end(), "clear");
	if (CLEAR_it != creatures.end()) {
		std::swap(*CLEAR_it, *(creatures.begin()));
	}

	auto UNKNOWN_it = std::find(creatures.begin(), creatures.end(), "unknown");
	UNKNOWN = creatureTextures["unknown"];
	if (UNKNOWN_it != creatures.end()) {
		std::swap(*UNKNOWN_it, *(creatures.end() - 1));
	}

	std::fstream parseFile(creaturesDirectory / "parse.txt");
	if (!parseFile.is_open()) return;

	while (std::getline(parseFile, line)) {
		std::string from = toLower(line.substr(0, line.find_first_of(">")));
		std::string to = toLower(line.substr(line.find_first_of(">") + 1));

		parseMap[from] = to;
	}
	
	parseFile.close();

	for (std::string creature : creatures) {
		if (creature == "clear" || creature == "unknown") continue;

		if (std::find(creatureOrder.begin(), creatureOrder.end(), creature) == creatureOrder.end()) {
			creatureOrder.push_back(creature);
		}
	}

	creatureOrder.push_back("unknown");
}

GLuint CreatureTextures::getTexture(std::string type) {
	if (type == "") return 0;

	type = toLower(type);

	if (creatureTagTextures.find(type) != creatureTagTextures.end()) {
		return creatureTagTextures[type];
	}

	if (creatureTextures.find(type) == creatureTextures.end()) {
		return creatureTextures["unknown"];
	}

	return creatureTextures[type];
}

std::string CreatureTextures::parse(std::string originalName) {
	if (originalName == "NONE") return "";

	originalName = toLower(originalName);

	if (parseMap.find(originalName) == parseMap.end()) {
		return originalName;
	}

	return parseMap[originalName];
}

bool CreatureTextures::known(std::string type) {
	if (type == "") return true;

	return creatureTextures.find(parse(type)) != creatureTextures.end();
}