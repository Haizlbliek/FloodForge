#pragma once

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
		AcronymPopup(Window *window);

		void draw(double mouseX, double mouseY, bool mouseInside, Vector2 screenBounds);

		void mouseClick(double mouseX, double mouseY);

		void accept();

		void reject();

		void close();

		static void keyCallback(void *object, int action, int key);

	protected:
		std::string text;
};