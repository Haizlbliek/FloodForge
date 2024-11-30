#ifndef LEVEL_UTILS_HPP
#define LEVEL_UTILS_HPP

#include "gl.h"

#include "../Grid.hpp"

int getShortcutTile(Grid *grid, unsigned int x, unsigned int y);

void drawTexturedGrid(Grid *grid, float scale, GLuint solids, GLuint shortcuts, GLuint items, float transparency);

void drawTexturedGrid(Grid *grid, float scale, GLuint solids, GLuint shortcuts, GLuint items);

void drawTexturedGrid(Grid *grid, float scale, GLuint solids, GLuint shortcuts, unsigned int tilesWidth, unsigned int tilesHeight);

void drawTexturedGrid(Grid *grid, double scaleX, double scaleY, GLuint solids, GLuint shortcuts, int startX, int startY, int endX, int endY);

#endif