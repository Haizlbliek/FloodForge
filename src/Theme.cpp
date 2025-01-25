#include "Theme.hpp"

#include "gl.h"

#include <iostream>
#include <fstream>
#include <sstream>

#include "Constants.hpp"
#include "Utils.hpp"
#include "Draw.hpp"

std::unordered_map<ThemeColour, Colour> themeBasic {
	{ ThemeColour::Background, Colour(0.3,  0.3,  0.3) },
	{ ThemeColour::Header, Colour(0.0,  0.0,  0.0) },
	{ ThemeColour::Border, Colour(0.75, 0.75, 0.75) },
	{ ThemeColour::BorderHighlight, Colour(0.0,  1.0,  1.0) },
	{ ThemeColour::Popup, Colour(0.0,  0.0,  0.0) },
	{ ThemeColour::PopupHeader, Colour(0.2,  0.2,  0.2)},
	{ ThemeColour::Button, Colour(0.2,  0.2,  0.2) },
	{ ThemeColour::ButtonDisabled, Colour(0.2,  0.2,  0.2) },
	{ ThemeColour::Text, Colour(1.0,  1.0,  1.0) },
	{ ThemeColour::TextDisabled, Colour(0.5,  0.5,  0.5) },
	{ ThemeColour::TextHighlight, Colour(0.0,  1.0,  1.0) },
	{ ThemeColour::SelectionBorder, Colour(0.3,  0.3,  0.3) },
	{ ThemeColour::Grid, Colour(0.3,  0.3,  0.3) },
    { ThemeColour::RoomBorder, Colour(0.6, 0.6, 0.6) },
    { ThemeColour::RoomBorderHighlight, Colour(0.00, 0.75, 0.00) }
};

std::unordered_map<ThemeColour, Colour> currentTheme;

std::string currentThemeName = "";

Colour parseHexColor(const std::string &hex) {
	if (hex.size() != 7 || hex[0] != '#') {
		throw std::invalid_argument("Invalid hex color format. Expected format: #RRGGBB");
	}

	int red, green, blue;
	std::stringstream ss;
	ss << std::hex;

	ss.str(hex.substr(1, 2));
	ss.clear();
	ss >> red;

	ss.str(hex.substr(3, 2));
	ss.clear();
	ss >> green;

	ss.str(hex.substr(5, 2));
	ss.clear();
	ss >> blue;

	return Colour(red / 255.0, green / 255.0, blue / 255.0);
}

void loadTheme(std::string theme) {
	if (!std::filesystem::exists(BASE_PATH + "assets/themes/" + theme)) return;

	std::string themePath = BASE_PATH + "assets/themes/" + theme + "/theme.txt";
	if (!std::filesystem::exists(themePath)) return;

	currentThemeName = theme;
	currentTheme = { themeBasic };

	std::fstream themeFile(themePath);

	std::string line;
	while (std::getline(themeFile, line)) {
		if (line.empty()) continue;

		std::string colourString = line.substr(line.find_first_of(':') + 2);
		Colour colour = parseHexColor(colourString);

		if (startsWith(line, "Background:")) currentTheme[ThemeColour::Background] = colour;
		else if (startsWith(line, "Grid:")) currentTheme[ThemeColour::Grid] = colour;
		else if (startsWith(line, "Header:")) currentTheme[ThemeColour::Header] = colour;
		else if (startsWith(line, "Border:")) currentTheme[ThemeColour::Border] = colour;
		else if (startsWith(line, "BorderHighlight:")) currentTheme[ThemeColour::BorderHighlight] = colour;
		else if (startsWith(line, "Popup:")) currentTheme[ThemeColour::Popup] = colour;
		else if (startsWith(line, "PopupHeader:")) currentTheme[ThemeColour::PopupHeader] = colour;
		else if (startsWith(line, "Button:")) currentTheme[ThemeColour::Button] = colour;
		else if (startsWith(line, "ButtonDisabled:")) currentTheme[ThemeColour::ButtonDisabled] = colour;
		else if (startsWith(line, "Text:")) currentTheme[ThemeColour::Text] = colour;
		else if (startsWith(line, "TextDisabled:")) currentTheme[ThemeColour::TextDisabled] = colour;
		else if (startsWith(line, "TextHighlight:")) currentTheme[ThemeColour::TextHighlight] = colour;
		else if (startsWith(line, "SelectionBorder:")) currentTheme[ThemeColour::SelectionBorder] = colour;
		else if (startsWith(line, "RoomBorder:")) currentTheme[ThemeColour::RoomBorder] = colour;
		else if (startsWith(line, "RoomBorderHighlight:")) currentTheme[ThemeColour::RoomBorderHighlight] = colour;
	}

	themeFile.close();
}

void setThemeColour(ThemeColour colour) {
	if (currentTheme.find(colour) == currentTheme.end()) return;

	const Colour& col = currentTheme[colour];
	Draw::color(col.r, col.g, col.b);
}

void setThemeColor(ThemeColour colour) {
	setThemeColour(colour);
}