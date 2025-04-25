#include "Settings.hpp"

std::unordered_map<Settings::Setting, std::variant<double, int, bool, Colour, std::string>> Settings::settings;

void Settings::loadDefaults() {
	settings[Setting::CameraPanSpeed] = 0.4;
	settings[Setting::CameraZoomSpeed] = 0.4;
	settings[Setting::PopupScrollSpeed] = 0.4;
	settings[Setting::ConnectionType] = 0;
	settings[Setting::OrignalControls] = false;
	settings[Setting::SelectorScale] = true;
	settings[Setting::DefaultFilePath] = "NON_EXISTANT_PATH_YOU_CAN'T_HAVE_THIS_PATH_PLSPLSPLS///";
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
		else if (key == "PopupScrollSpeed") settings[Setting::PopupScrollSpeed] = std::stod(value);
		else if (key == "ConnectionType") settings[Setting::ConnectionType] = int(toLower(value) == "bezier");
		else if (key == "OriginalControls") settings[Setting::OrignalControls] = (toLower(value) == "true" || toLower(value) == "yes" || toLower(value) == "1");
		else if (key == "SelectorScale") settings[Setting::SelectorScale] = (toLower(value) == "true" || toLower(value) == "yes" || toLower(value) == "1");
		else if (key == "DefaultFilePath") settings[Setting::DefaultFilePath] = value;
	}

	settingsFile.close();
}

void Settings::cleanup() {

}