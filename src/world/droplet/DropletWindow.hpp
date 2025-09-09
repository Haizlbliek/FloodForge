#pragma once

#include "../../math/Vector.hpp"

namespace DropletWindow {
	void UpdateCamera();

	void Draw();

	extern Vector2 cameraOffset;
	extern double cameraScale;
	extern double cameraScaleTo;

	extern bool cameraPanning;
	extern bool cameraPanningBlocked;
	extern Vector2 cameraPanStartMouse;
	extern Vector2 cameraPanStart;
	extern Vector2 cameraPanTo;
	extern double cameraScaleTo;

	extern Vector2i mouseTile;
	extern Vector2i lastMouseTile;
	extern bool lastMouseDrawing;
}