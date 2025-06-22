#pragma once

#include <filesystem>
#include <vector>
#include <string>
#include <fstream>
#include <algorithm>

#include "../Constants.hpp"

class RecentFiles {
	public:
		static void init();
		
		static void addPath(std::filesystem::path path);
	
		static std::vector<std::filesystem::path> recents;
	
	private:
		static void save();
};