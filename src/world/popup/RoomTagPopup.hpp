#pragma once

#include "../../Window.hpp"
#include "../../Utils.hpp"

#include "../../popup/Popups.hpp"

#include "../Room.hpp"

class RoomTagPopup : public Popup {
	public:
		RoomTagPopup(std::set<Room*> newRooms);

		void draw();

		void setTag(std::string tag);

		void toggleTag(std::string tag);
		
		std::string PopupName() { return "RoomTagPopup"; }

	private:
		std::set<Room*> rooms;

		void drawTagButton(std::string tag, std::string tagId, double y);
};