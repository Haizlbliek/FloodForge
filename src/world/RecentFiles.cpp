#include "RecentFiles.hpp"

#include "../Utils.hpp"

std::vector<std::filesystem::path> RecentFiles::recents;
std::vector<std::string> RecentFiles::recentsNames;

void RecentFiles::init() {
	std::filesystem::path recentsPath = BASE_PATH / "assets" / "recents.txt";
	if (!std::filesystem::exists(recentsPath)) return;
	
	std::ifstream recentsFile(recentsPath);
	
	std::string line;
	while (std::getline(recentsFile, line)) {
		if (line.empty()) continue;

		if (line.back() == '\r') line.pop_back();
		
		std::filesystem::path path = line;
		if (std::filesystem::exists(path)) {
			recents.push_back(path);

			std::filesystem::path displayNamePath = findFileCaseInsensitive(path.parent_path(), "displayname.txt");

			if (displayNamePath.empty()) {
				recentsNames.push_back("");
			} else {
				std::ifstream displayNameFile(displayNamePath);
				if (std::getline(displayNameFile, line)) {
					if (line.back() == '\r') line.pop_back();

					recentsNames.push_back(line);
				} else {
					recentsNames.push_back("");
				}
				displayNameFile.close();
			}
		}
	}
	
	recentsFile.close();
}

void RecentFiles::addPath(std::filesystem::path path) {
	auto it = std::find(recents.begin(), recents.end(), path);
	if (it != recents.end()) {
		recents.erase(it);
	}
	recents.insert(recents.begin(), path);
	
	save();
}

void RecentFiles::save() {
	std::ofstream recentsFile(BASE_PATH / "assets" / "recents.txt");
	
	for (std::filesystem::path path : recents) {
		recentsFile << path.generic_string() << "\n";
	}
	
	recentsFile.close();
}