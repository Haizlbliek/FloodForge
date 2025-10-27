#include "Theme.hpp"

#include "gl.h"

#include <iostream>
#include <fstream>
#include <sstream>
#include <algorithm>

#include "Constants.hpp"
#include "Utils.hpp"
#include "Draw.hpp"

std::unordered_map<ThemeColor, Color> themeBasic {
	{ ThemeColor::Background,            Color(0.3,  0.3,  0.3) },
	{ ThemeColor::Header,                Color(0.0,  0.0,  0.0) },
	{ ThemeColor::Border,                Color(0.75, 0.75, 0.75) },
	{ ThemeColor::BorderHighlight,       Color(0.0,  1.0,  1.0) },
	{ ThemeColor::Popup,                 Color(0.0,  0.0,  0.0) },
	{ ThemeColor::PopupHeader,           Color(0.2,  0.2,  0.2) },
	{ ThemeColor::Button,                Color(0.2,  0.2,  0.2) },
	{ ThemeColor::ButtonDisabled,        Color(0.2,  0.2,  0.2) },
	{ ThemeColor::Text,                  Color(1.0,  1.0,  1.0) },
	{ ThemeColor::TextDisabled,          Color(0.5,  0.5,  0.5) },
	{ ThemeColor::TextHighlight,         Color(0.0,  1.0,  1.0) },
	{ ThemeColor::SelectionBorder,       Color(0.3,  0.3,  0.3) },
	{ ThemeColor::Grid,                  Color(0.3,  0.3,  0.3) },
	{ ThemeColor::RoomBorder,            Color(0.6,  0.6,  0.6) },
	{ ThemeColor::RoomBorderHighlight,   Color(0.00, 0.75, 0.00) },
	{ ThemeColor::RoomAir,               Color(1.0,  1.0,  1.0) },
	{ ThemeColor::RoomSolid,             Color(0.0,  0.0,  0.0) },
	{ ThemeColor::RoomPole,              Color(0.0,  0.0,  0.0) },
	{ ThemeColor::RoomPlatform,          Color(0.0,  0.0,  0.0) },
	{ ThemeColor::RoomShortcutEnterance, Color(0.0,  1.0,  1.0) },
	{ ThemeColor::RoomShortcutDot,       Color(1.0,  1.0,  1.0) },
	{ ThemeColor::RoomShortcutRoom,      Color(1.0,  0.0,  1.0) },
	{ ThemeColor::RoomShortcutDen,       Color(0.0,  1.0,  0.0) },
	{ ThemeColor::RoomConnection,        Color(1.0,  1.0,  0.0) },
	{ ThemeColor::RoomConnectionHover,   Color(0.0,  1.0,  1.0) },
};

std::unordered_map<ThemeColor, Color> currentTheme = { themeBasic };
std::vector<std::string> activeThemes;

const std::filesystem::path THEMES_PATH = ASSETS_PATH / "themes";

void loadTheme(std::string theme) {
	std::filesystem::path themePath = THEMES_PATH / theme / "theme.txt";
	if (!std::filesystem::exists(themePath)) return;

	std::fstream themeFile(themePath);

	std::string line;
	while (std::getline(themeFile, line)) {
		if (line.empty()) continue;
		if (line.back() == '\r') line.pop_back();
		if (startsWith(line, "//")) continue;

		std::string colorString = line.substr(line.find_first_of(':') + 2);
		colorString.erase(std::remove(colorString.begin(), colorString.end(), '\r'), colorString.end());
		Color color = stringToColor(colorString);

		if      (startsWith(line, "Background:"           )) currentTheme[ThemeColor::Background           ] = color;
		else if (startsWith(line, "Grid:"                 )) currentTheme[ThemeColor::Grid                 ] = color;
		else if (startsWith(line, "Header:"               )) currentTheme[ThemeColor::Header               ] = color;
		else if (startsWith(line, "Border:"               )) currentTheme[ThemeColor::Border               ] = color;
		else if (startsWith(line, "BorderHighlight:"      )) currentTheme[ThemeColor::BorderHighlight      ] = color;
		else if (startsWith(line, "Popup:"                )) currentTheme[ThemeColor::Popup                ] = color;
		else if (startsWith(line, "PopupHeader:"          )) currentTheme[ThemeColor::PopupHeader          ] = color;
		else if (startsWith(line, "Button:"               )) currentTheme[ThemeColor::Button               ] = color;
		else if (startsWith(line, "ButtonDisabled:"       )) currentTheme[ThemeColor::ButtonDisabled       ] = color;
		else if (startsWith(line, "Text:"                 )) currentTheme[ThemeColor::Text                 ] = color;
		else if (startsWith(line, "TextDisabled:"         )) currentTheme[ThemeColor::TextDisabled         ] = color;
		else if (startsWith(line, "TextHighlight:"        )) currentTheme[ThemeColor::TextHighlight        ] = color;
		else if (startsWith(line, "SelectionBorder:"      )) currentTheme[ThemeColor::SelectionBorder      ] = color;
		else if (startsWith(line, "RoomBorder:"           )) currentTheme[ThemeColor::RoomBorder           ] = color;
		else if (startsWith(line, "RoomBorderHighlight:"  )) currentTheme[ThemeColor::RoomBorderHighlight  ] = color;
		else if (startsWith(line, "RoomAir:"              )) currentTheme[ThemeColor::RoomAir              ] = color;
		else if (startsWith(line, "RoomSolid:"            )) currentTheme[ThemeColor::RoomSolid            ] = color;
		else if (startsWith(line, "RoomPole:"             )) currentTheme[ThemeColor::RoomPole             ] = color;
		else if (startsWith(line, "RoomPlatform:"         )) currentTheme[ThemeColor::RoomPlatform         ] = color;
		else if (startsWith(line, "RoomShortcutEnterance:")) currentTheme[ThemeColor::RoomShortcutEnterance] = color;
		else if (startsWith(line, "RoomShortcutDot:"      )) currentTheme[ThemeColor::RoomShortcutDot      ] = color;
		else if (startsWith(line, "RoomShortcutRoom:"     )) currentTheme[ThemeColor::RoomShortcutRoom     ] = color;
		else if (startsWith(line, "RoomShortcutDen:"      )) currentTheme[ThemeColor::RoomShortcutDen      ] = color;
		else if (startsWith(line, "RoomConnection:"       )) currentTheme[ThemeColor::RoomConnection       ] = color;
		else if (startsWith(line, "RoomConnectionHover:"  )) currentTheme[ThemeColor::RoomConnectionHover  ] = color;
	}

	themeFile.close();
}

void loadThemes(std::string value) {
	activeThemes = split(value, ',');

	for (std::string theme : activeThemes) {
		loadTheme(theme);
	}
}

void setThemeColor(ThemeColor color) {
	if (currentTheme.find(color) == currentTheme.end()) return;

	const Color& col = currentTheme[color];
	Draw::color(col.r, col.g, col.b);
}

std::filesystem::path getPath(std::string fileName) {
	for (int i = activeThemes.size() - 1; i >= 0; i--) {
		std::filesystem::path path = THEMES_PATH / activeThemes[i] / fileName;

		if (std::filesystem::exists(path)) {
			return path;
		}
	}

	return ASSETS_PATH / fileName;
}