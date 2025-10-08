#pragma once

#include <vector>
#include "../../math/Vector.hpp"
#include "../../math/Rect.hpp"
#include "../../Texture.hpp"
#include "../Room.hpp"

#include "Node.hpp"

namespace DropletWindow {
	void init();
	void cleanup();

	void loadRoom();
	void resetChanges();

	void Draw();

	void exportGeometry();
	void exportProject(std::filesystem::path path);
	void render();
	void resizeRoom(int width, int height, bool stretch);

	extern Texture *toolsTexture;
	extern Texture *shortcutsTexture;

	extern Room *room;
	extern bool showObjects;

	extern bool showResize;
	extern Vector2i resizeSize;
	extern Vector2i resizeOffset;
}