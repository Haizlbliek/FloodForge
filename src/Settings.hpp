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
    enum class Setting {
        CameraPanSpeed,
        CameraZoomSpeed,
        PopupScrollSpeed,
        ConnectionType
    };

    extern std::unordered_map<Setting, std::variant<double, int, Colour, std::string>> settings;

    void loadDefaults();

    template<typename T>
    T getSetting(Setting setting) {
        return std::get<T>(settings.at(setting));
    }

    void init();

    void cleanup();
}