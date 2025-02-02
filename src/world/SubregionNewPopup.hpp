#pragma once

#include "../gl.h"

#include <iostream>
#include <algorithm>
#include <cctype>

#include "../Window.hpp"
#include "../Theme.hpp"

#include "../popup/Popups.hpp"

#include "Globals.hpp"
#include "Room.hpp"

class SubregionNewPopup : public Popup {
	public:
		SubregionNewPopup(Window *window, std::set<Room*> rooms) : Popup(window), rooms(rooms) {
			window->addKeyCallback(this, keyCallback);

			bounds = Rect(-0.25, -0.08, 0.25, 0.25);

			text = "";
		}

		void draw(double mouseX, double mouseY, bool mouseInside, Vector2 screenBounds) {
			Popup::draw(mouseX, mouseY, mouseInside, screenBounds);

			mouseX -= bounds.X0() + 0.25;
			mouseY -= bounds.Y0() + 0.08;

			Draw::pushMatrix();

			Draw::translate(bounds.X0() + 0.25, bounds.Y0() + 0.08);

			setThemeColour(ThemeColour::Text);
			glLineWidth(1);
			Fonts::rainworld->writeCentred("Enter Subregion Name:", 0.0, 0.18, 0.035, CENTRE_X);

			if (text.length() < 2) {
				Draw::color(1.0, 0.0, 0.0);
			} else if (text.length() > 2) {
				Draw::color(1.0, 1.0, 0.0);
			} else {
				setThemeColour(ThemeColour::Text);
			}
			Fonts::rainworld->writeCentred(text, 0.0, 0.12, 0.055, CENTRE_X);

			setThemeColour(ThemeColour::Button);
			fillRect(-0.2,  -0.03, -0.05, 0.03);

			if (text.length() < 2) {
				setThemeColour(ThemeColour::ButtonDisabled);
			} else {
				setThemeColour(ThemeColour::Button);
			}
			fillRect( 0.05, -0.03,  0.2,  0.03);

			setThemeColour(ThemeColour::Text);
			Fonts::rainworld->writeCentred("Cancel", -0.125, 0.0, 0.03, CENTRE_XY);

			if (text.length() < 2) {
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

			if (text.length() > 2) {
				// bounds.Y0(-0.25);
				// Draw::color(1.0, 1.0, 0.0);
				// Fonts::rainworld->writeCentred("WARNING:", 0.0, -0.055, 0.035, CENTRE_X);
				// Fonts::rainworld->writeCentred("Regions acronyms longer", 0.0, -0.09, 0.03, CENTRE_X);
				// Fonts::rainworld->writeCentred("than 2 characters behave", 0.0, -0.125, 0.03, CENTRE_X);
				// Fonts::rainworld->writeCentred("weirdly, to fix this", 0.0, -0.16, 0.03, CENTRE_X);
				// Fonts::rainworld->writeCentred("install REGION TITLE FIX", 0.0, -0.195, 0.03, CENTRE_X);
			} else {
				bounds.Y0(-0.08);
			}

			Draw::popMatrix();
		}

		void mouseClick(double mouseX, double mouseY);

		void accept();

		void reject() {
			close();
		}

		void close() {
			Popups::removePopup(this);

			window->removeKeyCallback(this, keyCallback);
		}

		static char parseCharacter(char character, bool shiftPressed) {
			if (!shiftPressed) return std::tolower(character);

			return std::toupper(character);
		}

		static void keyCallback(void *object, int action, int key) {
			SubregionNewPopup *popup = static_cast<SubregionNewPopup*>(object);

			if (action == GLFW_PRESS) {
				if (key >= GLFW_KEY_A && key <= GLFW_KEY_Z) {
					char character = parseCharacter(key, popup->window->keyPressed(GLFW_KEY_LEFT_SHIFT) || popup->window->keyPressed(GLFW_KEY_RIGHT_SHIFT));

					popup->text += character;
				}

                if (key == GLFW_KEY_SPACE) {
                    if (!popup->text.empty())
                        popup->text += " ";
                }

				if (key == GLFW_KEY_BACKSPACE) {
					if (!popup->text.empty()) popup->text.pop_back();
				}
			}
		}
        
		bool canStack(std::string popupName) { return false; }
		std::string PopupName() { return "SubregionNewPopup"; }

	private:
		std::string text;

		std::set<Room*> rooms;
};