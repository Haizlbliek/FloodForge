#pragma once

#include "../../gl.h"

#include "../../popup/Popups.hpp"

class ColorEditPopup : public Popup {
	public:
		ColorEditPopup(Colour &colour);

		void draw() override;

		std::string PopupName() { return "ColorEditPopup"; }

	private:
		Colour &colour;
		float hue;
};