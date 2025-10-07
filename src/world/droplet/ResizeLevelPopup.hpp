#pragma once

#include "../../popup/Popups.hpp"

class ResizeLevelPopup : public Popup {
	public:
		ResizeLevelPopup();

		void draw() override;

		void close() override;

		std::string PopupName() { return "ResizeLevelPopup"; }

	private:
		bool initial = true;
};