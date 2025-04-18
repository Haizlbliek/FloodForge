#pragma once

#include <filesystem>

#include "../math/Vector.hpp"

// For backwards-compatibility
#define LAYER_HIDDEN 5

#define ROOM_SNAP_NONE 0
#define ROOM_SNAP_TILE 1

#define LAYER_1 0
#define LAYER_2 1
#define LAYER_3 2

extern std::string ROOM_TAGS[9];
extern std::string ROOM_TAG_NAMES[9];

extern int roomColours;
extern bool visibleLayers[3];

#include "Room.hpp"
#include "OffscreenRoom.hpp"
#include "Connection.hpp"
#include "../font/Fonts.hpp"
#include "../Utils.hpp"

extern OffscreenRoom* offscreenDen;
extern std::vector<Room*> rooms;
extern std::vector<Connection*> connections;
extern std::vector<std::string> subregions;

extern Vector2 cameraOffset;
extern double cameraScale;