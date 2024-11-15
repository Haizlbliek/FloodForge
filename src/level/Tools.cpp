#include "Tools.hpp"

#include "Constants.hpp"

bool canSlopeConnect(unsigned int tile, unsigned int slope, unsigned int direction) {
	// Connecting to tile (solid, air, half-platform)
	if (tile <= 2) {
		if (tile == 2) {
			tile = (direction == 2) ? 0 : 1;
		}

		if (slope == 3) {
			if (direction == 0 || direction == 3) return tile == 1;
			if (direction == 1 || direction == 2) return tile == 0;
		}
		if (slope == 4) {
			if (direction == 0 || direction == 2) return tile == 1;
			if (direction == 1 || direction == 3) return tile == 0;
		}
		if (slope == 5) {
			if (direction == 1 || direction == 2) return tile == 1;
			if (direction == 0 || direction == 3) return tile == 0;
		}
		if (slope == 6) {
			if (direction == 1 || direction == 3) return tile == 1;
			if (direction == 0 || direction == 2) return tile == 0;
		}
	}

	// Connecting to slope
	switch (direction) {
		case 0: // Connecting left
		case 1: // Connecting right
			if (slope == 3 || slope == 4) return tile == 5 || tile == 6;
			if (slope == 5 || slope == 6) return tile == 3 || tile == 4;

		case 2: // Connecting up
		case 3: // Connecting down
			if (slope == 3 || slope == 6) return tile == 4 || tile == 5;
			if (slope == 4 || slope == 5) return tile == 3 || tile == 6;
	}

	return false;
}

bool slopeViable(Grid *grid, unsigned int tileX, unsigned int tileY, unsigned int slopeType) {
	return (
		canSlopeConnect(grid->getTile(tileX - 1, tileY), slopeType, 0) &&
		canSlopeConnect(grid->getTile(tileX + 1, tileY), slopeType, 1) &&
		canSlopeConnect(grid->getTile(tileX, tileY - 1), slopeType, 2) &&
		canSlopeConnect(grid->getTile(tileX, tileY + 1), slopeType, 3)
	);
}



Tool toolDraw = Tool{
	0,
	[](Grid *grid, unsigned int tileX, unsigned int tileY) {
		grid->setTile(tileX, tileY, 1);
	},
	[](Grid *grid, unsigned int tileX, unsigned int tileY) {
		grid->setTile(tileX, tileY, 0);
	}
};

Tool toolBorder = Tool{
	1,
	[](Grid *grid, unsigned int tileX, unsigned int tileY) {
		grid->setTile(tileX, tileY, 11);
	},
	[](Grid *grid, unsigned int tileX, unsigned int tileY) {
		grid->setTile(tileX, tileY, 0);
	}
};

Tool toolSlope = Tool{
	2,
	[](Grid *grid, unsigned int tileX, unsigned int tileY) {
		unsigned int tile = 0;

		if (slopeViable(grid, tileX, tileY, 3)) tile = 3;
		if (slopeViable(grid, tileX, tileY, 4)) tile = 4;
		if (slopeViable(grid, tileX, tileY, 5)) tile = 5;
		if (slopeViable(grid, tileX, tileY, 6)) tile = 6;

		if (tile != 0) grid->setTile(tileX, tileY, tile);
	},
	[](Grid *grid, unsigned int tileX, unsigned int tileY) {
		grid->setTile(tileX, tileY, 0);
	}
};

Tool toolOneWay = Tool{
	3,
	[](Grid *grid, unsigned int tileX, unsigned int tileY) {
		grid->setTile(tileX, tileY, 2);
	},
	[](Grid *grid, unsigned int tileX, unsigned int tileY) {
		grid->setTile(tileX, tileY, 0);
	}
};


