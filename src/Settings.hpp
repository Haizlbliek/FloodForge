#pragma once

#include <string>
#include <fstream>
#include <iostream>
#include <unordered_map>
#include <variant>

#include "Constants.hpp"
#include "Utils.hpp"
#include "Theme.hpp"

namespace Settings {
	enum class DropletGridVisibility { NONE, AIR, ALL };

	using SettingType = std::variant<double, int, bool, Color, std::string, std::vector<Color>, Settings::DropletGridVisibility>;


	enum class Setting {
		CameraPanSpeed,
		CameraZoomSpeed,
		PopupScrollSpeed,
		ConnectionType,
		ConnectionPoint,
		OrignalControls,
		WorldIconScale,
		DefaultFilePath,
		WarnMissingImages,
		HideTutorial,
		KeepFilesystemPath,
		UpdateWorldFiles,
		DebugVisibleOutputPadding,
		NoSubregionColor,
		SubregionColors,
		RoomTintStrength,
		ForceExportCasing,
		DropletGridVisibility,
		ConnectionOpacity,
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