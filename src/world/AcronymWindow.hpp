#ifndef ACRONYM_WINDOW_HPP
#define ACRONYM_WINDOW_HPP

#include "../gl.h"

#include <iostream>
#include <algorithm>
#include <cctype>

#include "../Window.hpp"
#include "../Theme.hpp"

#include "Globals.hpp"
#include "Room.hpp"
#include "Popups.hpp"

class AcronymWindow : public Popup {
	public:
		AcronymWindow(Window *window) : Popup(window) {
			window->addKeyCallback(this, keyCallback);

			bounds = Rect(-0.25, -0.08, 0.25, 0.25);

			text = "";
		}

		void draw(double mouseX, double mouseY, bool mouseInside) {
			Popup::draw(mouseX, mouseY, mouseInside);

			setThemeColour(THEME_TEXT_COLOUR);
			glLineWidth(1);
			Fonts::rainworld->writeCentred("Enter Region Acronym:", 0.0, 0.18, 0.035, CENTRE_X);

			if (text.length() < 2) {
				glColor3f(1.0, 0.0, 0.0);
			} else if (text.length() > 2) {
				glColor3f(1.0, 1.0, 0.0);
			} else {
				setThemeColour(THEME_TEXT_COLOUR);
			}
			Fonts::rainworld->writeCentred(text, 0.0, 0.12, 0.055, CENTRE_X);

			setThemeColour(THEME_BUTTON_COLOUR);
			fillrect(-0.2,  -0.03, -0.05, 0.03);

			if (text.length() < 2) {
				setThemeColour(THEME_BUTTON_DISABLED_COLOUR);
			} else {
				setThemeColour(THEME_BUTTON_COLOUR);
			}
			fillrect( 0.05, -0.03,  0.2,  0.03);

			setThemeColour(THEME_TEXT_COLOUR);
			Fonts::rainworld->writeCentred("Cancel", -0.125, 0.0, 0.03, CENTRE_XY);

			if (text.length() < 2) {
				setThemeColour(THEME_TEXT_DISABLED_COLOUR);
			} else {
				setThemeColour(THEME_TEXT_COLOUR);
			}
			Fonts::rainworld->writeCentred("Confirm", 0.125, 0.0, 0.03, CENTRE_XY);

			if (Rect(-0.2, -0.03, -0.05, 0.03).inside(mouseX, mouseY)) {
				setThemeColour(THEME_BORDER_HIGHLIGHT_COLOUR);
				strokerect(-0.2, -0.03, -0.05, 0.03);
			} else {
				setThemeColour(THEME_BORDER_COLOUR);
				strokerect(-0.2, -0.03, -0.05, 0.03);
			}

			if (Rect(0.05, -0.03, 0.2, 0.03).inside(mouseX, mouseY)) {
				setThemeColour(THEME_BORDER_HIGHLIGHT_COLOUR);
				strokerect(0.05, -0.03, 0.2, 0.03);
			} else {
				setThemeColour(THEME_BORDER_COLOUR);
				strokerect(0.05, -0.03, 0.2, 0.03);
			}

			if (text.length() > 2) {
				bounds.Y0(-0.25);
				glColor3f(1.0, 1.0, 0.0);
				Fonts::rainworld->writeCentred("WARNING:", 0.0, -0.055, 0.035, CENTRE_X);
				Fonts::rainworld->writeCentred("Regions acronyms longer", 0.0, -0.09, 0.03, CENTRE_X);
				Fonts::rainworld->writeCentred("than 2 characters behave", 0.0, -0.125, 0.03, CENTRE_X);
				Fonts::rainworld->writeCentred("weirdly, to fix this", 0.0, -0.16, 0.03, CENTRE_X);
				Fonts::rainworld->writeCentred("install REGION TITLE FIX", 0.0, -0.195, 0.03, CENTRE_X);
			} else {
				bounds.Y0(-0.08);
			}
		}

		void mouseClick(double mouseX, double mouseY);

		void accept();

		void reject() {
			close();
		}

		void close() {
			removePopup(this);

			window->removeKeyCallback(this, keyCallback);
		}

		static char parseCharacter(char character, bool shiftPressed) {
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

		static void keyCallback(void *object, int action, int key) {
			AcronymWindow *acronymWindow = static_cast<AcronymWindow*>(object);

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

	private:
		std::string text;
};

#endif