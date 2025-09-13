#pragma once

#include "../../math/Vector.hpp"
#include "../../math/Rect.hpp"
#include "../../Texture.hpp"

namespace DropletWindow {
	enum class EditorTab {
		DETAILS,
		GEOMETRY,
		CAMERA,
		GENERATOR
	};

	enum class GeometryTool {
		WALL,
		SLOPE,
		PLATFORM,
		BACKGROUND_WALL,
		HORIZONTAL_POLE,
		VERTICAL_POLE,
		SPEAR,
		ROCK,
		SHORTCUT,
		ROOM_EXIT,
		CREATURE_DEN,
		WACK_A_MOLE,
		SCAVENGER_DEN,
		GARBAGE_WORM_DEN,
		WORMGRASS,
		BATFLY_HIVE
	};

	void init();
	void cleanup();

	void UpdateCamera();

	void UpdateGeometry();

	void Draw();

	void exportGeometry();
	void render();

	extern Texture *toolsTexture;
	extern Texture *shortcutsTexture;

	extern Vector2 cameraOffset;
	extern double cameraScale;
	extern double cameraScaleTo;
	extern bool cameraPanning;
	extern bool cameraPanningBlocked;
	extern Vector2 cameraPanStartMouse;
	extern Vector2 cameraPanStart;
	extern Vector2 cameraPanTo;
	extern double cameraScaleTo;

	extern Rect roomRect;
	extern Vector2i mouseTile;
	extern Vector2i lastMouseTile;
	extern bool lastMouseDrawing;
	extern bool blockMouse;

	extern EditorTab currentTab;

	extern std::string TAB_NAMES[4];
	extern std::string GEOMETRY_TOOL_NAMES[16];

	extern GeometryTool selectedTool;
}