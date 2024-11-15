#include "LevelUtils.hpp"

#include "../stb_image.h"
#include "../stb_image_write.h"

#include <iostream>
#include <sstream>

#include "Constants.hpp"


bool isShortcutAir(unsigned int tile) {
	return tile == 0 || tile == 2;
}

int getShortcutTile(Grid *grid, unsigned int x, unsigned int y) {
	bool air00 = isShortcutAir(grid->getTile(x - 1, y - 1));
	bool air10 = isShortcutAir(grid->getTile(x + 0, y - 1));
	bool air20 = isShortcutAir(grid->getTile(x + 1, y - 1));
	bool air01 = isShortcutAir(grid->getTile(x - 1, y + 0));
	bool air11 = isShortcutAir(grid->getTile(x + 0, y + 0));
	bool air21 = isShortcutAir(grid->getTile(x + 1, y + 0));
	bool air02 = isShortcutAir(grid->getTile(x - 1, y + 1));
	bool air12 = isShortcutAir(grid->getTile(x + 0, y + 1));
	bool air22 = isShortcutAir(grid->getTile(x + 1, y + 1));

	bool shortcutL = grid->hasData(x - 1, y + 0, DATA_SHORTCUT);
	bool shortcutR = grid->hasData(x + 1, y + 0, DATA_SHORTCUT);
	bool shortcutU = grid->hasData(x + 0, y - 1, DATA_SHORTCUT);
	bool shortcutD = grid->hasData(x + 0, y + 1, DATA_SHORTCUT);

	if (!air00 && !air20 && !air02 && !air22) {
		if (air10 && !air01 && !air11 && !air21 & !air12)
			if (!shortcutL && !shortcutR && !shortcutU && shortcutD)
				return 0;

		if (!air10 && air01 && !air11 && !air21 & !air12)
			if (!shortcutL && shortcutR && !shortcutU && !shortcutD)
				return 2;

		if (!air10 && !air01 && !air11 && air21 & !air12)
			if (shortcutL && !shortcutR && !shortcutU && !shortcutD)
				return 1;

		if (!air10 && !air01 && !air11 && !air21 & air12)
			if (!shortcutL && !shortcutR && shortcutU && !shortcutD)
				return 3;
	}

	unsigned int sides = shortcutL + shortcutR + shortcutU + shortcutD;
	if (sides == 2) {
		if (shortcutU && shortcutD) return 10;
		if (shortcutL && shortcutR) return 11;

		return 5;
	} else if (sides == 4) {
		return 5;
	}

	return 6;
}

void drawTextureQuad(uint8_t tile, float x, float y, float width, float height, unsigned int tilesWidth, unsigned int tilesHeight) {
	int uvX = tile % tilesWidth;
	int uvY = tile / tilesWidth;
	float u = (uvX + 0.0) / ((float) tilesWidth);
	float v = (uvY + 0.0) / ((float) tilesHeight);
	float uWidth = 1.0 / ((float) tilesWidth);
	float vHeight = 1.0 / ((float) tilesHeight);

	glTexCoord2f(u,          v);           glVertex2f(x,         y);
	glTexCoord2f(u + uWidth, v);           glVertex2f(x + width, y);
	glTexCoord2f(u + uWidth, v + vHeight); glVertex2f(x + width, y + height);
	glTexCoord2f(u,          v + vHeight); glVertex2f(x,         y + height);
}

void drawTextureQuad(uint8_t tile, float x, float y, float width, float height) {
	drawTextureQuad(tile, x, y, width, height, TEXTURE_TILES_WIDTH, TEXTURE_TILES_HEIGHT);
}

bool isAir(uint8_t tile) {
	return tile == 0 || tile == 9 || tile == 10;
}

