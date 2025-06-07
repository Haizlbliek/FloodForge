#include "SubregionNewPopup.hpp"

#include "MenuItems.hpp"

void SubregionNewPopup::draw(double mouseX, double mouseY, bool mouseInside, Vector2 screenBounds) {
	Popup::draw(mouseX, mouseY, mouseInside, screenBounds);
	
	if (minimized) return;

	mouseX -= bounds.X0() + 0.4;
	mouseY -= bounds.Y0() + 0.08;

	Draw::pushMatrix();

	Draw::translate(bounds.X0() + 0.4, bounds.Y0() + 0.08);

	setThemeColour(ThemeColour::Text);
	glLineWidth(1);
	Fonts::rainworld->writeCentred(editIndex == -1 ? "Enter Subregion Name:" : "Edit Subregion Name:", 0.0, 0.18, 0.035, CENTRE_X);
	Fonts::rainworld->writeCentred(text, 0.0, 0.12, 0.055, CENTRE_X);

	setThemeColour(ThemeColour::Button);
	fillRect(-0.2, -0.03, -0.05, 0.03);

	if (text.length() < 1) {
		setThemeColour(ThemeColour::ButtonDisabled);
	} else {
		setThemeColour(ThemeColour::Button);
	}
	fillRect( 0.05, -0.03,  0.2,  0.03);

	setThemeColour(ThemeColour::Text);
	Fonts::rainworld->writeCentred("Cancel", -0.125, 0.0, 0.03, CENTRE_XY);

	if (text.length() < 1) {
		setThemeColour(ThemeColour::TextDisabled);
	} else {
		setThemeColour(ThemeColour::Text);
	}
	Fonts::rainworld->writeCentred("Confirm", 0.125, 0.0, 0.03, CENTRE_XY);

	if (Rect(-0.2, -0.03, -0.05, 0.03).inside(mouseX, mouseY)) {
		setThemeColour(ThemeColour::BorderHighlight);
		strokeRect(-0.2, -0.03, -0.05, 0.03);
	} else {
		setThemeColour(ThemeColour::Border);
		strokeRect(-0.2, -0.03, -0.05, 0.03);
	}

	if (Rect(0.05, -0.03, 0.2, 0.03).inside(mouseX, mouseY)) {
		setThemeColour(ThemeColour::BorderHighlight);
		strokeRect(0.05, -0.03, 0.2, 0.03);
	} else {
		setThemeColour(ThemeColour::Border);
		strokeRect(0.05, -0.03, 0.2, 0.03);
	}

	Draw::popMatrix();
}

void SubregionNewPopup::mouseClick(double mouseX, double mouseY) {
	Popup::mouseClick(mouseX, mouseY);

	mouseX -= bounds.X0() + 0.4;
	mouseY -= bounds.Y0() + 0.08;

	if (Rect(-0.2, -0.03, -0.05, 0.03).inside(mouseX, mouseY)) {
		reject();
	}

	if (Rect(0.05, -0.03, 0.2, 0.03).inside(mouseX, mouseY)) {
		accept();
	}
}

void SubregionNewPopup::accept() {
	if (text.empty()) return;
	
	if (editIndex == -1) {
		if (std::find(subregions.begin(), subregions.end(), text) != subregions.end())
			return;
	
		subregions.push_back(text);
		for (Room *room : rooms) {
			room->subregion = std::distance(subregions.begin(), std::find(subregions.begin(), subregions.end(), text));
		}
	} else {
		if (std::find(subregions.begin(), subregions.end(), text) != subregions.end())
			return;
	
		subregions[editIndex] = text;
	}

	close();
}