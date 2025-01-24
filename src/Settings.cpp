#include "Settings.hpp"

std::unordered_map<Settings::Setting, std::variant<double, int, Colour, std::string>> Settings::settings;

void Settings::loadDefaults() {
    settings[Setting::CameraPanSpeed] = 0.4;
    settings[Setting::CameraZoomSpeed] = 0.4;
    settings[Setting::PopupScrollSpeed] = 0.4;
}

void Settings::init() {
    loadDefaults();

    std::string settingsPath = BASE_PATH + "assets/settings.txt";
    if (!std::filesystem::exists(settingsPath)) return;

    std::fstream settingsFile(settingsPath);

    std::string line;
    while (std::getline(settingsFile, line)) {
        if (line.empty()) continue;

        std::string key = line.substr(0, line.find_first_of(':'));
        std::string value = line.substr(line.find_first_of(':') + 2);
        if (key == "Theme") loadTheme(value);
        else if (key == "CameraPanSpeed") settings[Setting::CameraPanSpeed] = std::stod(value);
        else if (key == "CameraZoomSpeed") settings[Setting::CameraZoomSpeed] = std::stod(value);
    }

    settingsFile.close();
}

void Settings::cleanup() {

}