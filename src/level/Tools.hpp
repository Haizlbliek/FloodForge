#ifndef TOOLS_HPP
#define TOOLS_HPP

#include <functional>

#include "Grid.hpp"

using ToolAction = std::function<void(Grid*, unsigned int, unsigned int)>;

struct Tool {
	unsigned int iconId;
	ToolAction left;
	ToolAction right;
};

// Standard Solids
extern Tool toolDraw;
extern Tool toolBorder;
extern Tool toolSlope;
extern Tool toolOneWay;

// Items
extern Tool toolSpear;
extern Tool toolRock;

// Data
extern Tool toolPoleHorizontal;
extern Tool toolPoleVertical;
extern Tool toolBatflyHive;
extern Tool toolWormgrass;
extern Tool toolGarbageWorm;
extern Tool toolWaterfall;

// Shortcuts & Exits
extern Tool toolShortcut;
extern Tool toolRoomExit;
extern Tool toolWackAMole;
extern Tool toolLizardDen;
extern Tool toolScavengerDen;

#endif