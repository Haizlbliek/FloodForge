#include "AcronymPopup.hpp"

#include "MenuItems.hpp"

void AcronymPopup::draw(double mouseX, double mouseY, bool mouseInside, Vector2 screenBounds) {
	Popup::draw(mouseX, mouseY, mouseInside, screenBounds);

	double centreX = (bounds.X0() + bounds.X1()) * 0.5;

	setThemeColour(ThemeColour::Text);
	glLineWidth(1);
	Fonts::rainworld->writeCentred("Enter Region Acronym:", centreX, bounds.Y1() - 0.07, 0.035, CENTRE_X);

	if (text.length() < 2) {
		Draw::color(1.0, 0.0, 0.0);
	} else if (text.length() > 2) {
		Draw::color(1.0, 1.0, 0.0);
	} else {
		setThemeColour(ThemeColour::Text);
	}
	Fonts::rainworld->writeCentred(text, centreX, bounds.Y1() - 0.13, 0.055, CENTRE_X);

	setThemeColour(ThemeColour::Button);
	fillRect(centreX - 0.2, bounds.Y1() - 0.28, centreX - 0.05, bounds.Y1() - 0.22);

	if (text.length() < 2) {
		setThemeColour(ThemeColour::ButtonDisabled);
	} else {
		setThemeColour(ThemeColour::Button);
	}
	fillRect(centreX + 0.05, bounds.Y1() - 0.28, centreX + 0.2, bounds.Y1() - 0.22);

	setThemeColour(ThemeColour::Text);
	Fonts::rainworld->writeCentred("Cancel", centreX - 0.125, bounds.Y1() - 0.25, 0.03, CENTRE_XY);

	if (text.length() < 2) {
		setThemeColour(ThemeColour::TextDisabled);
	} else {
		setThemeColour(ThemeColour::Text);
	}
	Fonts::rainworld->writeCentred("Confirm", centreX + 0.125, bounds.Y1() - 0.25, 0.03, CENTRE_XY);

	if (Rect(centreX - 0.2, bounds.Y1() - 0.28, centreX - 0.05, bounds.Y1() - 0.22).inside(mouseX, mouseY)) {
		setThemeColour(ThemeColour::BorderHighlight);
	} else {
		setThemeColour(ThemeColour::Border);
	}
	strokeRect(centreX - 0.2, bounds.Y1() - 0.28, centreX - 0.05, bounds.Y1() - 0.22);

	if (Rect(centreX + 0.05, bounds.Y1() - 0.28, centreX + 0.2, bounds.Y1() - 0.22).inside(mouseX, mouseY)) {
		setThemeColour(ThemeColour::BorderHighlight);
	} else {
		setThemeColour(ThemeColour::Border);
	}
	strokeRect(centreX + 0.05, bounds.Y1() - 0.28, centreX + 0.2, bounds.Y1() - 0.22);

	if (text.length() > 2) {
		bounds.Y0(bounds.Y1() - 0.5);
		Draw::color(1.0, 1.0, 0.0);
		Draw::translate(0.0, bounds.Y1() - 0.25);
		Fonts::rainworld->writeCentred("WARNING:", centreX, -0.055, 0.035, CENTRE_X);
		Fonts::rainworld->writeCentred("Region acronyms longer", centreX, -0.09, 0.03, CENTRE_X);
		Fonts::rainworld->writeCentred("than 2 characters behave", centreX, -0.125, 0.03, CENTRE_X);
		Fonts::rainworld->writeCentred("weirdly, to fix this", centreX, -0.16, 0.03, CENTRE_X);
		Fonts::rainworld->writeCentred("install REGION TITLE FIX", centreX, -0.195, 0.03, CENTRE_X);
		Draw::translate(0.0, -(bounds.Y1() - 0.25));
	} else {
		bounds.Y0(bounds.Y1() - 0.33);
	}
}

void AcronymPopup::mouseClick(double mouseX, double mouseY) {
	Popup::mouseClick(mouseX, mouseY);

	double centreX = (bounds.X0() + bounds.X1()) * 0.5;

	if (Rect(centreX - 0.2, bounds.Y1() - 0.28, centreX - 0.05, bounds.Y1() - 0.22).inside(mouseX, mouseY)) {
		reject();
	}

	if (Rect(centreX + 0.05, bounds.Y1() - 0.28, centreX + 0.2, bounds.Y1() - 0.22).inside(mouseX, mouseY)) {
		accept();
	}
}

void AcronymPopup::accept() {
	if (text.length() < 2) return;

	close();
	for (Room *room : rooms) delete room;
	rooms.clear();
	for (Connection *connection : connections) delete connection;
	connections.clear();
    subregions.clear();
	offscreenDen = new OffscreenRoom("offscreenden" + toLower(text), "OffscreenDen" + text);
	rooms.push_back(offscreenDen);
	MenuItems::WorldAcronym(toLower(text));
	MenuItems::extraProperties = "";
	MenuItems::extraWorld = "";
}

char AcronymPopup::parseCharacter(char character, bool shiftPressed) {
	if (!shiftPressed) return character;

	if (std::islower(character)) {
		return std::toupper(character);
	}

	switch (character) {
		case '1': return '!';
		case '2': return '@';
		case '3': return '#';
		case '4': return '$';
		case '5': return '%';
		case '6': return '^';
		case '7': return '&';
		case '8': return '*';
		case '9': return '(';
		case '0': return ')';
		case '`': return '~';
		case '-': return '_';
		case '=': return '+';
		case '[': return '{';
		case ']': return '}';
		case ';': return ':';
		case '\'': return '"';
		case '\\': return '|';
		case ',': return '<';
		case '.': return '>';
		case '/': return '?';
	}

	return character;
}

void AcronymPopup::keyCallback(void *object, int action, int key) {
	AcronymPopup *acronymWindow = static_cast<AcronymPopup*>(object);

	if (action == GLFW_PRESS) {
		if (key == GLFW_KEY_V && (acronymWindow->window->keyPressed(GLFW_KEY_LEFT_CONTROL) || acronymWindow->window->keyPressed(GLFW_KEY_RIGHT_CONTROL))) {
			std::string clipboardText = toUpper(acronymWindow->window->getClipboard());

			for (char character : clipboardText) {
				if (character == '/') continue;
				if (character == '\\') continue;

				acronymWindow->text += character;
			}
			return;
		}

		if (key >= 33 && key <= 126) {
			char character = parseCharacter(key, acronymWindow->window->keyPressed(GLFW_KEY_LEFT_SHIFT) || acronymWindow->window->keyPressed(GLFW_KEY_RIGHT_SHIFT));

			if (character == '/') return;
			if (character == '\\') return;

			acronymWindow->text += character;
		}

		if (key == GLFW_KEY_BACKSPACE) {
			if (!acronymWindow->text.empty()) acronymWindow->text.pop_back();
		}
	}
}