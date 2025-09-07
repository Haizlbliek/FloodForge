#pragma once

#include "Room.hpp"

class OffscreenRoom : public Room {
	public:
		bool isOffscreen() override;

		OffscreenRoom(std::string path, std::string name);

		Den &getDen();

		void draw(Vector2 mousePosition, int positionType) override;
};