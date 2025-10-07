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

	extern Texture *toolsTexture;
	extern Texture *shortcutsTexture;

	extern Room *room;
	extern bool showObjects;
}