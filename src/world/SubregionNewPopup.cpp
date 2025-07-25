#include "SubregionNewPopup.hpp"

#include "MenuItems.hpp"

SubregionNewPopup::SubregionNewPopup(Window *window, std::set<Room*> rooms, int editIndex) : Popup(window), rooms(rooms), editIndex(editIndex) {
	window->addKeyCallback(this, keyCallback);

	bounds = Rect(-0.4, -0.08, 0.4, 0.25);

	text = editIndex == -1 ? "" : EditorState::subregions[editIndex];
}

void SubregionNewPopup::draw(double mouseX, double mouseY, bool mouseInside, Vector2 screenBounds) {
	Popup::draw(mouseX, mouseY, mouseInside, screenBounds);
	
	if (minimized) return;

	mouseX -= bounds.x0 + 0.4;
	mouseY -= bounds.y0 + 0.08;

	Draw::pushMatrix();

	Draw::translate(bounds.x0 + 0.4, bounds.y0 + 0.08);

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

	mouseX -= bounds.x0 + 0.4;
	mouseY -= bounds.y0 + 0.08;

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
		if (std::find(EditorState::subregions.begin(), EditorState::subregions.end(), text) != EditorState::subregions.end()) {
			return;
		}
	
		EditorState::subregions.push_back(text);
		for (Room *room : rooms) {
			room->subregion = std::distance(EditorState::subregions.begin(), std::find(EditorState::subregions.begin(), EditorState::subregions.end(), text));
		}
	} else {
		if (std::find(EditorState::subregions.begin(), EditorState::subregions.end(), text) != EditorState::subregions.end()) {
			return;
		}
	
		EditorState::subregions[editIndex] = text;
	}

	close();
}

void SubregionNewPopup::reject() {
	close();
}

void SubregionNewPopup::close() {
	Popups::removePopup(this);

	window->removeKeyCallback(this, keyCallback);
}

void SubregionNewPopup::keyCallback(void *object, int action, int key) {
	SubregionNewPopup *popup = static_cast<SubregionNewPopup*>(object);
	
	if (popup->minimized) return;

	if (action == GLFW_PRESS) {
		if (key >= 33 && key <= 126) {
			char character = parseCharacter(key, popup->window->modifierPressed(GLFW_MOD_SHIFT));
			
			if (character == ':') return;
			if (character == '<') return;
			if (character == '>') return;

			popup->text += character;
		} else if (key == GLFW_KEY_SPACE) {
			if (!popup->text.empty()) {
				popup->text += " ";
			}
		} else if (key == GLFW_KEY_BACKSPACE) {
			if (!popup->text.empty()) {
				popup->text.pop_back();
			}
		}
	}
}