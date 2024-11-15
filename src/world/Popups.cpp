#include "Popups.hpp"

#include <algorithm>

#include "../Theme.hpp"

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
	glBegin(GL_QUADS);

	setThemeColour(THEME_POPUP_COLOUR);
	// glColor3f(0.0, 0.0, 0.0);
	glVertex2f(bounds.X0(), bounds.Y0());
	glVertex2f(bounds.X0(), bounds.Y1());
	glVertex2f(bounds.X1(), bounds.Y1());
	glVertex2f(bounds.X1(), bounds.Y0());

	setThemeColour(THEME_POPUP_HEADER_COLOUR);
	// glColor3f(0.2, 0.2, 0.2);
	glVertex2f(bounds.X0(),  bounds.Y1() - 0.00);
	glVertex2f(bounds.X0(),  bounds.Y1() - 0.05);
	glVertex2f(bounds.X1(),  bounds.Y1() - 0.05);
	glVertex2f(bounds.X1(),  bounds.Y1() - 0.00);

	glEnd();

	if (mouseInside) {
		setThemeColour(THEME_BORDER_HIGHLIGHT_COLOUR);
		// glColor3f(0.0, 1.0, 0.0);
		glLineWidth(2);
		strokerect(bounds.X0(), bounds.Y0(), bounds.X1(), bounds.Y1());
	} else {
		setThemeColour(THEME_BORDER_COLOUR);
		// glColor3f(0.75, 0.75, 0.75);
		glLineWidth(1);
		strokerect(bounds.X0(), bounds.Y0(), bounds.X1(), bounds.Y1());
	}

	setThemeColour(THEME_TEXT_COLOUR);
	glBindTexture(GL_TEXTURE_2D, Popups::textureUI);
	glEnable(GL_TEXTURE_2D);
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glBegin(GL_QUADS);

	glTexCoord2f(0.00f, 0.00f); glVertex2f(bounds.X1() - 0.05, bounds.Y1() - 0.00);
	glTexCoord2f(0.25f, 0.00f); glVertex2f(bounds.X1() - 0.00, bounds.Y1() - 0.00);
	glTexCoord2f(0.25f, 0.25f); glVertex2f(bounds.X1() - 0.00, bounds.Y1() - 0.05);
	glTexCoord2f(0.00f, 0.25f); glVertex2f(bounds.X1() - 0.05, bounds.Y1() - 0.05);

	glEnd();
	glDisable(GL_TEXTURE_2D);
	glDisable(GL_BLEND);

	if (mouseInside && mouseX >= bounds.X1() - 0.05 && mouseY >= bounds.Y1() - 0.05) {
		setThemeColour(THEME_BORDER_HIGHLIGHT_COLOUR);
	} else {
		setThemeColour(THEME_BORDER_COLOUR);
	}

	glLineWidth(1);
	glBegin(GL_LINE_LOOP);

	glVertex2f(bounds.X1() - 0.05, bounds.Y1() - 0.00);
	glVertex2f(bounds.X1() - 0.00, bounds.Y1() - 0.00);
	glVertex2f(bounds.X1() - 0.00, bounds.Y1() - 0.05);
	glVertex2f(bounds.X1() - 0.05, bounds.Y1() - 0.05);

	glEnd();
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
	popups.push_back(popup);
}

void removePopup(Popup *popup) {
	Popups::popupTrash.push_back(popup);
}