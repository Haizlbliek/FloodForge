#pragma once

#include "../../popup/Popups.hpp"
#include "../RoomAttractiveness.hpp"
#include "../Room.hpp"

class RoomAttractivenessPopup : public Popup {
	public:
		RoomAttractivenessPopup(std::set<Room *> rooms);

		~RoomAttractivenessPopup();

		void close() override;

		void draw() override;

		static void scrollCallback(void *object, double deltaX, double deltaY);

		std::string PopupName() { return "RoomAttractivenessPopup"; }

	private:
		const static RoomAttractiveness attractivenessIds[6];
		const static Color attractivenessColors[6];
		const static std::string attractivenessNames[6];

		RoomAttractiveness selectAttractiveness = RoomAttractiveness::NEUTRAL;
		std::set<Room *> rooms;

		double currentScroll;
		double targetScroll;

		void clampScroll();
		
		void setAllTo(RoomAttractiveness attr, std::string creature);
};