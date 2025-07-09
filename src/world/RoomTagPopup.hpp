#pragma once

#include "../Window.hpp"
#include "../Utils.hpp"

#include "../popup/Popups.hpp"

#include "Room.hpp"

class RoomTagPopup : public Popup {
	public:
		RoomTagPopup(Window *window, std::set<Room*> newRooms);

		void draw(double mouseX, double mouseY, bool mouseInside, Vector2 screenBounds);

		void setTag(std::string tag);

		void toggleTag(std::string tag);

		void mouseClick(double mouseX, double mouseY);
		
		std::string PopupName() { return "RoomTagPopup"; }

	private:
		std::set<Room*> rooms;

		int getButtonIndex(double mouseX, double mouseY);

		void drawTagButton(std::string tag, std::string tagId, double y, double mouseX, double mouseY);
};