#include <filesystem>

#ifndef GLOBALS_HPP
#define GLOBALS_HPP

extern bool debugRoomConnections;

extern int roomColours;

#include "Room.hpp"
#include "Connection.hpp"
#include "../font/Fonts.hpp"
#include "../Utils.hpp"

extern std::vector<Room*> rooms;
extern std::vector<Connection*> connections;
extern std::vector<std::string> subregions;

#endif