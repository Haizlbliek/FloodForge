#pragma once

#include <filesystem>
#include <vector>

#include "../math/Vector.hpp"
#include "../Window.hpp"

// For backwards-compatibility
#define LAYER_HIDDEN 5

#define ROOM_SNAP_NONE 0
#define ROOM_SNAP_TILE 1

#define LAYER_COUNT 3

extern std::string ROOM_TAGS[9];
extern std::string ROOM_TAG_NAMES[9];

#include "Room.hpp"
#include "OffscreenRoom.hpp"
#include "Connection.hpp"
#include "Region.hpp"

namespace EditorState {
	extern Mouse *mouse;
	extern Window *window;

	extern Vector2 cameraOffset;
	extern double cameraScale;
	extern double selectorScale;

	extern OffscreenRoom* offscreenDen;
	extern std::vector<Room*> rooms;
	extern std::vector<Connection*> connections;
	extern std::vector<std::string> subregions;

	extern int roomColours;
	extern bool visibleLayers[3];
	extern bool visibleDevItems;
	
	extern Region region;

	extern int selectingState;
	extern std::set<Room*> selectedRooms;
	extern Room *roomPossibleSelect;
}