Tool toolSpear = Tool{
	6,
	[](Grid *grid, unsigned int tileX, unsigned int tileY) {
		grid->addData(tileX, tileY, DATA_SPEAR);
	},
	[](Grid *grid, unsigned int tileX, unsigned int tileY) {
		grid->removeData(tileX, tileY, DATA_SPEAR);
	}
};

Tool toolRock = Tool{
	7,
	[](Grid *grid, unsigned int tileX, unsigned int tileY) {
		grid->addData(tileX, tileY, DATA_ROCK);
	},
	[](Grid *grid, unsigned int tileX, unsigned int tileY) {
		grid->removeData(tileX, tileY, DATA_ROCK);
	}
};


Tool toolPoleHorizontal = Tool{
	4,
	[](Grid *grid, unsigned int tileX, unsigned int tileY) {
		grid->addData(tileX, tileY, DATA_HORIZONTAL_POLE);
	},
	[](Grid *grid, unsigned int tileX, unsigned int tileY) {
		grid->removeData(tileX, tileY, DATA_HORIZONTAL_POLE);
	}
};

Tool toolPoleVertical = Tool{
	5,
	[](Grid *grid, unsigned int tileX, unsigned int tileY) {
		grid->addData(tileX, tileY, DATA_VERTICAL_POLE);
	},
	[](Grid *grid, unsigned int tileX, unsigned int tileY) {
		grid->removeData(tileX, tileY, DATA_VERTICAL_POLE);
	}
};

Tool toolBatflyHive = Tool{
	11,
	[](Grid *grid, unsigned int tileX, unsigned int tileY) {
		grid->addData(tileX, tileY, DATA_HIVE);
	},
	[](Grid *grid, unsigned int tileX, unsigned int tileY) {
		grid->removeData(tileX, tileY, DATA_HIVE);
	}
};

Tool toolWormgrass = Tool{
	12,
	[](Grid *grid, unsigned int tileX, unsigned int tileY) {
		grid->addData(tileX, tileY, DATA_WORMGRASS);
	},
	[](Grid *grid, unsigned int tileX, unsigned int tileY) {
		grid->removeData(tileX, tileY, DATA_WORMGRASS);
	}
};

Tool toolGarbageWorm = Tool{
	13,
	[](Grid *grid, unsigned int tileX, unsigned int tileY) {
		grid->addData(tileX, tileY, DATA_GARBAGE_WORM);
	},
	[](Grid *grid, unsigned int tileX, unsigned int tileY) {
		grid->removeData(tileX, tileY, DATA_GARBAGE_WORM);
	}
};

Tool toolWaterfall = Tool{
	15,
	[](Grid *grid, unsigned int tileX, unsigned int tileY) {
		grid->addData(tileX, tileY, DATA_WATERFALL);
	},
	[](Grid *grid, unsigned int tileX, unsigned int tileY) {
		grid->removeData(tileX, tileY, DATA_WATERFALL);
	}
};


Tool toolShortcut = Tool{
	8,
	[](Grid *grid, unsigned int tileX, unsigned int tileY) {
		grid->addData(tileX, tileY, DATA_SHORTCUT);
		grid->removeData(tileX, tileY, DATA_ROOM_EXIT);
		grid->removeData(tileX, tileY, DATA_LIZARD_DEN);
		grid->removeData(tileX, tileY, DATA_WACKAMOLE);
		grid->removeData(tileX, tileY, DATA_SCAVENGER_DEN);
	},
	[](Grid *grid, unsigned int tileX, unsigned int tileY) {
		grid->removeData(tileX, tileY, DATA_SHORTCUT);
		grid->removeData(tileX, tileY, DATA_ROOM_EXIT);
		grid->removeData(tileX, tileY, DATA_LIZARD_DEN);
		grid->removeData(tileX, tileY, DATA_WACKAMOLE);
		grid->removeData(tileX, tileY, DATA_SCAVENGER_DEN);
	}
};

