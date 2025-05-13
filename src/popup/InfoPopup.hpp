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
		InfoPopup(Window *window)
		: Popup(window) {
			bounds = Rect(-0.9, -0.1, 0.9, 0.1);
		}

		InfoPopup(Window *window, std::string warningText)
		: Popup(window) {
			std::istringstream stream(warningText);
			std::string line;

			while (std::getline(stream, line)) {
				warning.push_back(line);
			}

			double height = std::max(0.2, warning.size() * 0.05 + 0.07);
			bounds = Rect(-0.9, -height * 0.5, 0.9, height * 0.5);
		}

		void draw(double mouseX, double mouseY, bool mouseInside, Vector2 screenBounds) {
			Popup::draw(mouseX, mouseY, mouseInside, screenBounds);
			
			if (minimized) return;

			setThemeColour(ThemeColour::Text);

			int lineId = 0;
			for (std::string line : warning) {
				double y = -((lineId - warning.size() * 0.5) * 0.05) - 0.04 + (bounds.Y0() + bounds.Y1()) * 0.5;
				Fonts::rainworld->writeCentred(line, (bounds.X0() + bounds.X1()) * 0.5, y, 0.04, CENTRE_XY);

				lineId++;
			}
		}
		std::string PopupName() { return "InfoPopup"; }

	private:
		std::vector<std::string> warning;
};