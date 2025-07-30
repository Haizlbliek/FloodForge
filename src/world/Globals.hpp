#pragma once

#include <filesystem>
#include <vector>

#include "../math/Vector.hpp"
#include "../Window.hpp"

#define FLOODFORGE_VERSION "v1.5.4"

// For backwards-compatibility
#define LAYER_HIDDEN 5

#define LAYER_COUNT 3
#define ROOM_TAG_COUNT 9

// Enums
#define ROOM_SNAP_NONE 0
#define ROOM_SNAP_TILE 1

#define CANON_POSITION 2
#define DEV_POSITION 3


extern std::string ROOM_TAGS[ROOM_TAG_COUNT];
extern std::string ROOM_TAG_NAMES[ROOM_TAG_COUNT];

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
	extern int roomPositionType;
	extern bool visibleLayers[LAYER_COUNT];
	extern bool visibleDevItems;
	
	extern Region region;

	extern int selectingState;
	extern std::set<Room*> selectedRooms;
	extern Room *roomPossibleSelect;
	
	extern std::vector<std::string> fails;
}