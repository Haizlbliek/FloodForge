#pragma once

#include "../../Window.hpp"
#include "../../Theme.hpp"
#include "../../ui/UI.hpp"

#include "../Globals.hpp"
#include "../Room.hpp"
#include "../../popup/Popups.hpp"

class CreateRoomPopup : public Popup {
	public:
		CreateRoomPopup();

		void draw() override;

		void close() override;

		std::string PopupName() { return "CreateRoomPopup"; }

		bool canStack(std::string popupName) { return popupName == "InfoPopup"; }

	private:
		bool init = true;
};