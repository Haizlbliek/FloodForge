#include "ConditionalTimelineTextures.hpp"

#include <fstream>
#include <algorithm>

#include "../Constants.hpp"
#include "../Logger.hpp"

std::unordered_map<std::string, GLuint> ConditionalTimelineTextures::textures;
std::vector<std::string> ConditionalTimelineTextures::timelines;

void ConditionalTimelineTextures::init() {
	std::filesystem::path timelinesDirectory = BASE_PATH / "assets" / "timelines";

	Logger::info("Loading timelines from: '", timelinesDirectory, "'");

	for (const auto& entry : std::filesystem::directory_iterator(timelinesDirectory)) {
		if (std::filesystem::is_regular_file(entry.path()) && entry.path().extension().generic_u8string() == ".png") {
			std::string creature = entry.path().stem().generic_u8string();
			timelines.push_back(creature);
			textures[creature] = loadTexture(entry.path().generic_u8string());
		}
	}

	timelines.erase(std::remove(timelines.begin(), timelines.end(), "UNKNOWN"), timelines.end());
	timelines.erase(std::remove(timelines.begin(), timelines.end(), "WARNING"), timelines.end());
}

GLuint ConditionalTimelineTextures::getTexture(std::string type) {
	if (type == "") return 0;

	std::unordered_map<std::string, GLuint>::iterator index = textures.find(type);

	if (index == textures.end()) {
		return textures["UNKNOWN"];
	}

	return (*index).second;
}