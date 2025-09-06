#pragma once

#include "../gl.h"

#include <iostream>
#include <string>
#include <vector>
#include <sstream>

#include "../Window.hpp"
#include "../font/Fonts.hpp"
#include "../Theme.hpp"

#include "Popups.hpp"

class InfoPopup : public Popup {
	public:
		InfoPopup(Window *window);

		InfoPopup(Window *window, std::string warningText);

		void draw(double mouseX, double mouseY, bool mouseInside, Vector2 screenBounds);

		std::string PopupName() { return "InfoPopup"; }

	private:
		std::vector<std::string> warning;
};