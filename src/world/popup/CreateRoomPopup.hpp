#pragma once

#include "../../Window.hpp"
#include "../../Theme.hpp"
#include "../../ui/UI.hpp"

#include "../Globals.hpp"
#include "../Room.hpp"
#include "../../popup/Popups.hpp"

class CreateRoomPopup : public Popup {
	public:
		CreateRoomPopup(Window *window);

		void draw(double mouseX, double mouseY, bool mouseInside, Vector2 screenBounds);

		void close();

		std::string PopupName() { return "CreateRoomPopup"; }

		bool canStack(std::string popupName) { return popupName == "InfoPopup"; }
};