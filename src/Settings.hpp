#ifndef SETTINGS_HPP
#define SETTINGS_HPP

#include <string>
#include <fstream>
#include <iostream>

#include "Constants.hpp"
#include "Utils.hpp"
#include "Theme.hpp"

namespace Settings {
    void init() {
	    std::string settingsPath = BASE_PATH + "assets/settings.txt";
	    if (!std::filesystem::exists(settingsPath)) return;

        std::fstream settingsFile(settingsPath);

        std::string line;
        while (std::getline(settingsFile, line)) {
            if (line.empty()) continue;

            if (startsWith(line, "Theme:")) {
                loadTheme(line.substr(line.find_first_of(":") + 1));
            }
        }

        settingsFile.close();
    }

    void cleanup() {

    }
}

#endif