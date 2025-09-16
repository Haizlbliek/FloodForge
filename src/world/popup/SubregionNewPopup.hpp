#pragma once

#include "../../gl.h"

#include <iostream>
#include <algorithm>
#include <cctype>

#include "../../Window.hpp"
#include "../../Theme.hpp"

#include "../../popup/Popups.hpp"
#include "AcronymPopup.hpp"

#include "../Globals.hpp"
#include "../Room.hpp"

class SubregionNewPopup : public AcronymPopup {
	public:
		SubregionNewPopup(std::set<Room*> rooms, int editIndex = -1);

		void submit(std::string acronym) override;

		std::string banLetters() override { return ":<>"; };

		bool canStack(std::string popupName) { return false; }
		std::string PopupName() { return "SubregionNewPopup"; }

	private:
		std::set<Room*> rooms;

		int editIndex;
};