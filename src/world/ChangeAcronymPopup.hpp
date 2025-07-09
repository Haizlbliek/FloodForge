#pragma once

#include "AcronymPopup.hpp"
#include "MenuItems.hpp"

class ChangeAcronymPopup : public AcronymPopup {
	public:
		ChangeAcronymPopup(Window *window);

		void accept();
};