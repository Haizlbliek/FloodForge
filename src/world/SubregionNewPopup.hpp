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
		SubregionNewPopup(Window *window, std::set<Room*> rooms, int editIndex = -1) : Popup(window), rooms(rooms), editIndex(editIndex) {
			window->addKeyCallback(this, keyCallback);

			bounds = Rect(-0.4, -0.08, 0.4, 0.25);

			text = editIndex == -1 ? "" : subregions[editIndex];
		}

		void draw(double mouseX, double mouseY, bool mouseInside, Vector2 screenBounds);

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

			return std::toupper(character);
		}

		static void keyCallback(void *object, int action, int key) {
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
					if (!popup->text.empty())
						popup->text += " ";
				} else if (key == GLFW_KEY_BACKSPACE) {
					if (!popup->text.empty()) popup->text.pop_back();
				}
			}
		}
		
		bool canStack(std::string popupName) { return false; }
		std::string PopupName() { return "SubregionNewPopup"; }

	private:
		std::string text;

		std::set<Room*> rooms;
		
		int editIndex;
};