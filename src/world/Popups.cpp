#include "Popups.hpp"

#include <algorithm>

#include "../Theme.hpp"
#include "../Draw.hpp"

GLuint Popups::textureUI = 0;
std::vector<Popup*> Popups::popupTrash;

void Popups::cleanup() {
	for (Popup *popup : Popups::popupTrash) {
		popups.erase(std::remove(popups.begin(), popups.end(), popup), popups.end());
		
		delete popup;
	}

	Popups::popupTrash.clear();
}

Popup::Popup(Window *window) : bounds(Rect(-0.5, -0.5, 0.5, 0.5)) {
	this->window = window;
}

void Popup::draw(double mouseX, double mouseY, bool mouseInside) {
	Draw::begin(Draw::QUADS);

	setThemeColour(THEME_POPUP_COLOUR);
	// Draw::color(0.0, 0.0, 0.0);
	Draw::vertex(bounds.X0(), bounds.Y0());
	Draw::vertex(bounds.X0(), bounds.Y1());
	Draw::vertex(bounds.X1(), bounds.Y1());
	Draw::vertex(bounds.X1(), bounds.Y0());

	setThemeColour(THEME_POPUP_HEADER_COLOUR);
	// Draw::color(0.2, 0.2, 0.2);
	Draw::vertex(bounds.X0(),  bounds.Y1() - 0.00);
	Draw::vertex(bounds.X0(),  bounds.Y1() - 0.05);
	Draw::vertex(bounds.X1(),  bounds.Y1() - 0.05);
	Draw::vertex(bounds.X1(),  bounds.Y1() - 0.00);

	Draw::end();

	if (mouseInside) {
		setThemeColour(THEME_BORDER_HIGHLIGHT_COLOUR);
		// Draw::color(0.0, 1.0, 0.0);
		glLineWidth(2);
		strokeRect(bounds.X0(), bounds.Y0(), bounds.X1(), bounds.Y1());
	} else {
		setThemeColour(THEME_BORDER_COLOUR);
		// Draw::color(0.75, 0.75, 0.75);
		glLineWidth(1);
		strokeRect(bounds.X0(), bounds.Y0(), bounds.X1(), bounds.Y1());
	}

	setThemeColour(THEME_TEXT_COLOUR);
	Draw::useTexture(Popups::textureUI);
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	Draw::begin(Draw::QUADS);

	Draw::texCoord(0.00f, 0.00f); Draw::vertex(bounds.X1() - 0.05, bounds.Y1() - 0.00);
	Draw::texCoord(0.25f, 0.00f); Draw::vertex(bounds.X1() - 0.00, bounds.Y1() - 0.00);
	Draw::texCoord(0.25f, 0.25f); Draw::vertex(bounds.X1() - 0.00, bounds.Y1() - 0.05);
	Draw::texCoord(0.00f, 0.25f); Draw::vertex(bounds.X1() - 0.05, bounds.Y1() - 0.05);

	Draw::end();
	Draw::useTexture(0);
	glDisable(GL_BLEND);

	if (mouseInside && mouseX >= bounds.X1() - 0.05 && mouseY >= bounds.Y1() - 0.05) {
		setThemeColour(THEME_BORDER_HIGHLIGHT_COLOUR);
	} else {
		setThemeColour(THEME_BORDER_COLOUR);
	}

	glLineWidth(1);
	Draw::begin(Draw::LINE_LOOP);

	Draw::vertex(bounds.X1() - 0.05, bounds.Y1() - 0.00);
	Draw::vertex(bounds.X1() - 0.00, bounds.Y1() - 0.00);
	Draw::vertex(bounds.X1() - 0.00, bounds.Y1() - 0.05);
	Draw::vertex(bounds.X1() - 0.05, bounds.Y1() - 0.05);

	Draw::end();
}

void Popup::mouseClick(double mouseX, double mouseY) {
	if (mouseX >= bounds.X1() - 0.05 && mouseY >= bounds.Y1() - 0.05) {
		close();
	}
}

void Popup::close() {
	removePopup(this);
}

std::vector<Popup*> popups;

void addPopup(Popup *popup) {
	bool canStack = true;
	for (Popup *otherPopup : popups) {
		if (!otherPopup->canStack(popup->PopupName())) {
			canStack = false;
			break;
		}
	}
	
	if (canStack) {
		popups.push_back(popup);
	} else {
		Popups::popupTrash.push_back(popup);
	}
}

void removePopup(Popup *popup) {
	Popups::popupTrash.push_back(popup);
}