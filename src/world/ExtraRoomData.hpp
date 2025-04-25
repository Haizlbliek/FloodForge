#pragma once

#include <vector>
#include <string>
#include "../math/Vector.hpp"

#include "../gl.h"

class DevItem {
	public:
		DevItem() {}
		
		std::string name = "";
		GLuint texture = 0;
		Vector2 position = Vector2(0.0, 0.0);
};

class ExtraRoomData {
	public:
		ExtraRoomData() {}
		
		bool hidden = false;
		bool merge = false;
		
		std::vector<DevItem> devItems;
};