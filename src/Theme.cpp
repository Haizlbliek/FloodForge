#include "Theme.hpp"

#include "gl.h"

Theme themeA {
	Colour(0.3,  0.3,  0.3),  // Background
	Colour(0.0,  0.0,  0.0),  // Header
	Colour(0.75, 0.75, 0.75), // Border
	Colour(0.0,  1.0,  1.0),  // Border Highlight
	Colour(0.0,  0.0,  0.0),  // Popup Background
	Colour(0.2,  0.2,  0.2),  // Popup Header
	Colour(0.2,  0.2,  0.2),  // Button
	Colour(0.2,  0.2,  0.2),  // Button Disabled
	Colour(1.0,  1.0,  1.0),  // Text
	Colour(0.5,  0.5,  0.5)   // Text Disabled
};

Theme themeB {
	Colour(0.96, 0.79, 0.62),  // Background
	Colour(0.54, 0.28, 0.21),  // Header
	Colour(0.75, 0.44, 0.29),  // Border
	Colour(1.00, 0.64, 0.08),  // Border Highlight
	Colour(0.36, 0.17, 0.16),  // Popup Background
	Colour(0.54, 0.28, 0.21),  // Popup Header
	Colour(0.36, 0.17, 0.15),  // Button
	Colour(0.36, 0.17, 0.15),  // Button Disabled
	Colour(0.75, 0.44, 0.29),  // Text
	Colour(0.22, 0.12, 0.13)   // Text Disabled
};

Theme *currentTheme = &themeA;

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
	}

	if (colour.A() == 0.0) return;

	glColor3f(colour.R(), colour.G(), colour.B());
}

void setThemeColor(unsigned int color) {
	setThemeColour(color);
}