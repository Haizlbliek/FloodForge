#pragma once

#include <string>
#include <unordered_map>

#include "math/Colour.hpp"

enum class ThemeColour {
	Background,
	Grid,
	Header,
	Border,
	BorderHighlight,
	Popup,
	PopupHeader,
	Button,
	ButtonDisabled,
	Text,
	TextDisabled,
	TextHighlight,
	SelectionBorder,
	RoomBorder,
	RoomBorderHighlight,
};

extern std::unordered_map<ThemeColour, Colour> currentTheme;

extern std::string currentThemeName;

void loadTheme(std::string theme);

void setThemeColour(ThemeColour colour);

void setThemeColor(ThemeColour color);