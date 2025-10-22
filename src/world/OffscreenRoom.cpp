#include "OffscreenRoom.hpp"

#include "../math/Rect.hpp"

#include "Globals.hpp"
#include "CreatureTextures.hpp"

bool OffscreenRoom::isOffscreen() {
	return true;
}

OffscreenRoom::OffscreenRoom(std::string path, std::string name) {
	this->path = path;
	this->roomName = name;

	canonPosition = new Vector2(
		0.0f,
		0.0f
	);
	devPosition = new Vector2(
		0.0f,
		0.0f
	);

	width = 72;
	height = 43;
	cameras = 0;

	valid = true;
	
	geometry = nullptr;

	layer = 0;
	water = -1;
	subregion = -1;
	
	tags.push_back("OffscreenRoom");
	
	data = ExtraRoomData();
}

Den &OffscreenRoom::getDen() {
	if (dens.size() == 0) {
		dens.push_back(Den());
		denShortcutEntrances.push_back({ 0, 0 });
	}

	return dens[0];
}

void OffscreenRoom::draw(Vector2 mousePosition, PositionType positionType) {
	Vector2 &position = positionType == PositionType::CANON ? canonPosition : devPosition;

	Draw::color(RoomHelpers::RoomAir);
	fillRect(position.x, position.y, position.x + width, position.y - height);

	Draw::color(RoomHelpers::RoomSolid);
	Fonts::rainworld->writeCentered(this->roomName, position.x + (width * 0.5), position.y - (height * 0.5), 5, CENTER_XY);

	getDen();
	Room::drawDen(
		dens[0],
		position.x + width * 0.5,
		position.y - height * 0.25,
		0 == hoveredDen
	);

	if (inside(mousePosition)) {
		Draw::color(0.00f, 0.75f, 0.00f);
	} else {
		Draw::color(0.75f, 0.75f, 0.75f);
	}
	strokeRect(position.x, position.y, position.x + width, position.y - height);
}