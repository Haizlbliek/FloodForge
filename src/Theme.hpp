#ifndef THEME_HPP
#define THEME_HPP

#include <string>
#include <unordered_map>

#include "math/Colour.hpp"

enum class ThemeColour {
	Background,
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
	Grid
};

extern std::unordered_map<ThemeColour, Colour> currentTheme;

void loadTheme(std::string theme);

void setThemeColour(ThemeColour colour);

void setThemeColor(ThemeColour color);

#endif