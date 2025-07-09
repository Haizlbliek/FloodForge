#pragma once

#include "../popup/Popups.hpp"

#include "DenPopup.hpp"

class RoomAttractivenessPopup : public Popup {
	public:
		RoomAttractivenessPopup(Window *window, std::set<Room *> rooms);

		~RoomAttractivenessPopup();

		void close();

		void draw(double mouseX, double mouseY, bool mouseInside, Vector2 screenBounds);

		void mouseClick(double mouseX, double mouseY);

		static void scrollCallback(void *object, double deltaX, double deltaY);

		std::string PopupName() { return "RoomAttractivenessPopup"; }

	private:
		const static RoomAttractiveness attractivenessIds[6];
		const static Colour attractivenessColors[6];
		const static std::string attractivenessNames[6];

		RoomAttractiveness selectAttractiveness = RoomAttractiveness::NEUTRAL;
		std::set<Room *> rooms;

		double currentScroll;
		double targetScroll;

		void clampScroll();
		
		void setAllTo(RoomAttractiveness attr, std::string creature);
};