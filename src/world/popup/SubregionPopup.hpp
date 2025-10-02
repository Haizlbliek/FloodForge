#pragma once

#include "../../Window.hpp"
#include "../../Utils.hpp"

#include "../../popup/Popups.hpp"
#include "../../popup/InfoPopup.hpp"

#include "../Room.hpp"
#include "SubregionNewPopup.hpp"

class SubregionPopup : public Popup {
	public:
		SubregionPopup(std::set<Room*> newRooms);

		~SubregionPopup();

		void draw();

		void setSubregion(int subregion);
		
		void close();
		
		bool canStack(std::string popupName) { return popupName == "SubregionNewPopup" || popupName == "InfoPopup" || popupName == "ColorEditPopup"; }
		std::string PopupName() { return "SubregionPopup"; }

	private:
		std::set<Room*> rooms;

		int getButtonIndex(double mouseX, double mouseY);

		void drawSubregionButton(int subregionId, std::string subregion, double centreX, double y);

		static void scrollCallback(void *object, double deltaX, double deltaY);

		void clampScroll();

		double scroll;
		double scrollTo;
};