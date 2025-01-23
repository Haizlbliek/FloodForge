#include <filesystem>

#include "../math/Vector.hpp"

#ifndef GLOBALS_HPP
#define GLOBALS_HPP

#define LAYER_HIDDEN 5

#define ROOM_SNAP_NONE 0
#define ROOM_SNAP_TILE 1

extern std::string ROOM_TAGS[9];

extern int roomColours;

#include "Room.hpp"
#include "Connection.hpp"
#include "../font/Fonts.hpp"
#include "../Utils.hpp"

extern std::vector<Room*> rooms;
extern std::vector<Connection*> connections;
extern std::vector<std::string> subregions;

extern Vector2 cameraOffset;
extern Vector2 cameraScale;

#endif