bool isValid(Grid *grid, uint8_t data, int x, int y) {
	if (!grid->inBoundary(x, y)) return false;

	if (data == DATA_HIVE) {
		return isAir(grid->getTile(x, y)) && grid->getTile(x, y + 1) == 1;
	}

	if (data == DATA_WORMGRASS) {
		return isAir(grid->getTile(x, y)) && grid->getTile(x, y + 1) == 1;
	}

	if (data == DATA_GARBAGE_WORM) {
		return grid->getTile(x, y) == 1 && isAir(grid->getTile(x, y - 1));
	}

	return true;
}

void colourValid(Grid *grid, uint8_t data, int x, int y, float transparency) {
	if (isValid(grid, data, x, y)) {
		glColor4f(1.0f, 1.0f, 1.0f, transparency);
	} else {
		glColor4f(1.0f, 0.0f, 0.0f, transparency);
	}
}

void colourValidBlack(Grid *grid, uint8_t data, int x, int y, float transparency) {
	if (isValid(grid, data, x, y)) {
		glColor4f(0.0f, 0.0f, 0.0f, transparency);
	} else {
		glColor4f(1.0f, 0.0f, 0.0f, transparency);
	}
}

void drawTexturedGrid(Grid *grid, float scale, GLuint solids, GLuint shortcuts, GLuint items) {
	drawTexturedGrid(grid, scale, solids, shortcuts, items, 1.0f);
}

