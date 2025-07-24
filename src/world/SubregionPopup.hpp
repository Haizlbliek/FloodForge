#pragma once

#include "../Window.hpp"
#include "../Utils.hpp"

#include "../popup/Popups.hpp"
#include "../popup/InfoPopup.hpp"

#include "Room.hpp"
#include "SubregionNewPopup.hpp"

class SubregionPopup : public Popup {
	public:
		SubregionPopup(Window *window, std::set<Room*> newRooms);

		~SubregionPopup();

		void draw(double mouseX, double mouseY, bool mouseInside, Vector2 screenBounds);

		void setSubregion(int subregion);

		void mouseClick(double mouseX, double mouseY);
		
		bool canStack(std::string popupName) { return popupName == "SubregionNewPopup" || popupName == "InfoPopup"; }
		std::string PopupName() { return "SubregionPopup"; }

	private:
		std::set<Room*> rooms;

		int getButtonIndex(double mouseX, double mouseY);

		void drawSubregionButton(int subregionId, std::string subregion, double centreX, double y, double mouseX, double mouseY);
};