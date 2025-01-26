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
#include "../popup/Popups.hpp"

class AcronymPopup : public Popup {
	public:
		AcronymPopup(Window *window) : Popup(window) {
			window->addKeyCallback(this, keyCallback);

			bounds = Rect(-0.25, -0.08, 0.25, 0.25);

			text = "";
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

		static char parseCharacter(char character, bool shiftPressed);

		static void keyCallback(void *object, int action, int key);

	private:
		std::string text;
};

#endif