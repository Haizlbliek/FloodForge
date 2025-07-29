#pragma once

#include <vector>
#include <string>
#include <unordered_map>

#include "RoomAttractiveness.hpp"
#include "../math/Vector.hpp"

class DevItem {
	public:
		DevItem() = default;
		
		std::string name = "";
		GLuint texture = 0;
		Vector2 position = Vector2(0.0, 0.0);
};

class ExtraRoomData {
	public:
		ExtraRoomData() = default;
		
		bool hidden = false;
		bool merge = true;
		
		std::vector<DevItem> devItems;
		
		std::unordered_map<std::string, RoomAttractiveness> attractiveness;
		
		bool empty() {
			return !hidden && merge;
		}
};