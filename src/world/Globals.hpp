#pragma once

#include <filesystem>

#include "../math/Vector.hpp"

#define LAYER_HIDDEN 5

#define ROOM_SNAP_NONE 0
#define ROOM_SNAP_TILE 1

extern std::string ROOM_TAGS[9];
extern std::string ROOM_TAG_NAMES[9];

extern int roomColours;

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