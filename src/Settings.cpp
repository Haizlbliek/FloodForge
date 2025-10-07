#include "Settings.hpp"

#include "Logger.hpp"

std::unordered_map<Settings::Setting, SettingType> Settings::settings;

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
	settings[Setting::NoSubregionColor] = Colour(1.0, 1.0, 1.0);
	settings[Setting::RoomTintStrength] = 0.5;

	std::vector<Colour> subregionColors;
	subregionColors.push_back(Colour(1.0, 0.0, 0.0)); // #ff0000
	subregionColors.push_back(Colour(0.0, 1.0, 0.0)); // #00ff00
	subregionColors.push_back(Colour(0.0, 0.0, 1.0)); // #0000ff
	subregionColors.push_back(Colour(1.0, 1.0, 0.0)); // #ffff00
	subregionColors.push_back(Colour(0.0, 1.0, 1.0)); // #00ffff
	subregionColors.push_back(Colour(1.0, 0.0, 1.0)); // #ff00ff
	subregionColors.push_back(Colour(1.0, 0.5, 0.0)); // #ff7f00
	subregionColors.push_back(Colour(0.5, 0.5, 0.5)); // #7f7f7f
	subregionColors.push_back(Colour(0.5, 0.0, 1.0)); // #7f00ff
	subregionColors.push_back(Colour(1.0, 0.5, 1.0)); // #ff7fff
	settings[Setting::SubregionColors] = subregionColors;
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

		try {
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
			else if (key == "NoSubregionColor") settings[Setting::NoSubregionColor] = stringToColour(value);
			else if (key == "RoomTintStrength") settings[Setting::RoomTintStrength] = std::stod(value);
			else if (key == "SubregionColors") {
				std::vector<Colour> subregionColors;
				for (std::string item : split(value, ", ")) {
					subregionColors.push_back(stringToColour(item));
				}
				settings[Setting::SubregionColors] = subregionColors;
			};
		} catch (...) {
			Logger::error("Error while loading setting: ", key, " - ", value);
		}
	}

	settingsFile.close();
}

void Settings::cleanup() {

}