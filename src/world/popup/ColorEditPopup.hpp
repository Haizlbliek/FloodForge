#pragma once

#include "../../gl.h"

#include "../../popup/Popups.hpp"

class ColorEditPopup : public Popup {
	public:
		ColorEditPopup(Color &color);

		ColorEditPopup(Color &color, std::function<void(Color oldColor, Color &color)> callback);

		void draw() override;

		void close() override;

		std::string PopupName() { return "ColorEditPopup"; }

	private:
		Color oldColor;
		Color &color;
		float hue;
		std::function<void(Color oldColor, Color &color)> callback;
};