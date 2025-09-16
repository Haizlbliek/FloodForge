#pragma once

#include "AcronymPopup.hpp"
#include "../MenuItems.hpp"

class ChangeAcronymPopup : public AcronymPopup {
	public:
		ChangeAcronymPopup();

		void submit(std::string acronym) override;
};