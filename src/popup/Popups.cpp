#include "Popups.hpp"

#include <algorithm>

#include "../Theme.hpp"
#include "../Draw.hpp"

#include "FilesystemPopup.hpp"

GLuint Popups::textureUI = 0;
std::vector<Popup*> Popups::popupTrash;
std::vector<Popup*> Popups::popups;

void Popups::cleanup() {
	for (Popup *popup : Popups::popupTrash) {
		popup->finalCleanup();
		Popups::popups.erase(std::remove(Popups::popups.begin(), Popups::popups.end(), popup), Popups::popups.end());
		
		delete popup;
	}

	Popups::popupTrash.clear();
}

Popup::Popup(Window *window) : bounds(Rect(-0.5, -0.5, 0.5, 0.5)), window(window) {
}

void Popup::draw(double mouseX, double mouseY, bool mouseInside, Vector2 screenBounds) {
	hovered = mouseInside;
	
	Draw::begin(Draw::QUADS);

	if (minimized) {
		setThemeColour(ThemeColour::Popup);
		Draw::vertex(bounds.x0, bounds.y1 - 0.05);
		Draw::vertex(bounds.x0, bounds.y1);
		Draw::vertex(bounds.x1, bounds.y1);
		Draw::vertex(bounds.x1, bounds.y1 - 0.05);
	} else {
		setThemeColour(ThemeColour::Popup);
		Draw::vertex(bounds.x0, bounds.y0);
		Draw::vertex(bounds.x0, bounds.y1);
		Draw::vertex(bounds.x1, bounds.y1);
		Draw::vertex(bounds.x1, bounds.y0);
	}
	setThemeColour(ThemeColour::PopupHeader);
	Draw::vertex(bounds.x0,  bounds.y1 - 0.00);
	Draw::vertex(bounds.x0,  bounds.y1 - 0.05);
	Draw::vertex(bounds.x1,  bounds.y1 - 0.05);
	Draw::vertex(bounds.x1,  bounds.y1 - 0.00);

	Draw::end();

	if (mouseInside) {
		setThemeColour(ThemeColour::BorderHighlight);
		glLineWidth(2);
	} else {
		setThemeColour(ThemeColour::Border);
		glLineWidth(1);
	}
	if (minimized) {
		strokeRect(bounds.x0, bounds.y1 - 0.05, bounds.x1, bounds.y1);
	} else {
		strokeRect(bounds.x0, bounds.y0, bounds.x1, bounds.y1);
	}

	setThemeColour(ThemeColour::Text);
	Draw::useTexture(Popups::textureUI);
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	Draw::begin(Draw::QUADS);

	Draw::texCoord(0.00f, 0.00f); Draw::vertex(bounds.x1 - 0.05, bounds.y1 - 0.00);
	Draw::texCoord(0.25f, 0.00f); Draw::vertex(bounds.x1 - 0.00, bounds.y1 - 0.00);
	Draw::texCoord(0.25f, 0.25f); Draw::vertex(bounds.x1 - 0.00, bounds.y1 - 0.05);
	Draw::texCoord(0.00f, 0.25f); Draw::vertex(bounds.x1 - 0.05, bounds.y1 - 0.05);

	if (minimized) {
		Draw::texCoord(0.75f, 0.25f); Draw::vertex(bounds.x1 - 0.10, bounds.y1 - 0.00);
		Draw::texCoord(1.00f, 0.25f); Draw::vertex(bounds.x1 - 0.05, bounds.y1 - 0.00);
		Draw::texCoord(1.00f, 0.50f); Draw::vertex(bounds.x1 - 0.05, bounds.y1 - 0.05);
		Draw::texCoord(0.75f, 0.50f); Draw::vertex(bounds.x1 - 0.10, bounds.y1 - 0.05);
	} else {
		Draw::texCoord(0.50f, 0.25f); Draw::vertex(bounds.x1 - 0.10, bounds.y1 - 0.00);
		Draw::texCoord(0.75f, 0.25f); Draw::vertex(bounds.x1 - 0.05, bounds.y1 - 0.00);
		Draw::texCoord(0.75f, 0.50f); Draw::vertex(bounds.x1 - 0.05, bounds.y1 - 0.05);
		Draw::texCoord(0.50f, 0.50f); Draw::vertex(bounds.x1 - 0.10, bounds.y1 - 0.05);
	}

	Draw::end();
	Draw::useTexture(0);
	glDisable(GL_BLEND);

	glLineWidth(1);
	
	if (mouseInside && mouseX >= bounds.x1 - 0.05 && mouseY >= bounds.y1 - 0.05) {
		setThemeColour(ThemeColour::BorderHighlight);
	} else {
		setThemeColour(ThemeColour::Border);
	}

	Draw::begin(Draw::LINE_LOOP);
	Draw::vertex(bounds.x1 - 0.05, bounds.y1 - 0.00);
	Draw::vertex(bounds.x1 - 0.00, bounds.y1 - 0.00);
	Draw::vertex(bounds.x1 - 0.00, bounds.y1 - 0.05);
	Draw::vertex(bounds.x1 - 0.05, bounds.y1 - 0.05);
	Draw::end();

	if (mouseInside && mouseX >= bounds.x1 - 0.1 && mouseX <= bounds.x1 - 0.05 && mouseY >= bounds.y1 - 0.05) {
		setThemeColour(ThemeColour::BorderHighlight);
	} else {
		setThemeColour(ThemeColour::Border);
	}

	Draw::begin(Draw::LINE_LOOP);
	Draw::vertex(bounds.x1 - 0.10, bounds.y1 - 0.00);
	Draw::vertex(bounds.x1 - 0.05, bounds.y1 - 0.00);
	Draw::vertex(bounds.x1 - 0.05, bounds.y1 - 0.05);
	Draw::vertex(bounds.x1 - 0.10, bounds.y1 - 0.05);
	Draw::end();
}

void Popup::mouseClick(double mouseX, double mouseY) {
	if (mouseX >= bounds.x1 - 0.05 && mouseY >= bounds.y1 - 0.05) {
		close();
	} else if (mouseX >= bounds.x1 - 0.1 && mouseY >= bounds.y1 - 0.05) {
		minimized = !minimized;
	}
}

void Popup::close() {
	Popups::removePopup(this);
}

bool Popup::drag(double mouseX, double mouseY) {
	if (mouseX >= bounds.x1 - 0.1 && mouseY >= bounds.y1 - 0.05)
		return false;

	return (mouseY >= bounds.y1 - 0.05);
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

void Popups::draw(Vector2 mouse, Vector2 screenBounds) {
	for (Popup *popup : Popups::popups) {
		Rect bounds = popup->Bounds();

		popup->draw(mouse.x, mouse.y, bounds.inside(mouse), screenBounds);
	}
}

bool Popups::hasPopup(std::string popupName) {
	for (Popup *popup : Popups::popups) {
		if (popup->PopupName() == popupName) return true;
	}

	return false;
}


std::filesystem::path FilesystemPopup::previousDirectory;