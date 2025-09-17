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
	settings[Setting::WarnMissingImages] = false;
	settings[Setting::HideTutorial] = false;
	settings[Setting::KeepFilesystemPath] = true;
	settings[Setting::UpdateWorldFiles] = true;
	settings[Setting::DebugVisibleOutputPadding] = false;
}

void Settings::init() {
	loadDefaults();

	std::filesystem::path settingsPath = BASE_PATH / "assets" / "settings.txt";
	if (!std::filesystem::exists(settingsPath)) return;

	std::fstream settingsFile(settingsPath);

	std::string line;
	while (std::getline(settingsFile, line)) {
		if (line.empty()) continue;
		if (line.back() == '\r') line.pop_back();
		if (startsWith(line, "//")) continue;

		std::string key = line.substr(0, line.find_first_of(':'));
		std::string value = line.substr(line.find_first_of(':') + 2);
		bool boolValue = (toLower(value) == "true" || toLower(value) == "yes" || toLower(value) == "1");
		
		if (key == "Theme") loadTheme(value);
		else if (key == "CameraPanSpeed") settings[Setting::CameraPanSpeed] = std::stod(value);
		else if (key == "CameraZoomSpeed") settings[Setting::CameraZoomSpeed] = std::stod(value);
		else if (key == "PopupScrollSpeed") settings[Setting::PopupScrollSpeed] = std::stod(value);
		else if (key == "ConnectionType") settings[Setting::ConnectionType] = int(toLower(value) == "bezier");
		else if (key == "OriginalControls") settings[Setting::OrignalControls] = boolValue;
		else if (key == "SelectorScale") settings[Setting::SelectorScale] = boolValue;
		else if (key == "DefaultFilePath") settings[Setting::DefaultFilePath] = value;
		else if (key == "WarnMissingImages") settings[Setting::WarnMissingImages] = boolValue;
		else if (key == "HideTutorial") settings[Setting::HideTutorial] = boolValue;
		else if (key == "KeepFilesystemPath") settings[Setting::KeepFilesystemPath] = boolValue;
		else if (key == "UpdateWorldFiles") settings[Setting::UpdateWorldFiles] = boolValue;
		else if (key == "DebugVisibleOutputPadding") settings[Setting::DebugVisibleOutputPadding] = boolValue;
	}

	settingsFile.close();
}

void Settings::cleanup() {

}