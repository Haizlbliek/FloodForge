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
		InfoPopup();

		InfoPopup(std::string warningText);

		void draw() override;

		std::string PopupName() { return "InfoPopup"; }

	private:
		std::vector<std::string> warning;
};