#include "Theme.hpp"

#include "gl.h"

#include <iostream>
#include <fstream>
#include <sstream>

#include "Constants.hpp"
#include "Utils.hpp"
#include "Draw.hpp"

Theme themeBasic {
	Colour(0.3,  0.3,  0.3),  // Background
	Colour(0.0,  0.0,  0.0),  // Header
	Colour(0.75, 0.75, 0.75), // Border
	Colour(0.0,  1.0,  1.0),  // Border Highlight
	Colour(0.0,  0.0,  0.0),  // Popup Background
	Colour(0.2,  0.2,  0.2),  // Popup Header
	Colour(0.2,  0.2,  0.2),  // Button
	Colour(0.2,  0.2,  0.2),  // Button Disabled
	Colour(1.0,  1.0,  1.0),  // Text
	Colour(0.5,  0.5,  0.5),  // Text Disabled
	Colour(0.0,  1.0,  1.0),  // Text Highlight
};

Theme *currentTheme = &themeBasic;

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

void loadTheme() {
	std::string themePath = BASE_PATH + "assets/theme.txt";

	// std::cout << themePath << std::endl;

	if (!std::filesystem::exists(themePath)) return;

	if (currentTheme != &themeBasic) delete currentTheme;

	currentTheme = new Theme(themeBasic);

	std::fstream themeFile(themePath);

	std::string line;
	while (std::getline(themeFile, line)) {
		if (line.empty()) continue;

		std::string colourString = line.substr(line.find_last_of(" ") + 1);
		Colour colour = parseHexColor(colourString);

		if (startsWith(line, "Background:")) currentTheme->background = colour;
		if (startsWith(line, "Header:")) currentTheme->header = colour;
		if (startsWith(line, "Border:")) currentTheme->border = colour;
		if (startsWith(line, "BorderHighlight:")) currentTheme->borderHighlight = colour;
		if (startsWith(line, "Popup:")) currentTheme->popup = colour;
		if (startsWith(line, "PopupHeader:")) currentTheme->popupHeader = colour;
		if (startsWith(line, "Button:")) currentTheme->button = colour;
		if (startsWith(line, "ButtonDisabled:")) currentTheme->buttonDisabled = colour;
		if (startsWith(line, "Text:")) currentTheme->text = colour;
		if (startsWith(line, "TextDisabled:")) currentTheme->textDisabled = colour;
		if (startsWith(line, "TextHighlight:")) currentTheme->textHighlight = colour;
	}

	themeFile.close();
}

void setThemeColour(unsigned int themeColour) {
	Colour colour = Colour(0.0, 0.0, 0.0, 0.0);

	switch (themeColour) {
		case THEME_HEADER_COLOUR:
			colour = currentTheme->header;
			break;
		case THEME_BACKGROUND_COLOUR:
			colour = currentTheme->background;
			break;
		case THEME_POPUP_COLOUR:
			colour = currentTheme->popup;
			break;
		case THEME_POPUP_HEADER_COLOUR:
			colour = currentTheme->popupHeader;
			break;
		case THEME_BORDER_COLOUR:
			colour = currentTheme->border;
			break;
		case THEME_BORDER_HIGHLIGHT_COLOUR:
			colour = currentTheme->borderHighlight;
			break;
		case THEME_BUTTON_COLOUR:
			colour = currentTheme->button;
			break;
		case THEME_BUTTON_DISABLED_COLOUR:
			colour = currentTheme->buttonDisabled;
			break;
		case THEME_TEXT_COLOUR:
			colour = currentTheme->text;
			break;
		case THEME_TEXT_DISABLED_COLOUR:
			colour = currentTheme->textDisabled;
			break;
		case THEME_TEXT_HIGHLIGHT_COLOUR:
			colour = currentTheme->textHighlight;
			break;
	}

	if (colour.A() == 0.0) return;

	Draw::color(colour.R(), colour.G(), colour.B());
}

void setThemeColor(unsigned int color) {
	setThemeColour(color);
}