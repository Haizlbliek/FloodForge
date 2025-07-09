#pragma once

#include "../Window.hpp"
#include "../font/Fonts.hpp"
#include "../Theme.hpp"

#include "Globals.hpp"

namespace DebugData {
	void draw(Window *window, Vector2 mouse, double lineSize, Vector2 screenBounds);
}