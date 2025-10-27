#pragma once

#include "../../gl.h"

#include "../../popup/Popups.hpp"

class ColorEditPopup : public Popup {
	public:
		ColorEditPopup(Color &color);

		void draw() override;

		std::string PopupName() { return "ColorEditPopup"; }

	private:
		Color &color;
		float hue;
};