#pragma once

#include <string>
#include <fstream>
#include <iostream>
#include <unordered_map>
#include <variant>

#include "Constants.hpp"
#include "Utils.hpp"
#include "Theme.hpp"

using SettingType = std::variant<double, int, bool, Colour, std::string, std::vector<Colour>>;

namespace Settings {
	enum class Setting {
		CameraPanSpeed,
		CameraZoomSpeed,
		PopupScrollSpeed,
		ConnectionType,
		ConnectionPoint,
		OrignalControls,
		SelectorScale,
		DefaultFilePath,
		WarnMissingImages,
		HideTutorial,
		KeepFilesystemPath,
		UpdateWorldFiles,
		DebugVisibleOutputPadding,
		NoSubregionColor,
		SubregionColors,
		RoomTintStrength,
		ForceExportCasing
	};

	extern std::unordered_map<Setting, SettingType> settings;

	void loadDefaults();

	template<typename T>
	T getSetting(Setting setting) {
		return std::get<T>(settings.at(setting));
	}

	void init();

	void cleanup();
}