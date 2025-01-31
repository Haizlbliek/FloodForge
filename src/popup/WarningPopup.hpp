#ifndef WARNING_POPUP_HPP
#define WARNING_POPUP_HPP

#include "../gl.h"

#include <iostream>
#include <string>
#include <vector>
#include <sstream>

#include "../Window.hpp"

#include "Popups.hpp"

class WarningPopup : public Popup {
	public:
		WarningPopup(Window *window)
		: Popup(window) {
			bounds = Rect(-0.42, -0.1, 0.42, 0.1);
		}

		WarningPopup(Window *window, std::string warningText)
		: Popup(window) {
			std::istringstream stream(warningText);
			std::string line;

			while (std::getline(stream, line)) {
				warning.push_back(line);
			}

			double height = std::max(0.2, warning.size() * 0.05 + 0.07);
			bounds = Rect(-0.42, -height * 0.5, 0.42, height * 0.5);
		}

		void draw(double mouseX, double mouseY, bool mouseInside, Vector2 screenBounds) {
			Popup::draw(mouseX, mouseY, mouseInside, screenBounds);

			setThemeColour(ThemeColour::Text);

			int lineId = 0;
			for (std::string line : warning) {
				double y = -((lineId - warning.size() * 0.5) * 0.05) - 0.04 + (bounds.Y0() + bounds.Y1()) * 0.5;
				Fonts::rainworld->writeCentred(line, (bounds.X0() + bounds.X1()) * 0.5, y, 0.04, CENTRE_XY);

				lineId++;
			}
		}
		std::string PopupName() { return "WarningPopup"; }

	private:
		std::vector<std::string> warning;
};

#endif