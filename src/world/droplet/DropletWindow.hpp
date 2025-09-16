#pragma once

#include <vector>
#include "../../math/Vector.hpp"
#include "../../math/Rect.hpp"
#include "../../Texture.hpp"

#include "Node.hpp"

namespace DropletWindow {
	enum class EditorTab {
		DETAILS,
		GEOMETRY,
		CAMERA
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

	class Camera {
		public:
			Vector2 position;
			Vector2 angle0;
			Vector2 angle1;
			Vector2 angle2;
			Vector2 angle3;
	};

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

	extern Vector2 cameraOffset;
	extern double cameraScale;
	extern double cameraScaleTo;
	extern bool cameraPanning;
	extern bool cameraPanningBlocked;
	extern Vector2 cameraPanStartMouse;
	extern Vector2 cameraPanStart;
	extern Vector2 cameraPanTo;
	extern double cameraScaleTo;

	extern Vector2 transformedMouse;
	extern Rect roomRect;
	extern Vector2i mouseTile;
	extern Vector2i lastMouseTile;
	extern bool lastMouseDrawing;
	extern bool blockMouse;

	extern EditorTab currentTab;

	extern std::string TAB_NAMES[3];
	extern std::string GEOMETRY_TOOL_NAMES[16];

	extern GeometryTool selectedTool;

	extern std::vector<Camera> cameras;
	extern Camera *selectedCamera;

	extern int *backupGeometry;
	extern int backupWater;

	extern std::vector<Object *> objects;
}