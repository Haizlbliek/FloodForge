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

			bounds = Rect(-0.42, -0.1, 0.42, 0.1);
		}

		void draw(double mouseX, double mouseY, bool mouseInside) {
			Popup::draw(mouseX, mouseY, mouseInside);

			setThemeColour(THEME_TEXT_COLOUR);

			int lineId = 0;
			for (std::string line : warning) {
				double y = -((lineId - warning.size() * 0.5) * 0.05) - 0.04;
				Fonts::rainworld->writeCentred(line, 0.0, y, 0.04, CENTRE_XY);

				lineId++;
			}
		}

	private:
		std::vector<std::string> warning;
};

#endif