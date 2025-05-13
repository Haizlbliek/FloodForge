#pragma once

#include <filesystem>
#include <vector>
#include <string>
#include <fstream>
#include <algorithm>

#include "../Constants.hpp"

class RecentFiles {
	public:
		static void init() {
			std::filesystem::path recentsPath = BASE_PATH + "assets/recents.txt";
			if (!std::filesystem::exists(recentsPath)) return;
			
			std::ifstream recentsFile(recentsPath);
			
			std::string line;
			while (std::getline(recentsFile, line)) {
				if (line.empty()) continue;
				
				if (std::filesystem::exists(line))
					recents.push_back(line);
			}
			
			recentsFile.close();
		}
		
		static void addPath(std::filesystem::path path) {
			auto it = std::find(recents.begin(), recents.end(), path);
			if (it != recents.end()) {
				recents.erase(it);
			}
			recents.insert(recents.begin(), path);
			
			save();
		}
	
		static std::vector<std::filesystem::path> recents;
	
	private:
		static void save() {
			std::ofstream recentsFile(BASE_PATH + "assets/recents.txt");
			
			for (std::filesystem::path path : recents) {
				recentsFile << path.generic_string() << "\n";
			}
			
			recentsFile.close();
		}
};