void drawTexturedGrid(Grid *grid, float scale, GLuint solids, GLuint shortcuts, GLuint items, float transparency) {
	glBindTexture(GL_TEXTURE_2D, solids);

	glEnable(GL_TEXTURE_2D);
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	glBegin(GL_QUADS);

	glColor4f(0.0, 0.0, 0.0, transparency);

	for (float x = 0; x < grid->Width(); x++) {
		for (float y = 0; y < grid->Height(); y++) {
			uint8_t tile = grid->getTile(x, y);

			drawTextureQuad(tile, -1.0 + x * scale, 1.0 - y * scale, scale, -scale);
		}
	}

	for (float x = 0; x < grid->Width(); x++) {
		for (float y = 0; y < grid->Height(); y++) {;
			if (grid->hasData(x, y, DATA_HORIZONTAL_POLE)) {
				colourValidBlack(grid, DATA_HORIZONTAL_POLE, x, y, transparency);
				drawTextureQuad(7, -1.0 + x * scale, 1.0 - y * scale, scale, -scale);
			}
			if (grid->hasData(x, y, DATA_VERTICAL_POLE)) {
				colourValidBlack(grid, DATA_VERTICAL_POLE, x, y, transparency);
				drawTextureQuad(8, -1.0 + x * scale, 1.0 - y * scale, scale, -scale);
			}
		}
	}

	glEnd();

	glBindTexture(GL_TEXTURE_2D, items);
	glBegin(GL_QUADS);

	for (int x = 0; x < grid->Width(); x++) {
		for (int y = 0; y < grid->Height(); y++) {
			if (grid->hasData(x, y, DATA_HIVE)) {
				colourValid(grid, DATA_HIVE, x, y, transparency);

				drawTextureQuad(0, -1.0 + x * scale, 1.0 - y * scale, scale, -scale);
			}

			if (grid->hasData(x, y, DATA_WORMGRASS)) {
				colourValid(grid, DATA_WORMGRASS, x, y, transparency);

				drawTextureQuad(1, -1.0 + x * scale, 1.0 - y * scale, scale, -scale);
			}

			if (grid->hasData(x, y, DATA_GARBAGE_WORM)) {
				colourValid(grid, DATA_GARBAGE_WORM, x, y, transparency);

				drawTextureQuad(2, -1.0 + x * scale, 1.0 - y * scale, scale, -scale);
			}

			if (grid->hasData(x, y, DATA_WATERFALL)) {
				colourValid(grid, DATA_WATERFALL, x, y, transparency);

				drawTextureQuad(3, -1.0 + x * scale, 1.0 - y * scale, scale, -scale);
			}

			if (grid->hasData(x, y, DATA_SPEAR)) {
				colourValid(grid, DATA_SPEAR, x, y, transparency);

				drawTextureQuad(4, -1.0 + x * scale, 1.0 - y * scale, scale, -scale);
			}

			if (grid->hasData(x, y, DATA_ROCK)) {
				colourValid(grid, DATA_ROCK, x, y, transparency);

				drawTextureQuad(5, -1.0 + x * scale, 1.0 - y * scale, scale, -scale);
			}
		}
	}

	glEnd();

	// glColor4f(1.0f, 1.0f, 1.0f, transparency);
	glBindTexture(GL_TEXTURE_2D, shortcuts);
	glBegin(GL_QUADS);

	for (float x = 0; x < grid->Width(); x++) {
		for (float y = 0; y < grid->Height(); y++) {
			if (grid->inBoundary(x, y)) {
				glColor4f(1.0f, 1.0f, 1.0f, transparency);
			} else {
				glColor4f(1.0f, 0.0f, 0.0f, transparency);
			}

			if (grid->hasData(x, y, DATA_ROOM_EXIT)) {
				drawTextureQuad(15, -1.0 + x * scale, 1.0 - y * scale, scale, -scale, TEXTURE_SHORTCUTS_WIDTH, TEXTURE_SHORTCUTS_HEIGHT);
			} else if (grid->hasData(x, y, DATA_LIZARD_DEN)) {
				drawTextureQuad(16, -1.0 + x * scale, 1.0 - y * scale, scale, -scale, TEXTURE_SHORTCUTS_WIDTH, TEXTURE_SHORTCUTS_HEIGHT);
			} else if (grid->hasData(x, y, DATA_WACKAMOLE)) {
				drawTextureQuad(18, -1.0 + x * scale, 1.0 - y * scale, scale, -scale, TEXTURE_SHORTCUTS_WIDTH, TEXTURE_SHORTCUTS_HEIGHT);
			} else if (grid->hasData(x, y, DATA_SCAVENGER_DEN)) {
				drawTextureQuad(17, -1.0 + x * scale, 1.0 - y * scale, scale, -scale, TEXTURE_SHORTCUTS_WIDTH, TEXTURE_SHORTCUTS_HEIGHT);
			} else if (grid->hasData(x, y, DATA_SHORTCUT)) {
				int tile = getShortcutTile(grid, x, y);

				if (tile == -1) continue;

				drawTextureQuad(tile, -1.0 + x * scale, 1.0 - y * scale, scale, -scale, TEXTURE_SHORTCUTS_WIDTH, TEXTURE_SHORTCUTS_HEIGHT);
			}
		}
	}

	glEnd();

	// Disable texture mapping after drawing
	glDisable(GL_TEXTURE_2D);
	glDisable(GL_BLEND);
}

// TODO: Update
void drawTexturedGrid(Grid *grid, double scaleX, double scaleY, GLuint solids, GLuint shortcuts, int startX, int startY, int endX, int endY) {
	glBindTexture(GL_TEXTURE_2D, solids);

	// Enable 2D texture
	glEnable(GL_TEXTURE_2D);
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	glBegin(GL_QUADS);

	for (float x = startX; x <= endX; x++) {
		for (float y = startY; y <= endY; y++) {
			unsigned int tile = grid->getTile(x, y);

			drawTextureQuad(tile, -1.0 + x * scaleX, 1.0 - y * scaleY, scaleX, -scaleY);

			if (grid->hasData(x, y, DATA_HORIZONTAL_POLE)) {
				drawTextureQuad(7, -1.0 + x * scaleX, 1.0 - y * scaleY, scaleX, -scaleY);
			}
			if (grid->hasData(x, y, DATA_VERTICAL_POLE)) {
				drawTextureQuad(8, -1.0 + x * scaleX, 1.0 - y * scaleY, scaleX, -scaleY);
			}
		}
	}

	glEnd();

	// Disable texture mapping after drawing
	glDisable(GL_TEXTURE_2D);
	glDisable(GL_BLEND);
}