#pragma once

#include "Room.hpp"

class OffscreenRoom : public Room {
	public:
		bool isOffscreen() override;

		OffscreenRoom(std::string path, std::string name);

		~OffscreenRoom();

		int AddDen();

		void cleanup();

		int denAt(double mouseX, double mouseY);

		void draw(Vector2 mousePosition, double lineSize, Vector2 screenBounds, int positionType) override;
};