#include "Popups.hpp"

#include <algorithm>

#include "../Theme.hpp"
#include "../Draw.hpp"

GLuint Popups::textureUI = 0;
std::vector<Popup*> Popups::popupTrash;
std::vector<Popup*> Popups::popups;

void Popups::cleanup() {
	for (Popup *popup : Popups::popupTrash) {
		Popups::popups.erase(std::remove(Popups::popups.begin(), Popups::popups.end(), popup), Popups::popups.end());
		
		delete popup;
	}

	Popups::popupTrash.clear();
}

Popup::Popup(Window *window) : bounds(Rect(-0.5, -0.5, 0.5, 0.5)), window(window) {
}

void Popup::draw(double mouseX, double mouseY, bool mouseInside) {
	Draw::begin(Draw::QUADS);

	setThemeColour(ThemeColour::Popup);
	// Draw::color(0.0, 0.0, 0.0);
	Draw::vertex(bounds.X0(), bounds.Y0());
	Draw::vertex(bounds.X0(), bounds.Y1());
	Draw::vertex(bounds.X1(), bounds.Y1());
	Draw::vertex(bounds.X1(), bounds.Y0());

	setThemeColour(ThemeColour::PopupHeader);
	// Draw::color(0.2, 0.2, 0.2);
	Draw::vertex(bounds.X0(),  bounds.Y1() - 0.00);
	Draw::vertex(bounds.X0(),  bounds.Y1() - 0.05);
	Draw::vertex(bounds.X1(),  bounds.Y1() - 0.05);
	Draw::vertex(bounds.X1(),  bounds.Y1() - 0.00);

	Draw::end();

	if (mouseInside) {
		setThemeColour(ThemeColour::BorderHighlight);
		// Draw::color(0.0, 1.0, 0.0);
		glLineWidth(2);
		strokeRect(bounds.X0(), bounds.Y0(), bounds.X1(), bounds.Y1());
	} else {
		setThemeColour(ThemeColour::Border);
		// Draw::color(0.75, 0.75, 0.75);
		glLineWidth(1);
		strokeRect(bounds.X0(), bounds.Y0(), bounds.X1(), bounds.Y1());
	}

	setThemeColour(ThemeColour::Text);
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
		setThemeColour(ThemeColour::BorderHighlight);
	} else {
		setThemeColour(ThemeColour::Border);
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
	Popups::removePopup(this);
}

bool Popup::drag(double mouseX, double mouseY) {
	if (mouseX >= bounds.X1() - 0.05 && mouseY >= bounds.Y1() - 0.05)
		return false;

	return (mouseY >= bounds.Y1() - 0.05);
}

void Popup::offset(Vector2 offset) {
	bounds.offset(offset);
}

void Popups::addPopup(Popup *popup) {
	bool canStack = true;
	for (Popup *otherPopup : Popups::popups) {
		if (!otherPopup->canStack(popup->PopupName())) {
			canStack = false;
			break;
		}
	}
	
	if (canStack) {
		Popups::popups.push_back(popup);
	} else {
		popup->close();
	}
}

void Popups::removePopup(Popup *popup) {
	Popups::popupTrash.push_back(popup);
}

void Popups::draw(Vector2 mouse) {
	for (Popup *popup : Popups::popups) {
		Rect bounds = popup->Bounds();

		popup->draw(mouse.x, mouse.y, bounds.inside(mouse));
	}
}