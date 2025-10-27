#pragma once

#include <filesystem>
#include <vector>

#include "../math/Vector.hpp"
#include "../Window.hpp"

// For backwards-compatibility
#define LAYER_HIDDEN 5

#define LAYER_COUNT 3
#define ROOM_TAG_COUNT 9

// Enums
#define ROOM_SNAP_NONE 0
#define ROOM_SNAP_TILE 1



extern std::string ROOM_TAGS[ROOM_TAG_COUNT];
extern std::string ROOM_TAG_NAMES[ROOM_TAG_COUNT];

#include "Room.hpp"
#include "OffscreenRoom.hpp"
#include "Connection.hpp"
#include "Region.hpp"
#include "PositionType.hpp"

namespace EditorState {
	extern Texture *geometry;

	extern bool dropletOpen;

	// FloodForge
	extern Vector2 cameraOffset;
	extern double cameraScale;
	extern double selectorScale;

	extern OffscreenRoom* offscreenDen;
	extern std::vector<Room*> rooms;
	extern std::vector<Connection*> connections;
	extern std::vector<std::string> subregions;
	extern int screenCount;

	extern Vector2 placingRoomPosition;
	extern Vector2i placingRoomSize;
	extern bool placingRoom;

	extern int roomColors;
	extern PositionType positionType;
	extern bool visibleLayers[LAYER_COUNT];
	extern bool visibleDevItems;

	extern Region region;

	extern int selectingState;
	extern std::set<Room*> selectedRooms;
	extern Room *roomPossibleSelect;

	extern std::vector<std::string> fails;

	extern bool denPopupLineageExtended;
	extern bool denPopupTagsExtended;

	extern Vector2 globalMouse;
	extern double lineSize;


	extern bool showAnniversary;
}