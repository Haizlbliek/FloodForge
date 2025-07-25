#include "AcronymPopup.hpp"

#include "MenuItems.hpp"

AcronymPopup::AcronymPopup(Window *window) : Popup(window) {
	window->addKeyCallback(this, keyCallback);

	bounds = Rect(-0.25, -0.08, 0.25, 0.25);

	text = "";
}

void AcronymPopup::draw(double mouseX, double mouseY, bool mouseInside, Vector2 screenBounds) {
	Popup::draw(mouseX, mouseY, mouseInside, screenBounds);
	
	if (minimized) return;

	double centreX = (bounds.x0 + bounds.x1) * 0.5;

	setThemeColour(ThemeColour::Text);
	glLineWidth(1);
	Fonts::rainworld->writeCentred("Enter Region Acronym:", centreX, bounds.y1 - 0.07, 0.035, CENTRE_X);

	if (text.length() < 2) {
		Draw::color(1.0, 0.0, 0.0);
	} else {
		setThemeColour(ThemeColour::Text);
	}
	Fonts::rainworld->writeCentred(text, centreX, bounds.y1 - 0.13, 0.055, CENTRE_X);

	setThemeColour(ThemeColour::Button);
	fillRect(centreX - 0.2, bounds.y1 - 0.28, centreX - 0.05, bounds.y1 - 0.22);

	if (text.length() < 2) {
		setThemeColour(ThemeColour::ButtonDisabled);
	} else {
		setThemeColour(ThemeColour::Button);
	}
	fillRect(centreX + 0.05, bounds.y1 - 0.28, centreX + 0.2, bounds.y1 - 0.22);

	setThemeColour(ThemeColour::Text);
	Fonts::rainworld->writeCentred("Cancel", centreX - 0.125, bounds.y1 - 0.25, 0.03, CENTRE_XY);

	if (text.length() < 2) {
		setThemeColour(ThemeColour::TextDisabled);
	} else {
		setThemeColour(ThemeColour::Text);
	}
	Fonts::rainworld->writeCentred("Confirm", centreX + 0.125, bounds.y1 - 0.25, 0.03, CENTRE_XY);

	if (Rect(centreX - 0.2, bounds.y1 - 0.28, centreX - 0.05, bounds.y1 - 0.22).inside(mouseX, mouseY)) {
		setThemeColour(ThemeColour::BorderHighlight);
	} else {
		setThemeColour(ThemeColour::Border);
	}
	strokeRect(centreX - 0.2, bounds.y1 - 0.28, centreX - 0.05, bounds.y1 - 0.22);

	if (Rect(centreX + 0.05, bounds.y1 - 0.28, centreX + 0.2, bounds.y1 - 0.22).inside(mouseX, mouseY)) {
		setThemeColour(ThemeColour::BorderHighlight);
	} else {
		setThemeColour(ThemeColour::Border);
	}
	strokeRect(centreX + 0.05, bounds.y1 - 0.28, centreX + 0.2, bounds.y1 - 0.22);
}

void AcronymPopup::mouseClick(double mouseX, double mouseY) {
	Popup::mouseClick(mouseX, mouseY);

	double centreX = (bounds.x0 + bounds.x1) * 0.5;

	if (Rect(centreX - 0.2, bounds.y1 - 0.28, centreX - 0.05, bounds.y1 - 0.22).inside(mouseX, mouseY)) {
		reject();
	}

	if (Rect(centreX + 0.05, bounds.y1 - 0.28, centreX + 0.2, bounds.y1 - 0.22).inside(mouseX, mouseY)) {
		accept();
	}
}

void AcronymPopup::accept() {
	if (text.length() < 2) return;

	close();

	EditorState::region.reset();
	EditorState::offscreenDen = new OffscreenRoom("offscreenden" + toLower(text), "OffscreenDen" + text);
	EditorState::rooms.push_back(EditorState::offscreenDen);
	EditorState::region.acronym = toLower(text);
}

void AcronymPopup::reject() {
	close();
}

void AcronymPopup::close() {
	Popups::removePopup(this);

	window->removeKeyCallback(this, keyCallback);
}

void AcronymPopup::keyCallback(void *object, int action, int key) {
	AcronymPopup *acronymWindow = static_cast<AcronymPopup*>(object);
	
	if (acronymWindow->minimized) return;

	if (action == GLFW_PRESS) {
		if (key == GLFW_KEY_V && (acronymWindow->window->keyPressed(GLFW_KEY_LEFT_CONTROL) || acronymWindow->window->keyPressed(GLFW_KEY_RIGHT_CONTROL))) {
			std::string clipboardText = toUpper(acronymWindow->window->getClipboard());

			for (char character : clipboardText) {
				if (character == '/') continue;
				if (character == '\\') continue;
				if (character == '_') continue;

				acronymWindow->text += character;
			}
			return;
		}

		if (key >= 33 && key <= 126) {
			char character = parseCharacter(key, acronymWindow->window->modifierPressed(GLFW_MOD_SHIFT));

			if (character == '/') return;
			if (character == '\\') return;
			if (character == '_') return;

			acronymWindow->text += character;
		}

		if (key == GLFW_KEY_BACKSPACE) {
			if (!acronymWindow->text.empty()) acronymWindow->text.pop_back();
		}
	}
}