#ifndef THEME_HPP
#define THEME_HPP

#include "math/Colour.hpp"

#define THEME_BACKGROUND_COLOUR 0
#define THEME_HEADER_COLOUR 1

#define THEME_BORDER_COLOUR 2
#define THEME_BORDER_HIGHLIGHT_COLOUR 3

#define THEME_POPUP_COLOUR 4
#define THEME_POPUP_HEADER_COLOUR 5

#define THEME_BUTTON_COLOUR 6
#define THEME_BUTTON_DISABLED_COLOUR 7

#define THEME_TEXT_COLOUR 8
#define THEME_TEXT_DISABLED_COLOUR 9

struct Theme {
	Colour background;
	Colour header;
	Colour border;
	Colour borderHighlight;
	Colour popup;
	Colour popupHeader;
	Colour button;
	Colour buttonDisabled;
	Colour text;
	Colour textDisabled;
};

extern Theme themeA;
extern Theme *currentTheme;

void setThemeColour(unsigned int colour);

void setThemeColor(unsigned int color);

#endif