Tool toolRoomExit = Tool{
	9,
	[](Grid *grid, unsigned int tileX, unsigned int tileY) {
		grid->addData(tileX, tileY, DATA_SHORTCUT);
		grid->addData(tileX, tileY, DATA_ROOM_EXIT);
		grid->removeData(tileX, tileY, DATA_LIZARD_DEN);
		grid->removeData(tileX, tileY, DATA_WACKAMOLE);
		grid->removeData(tileX, tileY, DATA_SCAVENGER_DEN);
	},
	[](Grid *grid, unsigned int tileX, unsigned int tileY) {
		grid->removeData(tileX, tileY, DATA_SHORTCUT);
		grid->removeData(tileX, tileY, DATA_ROOM_EXIT);
		grid->removeData(tileX, tileY, DATA_LIZARD_DEN);
		grid->removeData(tileX, tileY, DATA_WACKAMOLE);
		grid->removeData(tileX, tileY, DATA_SCAVENGER_DEN);
	}
};

Tool toolWackAMole = Tool{
	10,
	[](Grid *grid, unsigned int tileX, unsigned int tileY) {
		grid->addData(tileX, tileY, DATA_SHORTCUT);
		grid->addData(tileX, tileY, DATA_WACKAMOLE);
		grid->removeData(tileX, tileY, DATA_ROOM_EXIT);
		grid->removeData(tileX, tileY, DATA_LIZARD_DEN);
		grid->removeData(tileX, tileY, DATA_SCAVENGER_DEN);
	},
	[](Grid *grid, unsigned int tileX, unsigned int tileY) {
		grid->removeData(tileX, tileY, DATA_SHORTCUT);
		grid->removeData(tileX, tileY, DATA_WACKAMOLE);
		grid->removeData(tileX, tileY, DATA_ROOM_EXIT);
		grid->removeData(tileX, tileY, DATA_LIZARD_DEN);
		grid->removeData(tileX, tileY, DATA_SCAVENGER_DEN);
	}
};

Tool toolLizardDen = Tool{
	14,
	[](Grid *grid, unsigned int tileX, unsigned int tileY) {
		grid->addData(tileX, tileY, DATA_SHORTCUT);
		grid->addData(tileX, tileY, DATA_LIZARD_DEN);
		grid->removeData(tileX, tileY, DATA_ROOM_EXIT);
		grid->removeData(tileX, tileY, DATA_WACKAMOLE);
		grid->removeData(tileX, tileY, DATA_SCAVENGER_DEN);
	},
	[](Grid *grid, unsigned int tileX, unsigned int tileY) {
		grid->removeData(tileX, tileY, DATA_SHORTCUT);
		grid->removeData(tileX, tileY, DATA_ROOM_EXIT);
		grid->removeData(tileX, tileY, DATA_LIZARD_DEN);
		grid->removeData(tileX, tileY, DATA_WACKAMOLE);
		grid->removeData(tileX, tileY, DATA_SCAVENGER_DEN);
	}
};

Tool toolScavengerDen = Tool{
	16,
	[](Grid *grid, unsigned int tileX, unsigned int tileY) {
		grid->addData(tileX, tileY, DATA_SHORTCUT);
		grid->addData(tileX, tileY, DATA_SCAVENGER_DEN);
		grid->removeData(tileX, tileY, DATA_ROOM_EXIT);
		grid->removeData(tileX, tileY, DATA_WACKAMOLE);
		grid->removeData(tileX, tileY, DATA_LIZARD_DEN);
	},
	[](Grid *grid, unsigned int tileX, unsigned int tileY) {
		grid->removeData(tileX, tileY, DATA_SHORTCUT);
		grid->removeData(tileX, tileY, DATA_ROOM_EXIT);
		grid->removeData(tileX, tileY, DATA_LIZARD_DEN);
		grid->removeData(tileX, tileY, DATA_WACKAMOLE);
		grid->removeData(tileX, tileY, DATA_SCAVENGER_DEN);
	}
};