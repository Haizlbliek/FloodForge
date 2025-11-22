#pragma once

#include "../../math/Vector.hpp"

#include "../../popup/Popups.hpp"
#include "../Connection.hpp"
#include "../Room.hpp"

#include "../History.hpp"

namespace FloodForgeWindow {
	void updateCamera();

	void updateMain();

	void Draw();

	extern Vector2 worldMouse;
	extern History history;
}