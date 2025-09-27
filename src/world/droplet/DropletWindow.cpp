#include "DropletWindow.hpp"

#include "../../Utils.hpp"
#include "../../ui/UI.hpp"
#include "../Globals.hpp"
#include "../../popup/Popups.hpp"
#include "../../popup/InfoPopup.hpp"

#include "LevelUtils.hpp"
#include "../Backup.hpp"

Texture *DropletWindow::shortcutsTexture = nullptr;
Texture *DropletWindow::toolsTexture = nullptr;

Vector2 DropletWindow::cameraOffset;
double DropletWindow::cameraScale = 40.0;
double DropletWindow::cameraScaleTo = 40.0;
bool DropletWindow::cameraPanning = false;
bool DropletWindow::cameraPanningBlocked = false;
Vector2 DropletWindow::cameraPanStartMouse = Vector2(0.0f, 0.0f);
Vector2 DropletWindow::cameraPanStart = Vector2(0.0f, 0.0f);
Vector2 DropletWindow::cameraPanTo = Vector2(0.0f, 0.0f);

Vector2 DropletWindow::transformedMouse;
Rect DropletWindow::roomRect;
Vector2i DropletWindow::mouseTile;
Vector2i DropletWindow::lastMouseTile;
bool DropletWindow::lastMouseDrawing;
bool DropletWindow::blockMouse = false;

DropletWindow::EditorTab DropletWindow::currentTab;

std::string DropletWindow::TAB_NAMES[3] = { "Environment", "Geometry", "Cameras" };
std::string DropletWindow::GEOMETRY_TOOL_NAMES[16] = { "Wall", "Slope", "Platform", "Background Wall", "Horizontal Pole", "Vertical Pole", "Spear", "Rock", "Shortcut", "Room Exit", "Creature Den", "Wack a Mole Hole", "Scavenger Den", "Garbage Worm Den", "Wormgrass", "Batfly Hive" };

DropletWindow::GeometryTool DropletWindow::selectedTool = DropletWindow::GeometryTool::WALL;

std::vector<DropletWindow::Camera> DropletWindow::cameras;
DropletWindow::Camera *DropletWindow::selectedCamera = nullptr;

bool enclosedRoom = false;
bool waterInFront = false;

int *DropletWindow::backupGeometry = nullptr;
int DropletWindow::backupWater;

std::vector<Object *> DropletWindow::objects;
std::vector<TerrainHandleObject *> terrainHandleObjects;
std::vector<MudPitObject *> mudPitObjects;
std::vector<AirPocketObject *> airPocketObjects;

bool terrainNeedsRefresh = false;
bool hasTerrain = false;
std::vector<Vector2> terrain;
int trashCanState = 0;

struct WaterSpot {
	Vector2 pos;
	Vector2 size;

	bool intersects(WaterSpot &other) {
		return pos.x <= other.pos.x + other.size.x && pos.y <= other.pos.y + other.size.y && pos.x + size.x >= other.pos.x && pos.y + size.y >= other.pos.y;
	}
};
bool waterNeedsRefresh = false;
std::vector<WaterSpot> water;

void DropletWindow::init() {
	toolsTexture = new Texture(BASE_PATH / "assets" / "tools.png");
	shortcutsTexture = new Texture(BASE_PATH / "assets" / "shortcuts.png");
}

void DropletWindow::cleanup() {
	delete toolsTexture;
	delete shortcutsTexture;
}

void UpdateCamera() {
	bool isHoveringPopup = false;
	for (Popup *popup : Popups::popups) {
		Rect bounds = popup->Bounds();

		if (bounds.inside(UI::mouse)) {
			isHoveringPopup = true;
			break;
		}
	}

	double scrollY = -UI::window->getMouseScrollY();
	double zoom = std::pow(1.25, scrollY);

	Vector2 previousWorldMouse = Vector2(
		UI::mouse.x * DropletWindow::cameraScale + DropletWindow::cameraOffset.x,
		UI::mouse.y * DropletWindow::cameraScale + DropletWindow::cameraOffset.y
	);

	DropletWindow::cameraScaleTo *= zoom;
	DropletWindow::cameraScaleTo = std::clamp(DropletWindow::cameraScaleTo, 2.5, 1.0 * std::max(EditorState::dropletRoom->width, EditorState::dropletRoom->height));
	DropletWindow::cameraScale += (DropletWindow::cameraScaleTo - DropletWindow::cameraScale) * Settings::getSetting<double>(Settings::Setting::CameraZoomSpeed);

	Vector2 worldMouse = Vector2(
		UI::mouse.x * DropletWindow::cameraScale + DropletWindow::cameraOffset.x,
		UI::mouse.y * DropletWindow::cameraScale + DropletWindow::cameraOffset.y
	);

	DropletWindow::cameraOffset.x += previousWorldMouse.x - worldMouse.x;
	DropletWindow::cameraOffset.y += previousWorldMouse.y - worldMouse.y;
	DropletWindow::cameraPanTo.x += previousWorldMouse.x - worldMouse.x;
	DropletWindow::cameraPanTo.y += previousWorldMouse.y - worldMouse.y;

	// Panning
	if (UI::mouse.middleMouse) {
		if (!DropletWindow::cameraPanningBlocked && !DropletWindow::cameraPanning) {
			if (isHoveringPopup) DropletWindow::cameraPanningBlocked = true;

			if (!DropletWindow::cameraPanningBlocked) {
				DropletWindow::cameraPanStart.x = DropletWindow::cameraOffset.x;
				DropletWindow::cameraPanStart.y = DropletWindow::cameraOffset.y;
				DropletWindow::cameraPanStartMouse.x = EditorState::globalMouse.x;
				DropletWindow::cameraPanStartMouse.y = EditorState::globalMouse.y;
				DropletWindow::cameraPanning = true;
			}
		}

		if (DropletWindow::cameraPanning && !DropletWindow::cameraPanningBlocked) {
			DropletWindow::cameraPanTo.x = DropletWindow::cameraPanStart.x + DropletWindow::cameraScale * (DropletWindow::cameraPanStartMouse.x - EditorState::globalMouse.x) / 512.0;
			DropletWindow::cameraPanTo.y = DropletWindow::cameraPanStart.y + DropletWindow::cameraScale * (DropletWindow::cameraPanStartMouse.y - EditorState::globalMouse.y) / -512.0;
		}
	} else {
		DropletWindow::cameraPanning = false;
		DropletWindow::cameraPanningBlocked = false;
	}

	DropletWindow::cameraPanTo.x = std::clamp(DropletWindow::cameraPanTo.x, -(UI::screenBounds.x - 0.41) * DropletWindow::cameraScale, UI::screenBounds.x * DropletWindow::cameraScale + EditorState::dropletRoom->width);
	DropletWindow::cameraPanTo.y = std::clamp(DropletWindow::cameraPanTo.y, -(UI::screenBounds.y - 0.12) * DropletWindow::cameraScale - EditorState::dropletRoom->height, UI::screenBounds.y * DropletWindow::cameraScale);

	DropletWindow::cameraOffset.x += (DropletWindow::cameraPanTo.x - DropletWindow::cameraOffset.x) * Settings::getSetting<double>(Settings::Setting::CameraPanSpeed);
	DropletWindow::cameraOffset.y += (DropletWindow::cameraPanTo.y - DropletWindow::cameraOffset.y) * Settings::getSetting<double>(Settings::Setting::CameraPanSpeed);
}

double sampleHandle(double a, double b, double c, double d, double t) {
	double it = 1.0 - t;
	return it * it * it * a + 3.0 * it * it * t * b + 3.0 * it * t * t * c + t * t * t *d;
}

double sampleTerrain(TerrainHandleObject *left, TerrainHandleObject *right, double x) {
	if (x < left->Middle().x) {
		return MathUtils::lerp(left->Middle().y, left->Left().y, MathUtils::inverseLerp(x, left->Middle().x, left->Left().x));
	}
	if (x > right->Middle().x) {
		return MathUtils::lerp(right->Middle().y, right->Right().y, MathUtils::inverseLerp(x, right->Middle().x, right->Right().x));
	}
	double leftPos = 0.0;
	double rightPos = 1.0;
	for (int i = 0; i < 16; i++) {
		double middlePos = (leftPos + rightPos) * 0.5;
		if (sampleHandle(left->Middle().x, left->Right().x, right->Left().x, right->Middle().x, middlePos) < x) {
			leftPos = middlePos;
		} else {
			rightPos = middlePos;
		}
	}
	return sampleHandle(left->Middle().y, left->Right().y, right->Left().y, right->Middle().y, (leftPos + rightPos) * 0.5);
}

std::vector<WaterSpot> splitWater(WaterSpot water, WaterSpot by) {
	WaterSpot left = { water.pos, Vector2(by.pos.x - water.pos.x, water.size.y) };
	WaterSpot right = { Vector2(by.pos.x + by.size.x, water.pos.y), water.size - Vector2((by.pos.x + by.size.x) - water.pos.x, 0.0) };
	double middlePosX = std::max(by.pos.x, water.pos.x);
	double middleSizeX = std::min(by.pos.x + by.size.x, water.pos.x + water.size.x) - middlePosX;
	WaterSpot bottom = { Vector2(middlePosX, water.pos.y), Vector2(middleSizeX, by.pos.y - water.pos.y) };
	WaterSpot top = { Vector2(middlePosX, by.pos.y + by.size.y), Vector2(middleSizeX, water.size.y - ((by.pos.y + by.size.y) - water.pos.y)) };
	std::vector<WaterSpot> values;
	if (left.size.x > 0.0 && left.size.y > 0.0) values.push_back(left);
	if (right.size.x > 0.0 && right.size.y > 0.0) values.push_back(right);
	if (bottom.size.x > 0.0 && bottom.size.y > 0.0) values.push_back(bottom);
	if (top.size.x > 0.0 && top.size.y > 0.0) values.push_back(top);

	return values;
}

void UpdateDetailsTab() {
	if (hasTerrain && terrain.size() >= 2) {
		Draw::color(0.0, 1.0, 0.0);
		Draw::begin(Draw::LINE_STRIP);
		for (Vector2 &point : terrain) {
			Draw::vertex(DropletWindow::roomRect.x0 + point.x / 20.0, DropletWindow::roomRect.y0 + point.y / 20.0);
		}
		Draw::end();
	}

	static Node *movingNode = nullptr;
	Node *hoveredNode = nullptr;

	Vector2 nodeMouse = Vector2(DropletWindow::transformedMouse.x, DropletWindow::transformedMouse.y + (DropletWindow::roomRect.y1 - DropletWindow::roomRect.y0)) * 20.0;
	double mouseDistance = 0.3 * DropletWindow::cameraScale;

	for (Object *object : DropletWindow::objects) {
		object->draw(Vector2(DropletWindow::roomRect.x0, DropletWindow::roomRect.y0));

		for (Node *node : object->nodes) {
			Vector2 nodePos = node->position();

			if (nodePos.distanceTo(nodeMouse) < mouseDistance) {
				Draw::color(1.0);
				hoveredNode = node;
			} else {
				Draw::color(0.6);
			}
			Vector2 nodeRenderPos = { DropletWindow::roomRect.x0 + nodePos.x / 20.0, DropletWindow::roomRect.y0 + nodePos.y / 20.0 };
			fillCircle(nodeRenderPos.x, nodeRenderPos.y, 0.01 * DropletWindow::cameraScale, 8);
			Draw::color(0.0);
			strokeCircle(nodeRenderPos.x, nodeRenderPos.y, 0.01 * DropletWindow::cameraScale, 8);
		}
	}

	if (UI::mouse.justClicked() && hoveredNode != nullptr) {
		movingNode = hoveredNode;
	}

	if (movingNode != nullptr) {
		bool needsDeleted = false;

		if (movingNode->parent == nullptr) {
			trashCanState = 1;
			movingNode->pos.x = nodeMouse.x;
			movingNode->pos.y = nodeMouse.y;

			if (Rect::fromSize(-UI::screenBounds.x + 0.01, -UI::screenBounds.y + 0.01, 0.1, 0.1).inside(UI::mouse)) {
				trashCanState = 2;

				if (!UI::mouse.leftMouse) {
					needsDeleted = true;
				}
			}
		} else {
			trashCanState = 0;
			Vector2 newPos = nodeMouse - movingNode->parent->position();
			movingNode->pos.x = newPos.x;
			movingNode->pos.y = newPos.y;
		}

		if (static_cast<TerrainHandleObject *>(movingNode->object) != nullptr) {
			terrainNeedsRefresh = true;
		}
		else if (static_cast<AirPocketObject *>(movingNode->object) != nullptr) {
			waterNeedsRefresh = true;
		}

		if (!UI::mouse.leftMouse) {
			if (needsDeleted) {
				DropletWindow::objects.erase(std::remove(DropletWindow::objects.begin(), DropletWindow::objects.end(), movingNode->object), DropletWindow::objects.end());
				terrainHandleObjects.erase(std::remove(terrainHandleObjects.begin(), terrainHandleObjects.end(), movingNode->object), terrainHandleObjects.end());
				mudPitObjects.erase(std::remove(mudPitObjects.begin(), mudPitObjects.end(), movingNode->object), mudPitObjects.end());
				airPocketObjects.erase(std::remove(airPocketObjects.begin(), airPocketObjects.end(), movingNode->object), airPocketObjects.end());
				delete movingNode->object;
			}

			hoveredNode = nullptr;
			movingNode = nullptr;
		}
	}
	else {
		trashCanState = 0;

		if (!DropletWindow::blockMouse && UI::window->keyPressed(GLFW_KEY_W) && EditorState::dropletRoom->water != -1) {
			EditorState::dropletRoom->water = EditorState::dropletRoom->height - DropletWindow::mouseTile.y - 1;
			if (EditorState::dropletRoom->water < 0) EditorState::dropletRoom->water = 0;
			waterNeedsRefresh = true;
		}
	}

	if (terrainNeedsRefresh) {
		terrain.clear();
		std::vector<TerrainHandleObject *> handles;

		for (TerrainHandleObject *handle : terrainHandleObjects) {
			handles.push_back(handle);
		}

		if (handles.size() >= 2) {
			std::sort(handles.begin(), handles.end(), [](TerrainHandleObject *a, TerrainHandleObject *b) {
				return b->Middle().x > a->Middle().x;
			});
			int handleIndex = 0;
			int segments = EditorState::dropletRoom->width + 1;
			for (float x = 0; x < segments * 20.0; x += 20.0) {
				while (handleIndex < handles.size() - 2 && handles[handleIndex + 1]->Middle().x < x) {
					handleIndex++;
				}
				terrain.push_back(Vector2(x, sampleTerrain(handles[handleIndex], handles[handleIndex + 1], x)));
			}
			hasTerrain = true;
		} else {
			hasTerrain = false;
		}
	}

	if (waterNeedsRefresh) {
		water.clear();
		water.push_back({ Vector2(0, 0), Vector2(EditorState::dropletRoom->width * 20.0, EditorState::dropletRoom->water * 20.0 + 10.0) });
		for (AirPocketObject *pocket : airPocketObjects) {
			WaterSpot spot = { pocket->nodes[0]->pos, pocket->nodes[1]->pos };
			if (spot.size.x <= 0.0) continue;
			if (spot.size.y <= 0.0) continue;
			for (int i = water.size() - 1; i >= 0; i--) {
				if (water[i].intersects(spot)) {
					WaterSpot before = water[i];
					std::vector<WaterSpot> splits = splitWater(before, spot);
					if (splits.size() == 0) {
						if (i != water.size() - 1) water[i] = water.back();
						water.pop_back();
					} else {
						water[i] = splits[0];
						for (int j = 1; j < splits.size(); j++) {
							water.push_back(splits[j]);
						}
					}
				}
			}
			spot.size.y = std::min(pocket->nodes[2]->pos.y, spot.size.y);
			if (spot.size.y <= 0.0) continue;
			water.push_back(spot);
		}
	}
}

void verifyShortcut(int x, int y) {
	if ((EditorState::dropletRoom->getTile(x, y) & 128) == 0) {
		return;
	}

	bool shorcutEntrance = false;

	if (
		(EditorState::dropletRoom->getTile(x - 1, y - 1) % 16 == 1) && (EditorState::dropletRoom->getTile(x + 1, y - 1) % 16 == 1) &&
		(EditorState::dropletRoom->getTile(x - 1, y + 1) % 16 == 1) && (EditorState::dropletRoom->getTile(x + 1, y + 1) % 16 == 1)
	) {
		int dir = 0;
		dir += (EditorState::dropletRoom->getTile(x - 1, y) % 16 == 0 || EditorState::dropletRoom->getTile(x - 1, y) % 16 == 2) ? 1 : 0;
		dir += (EditorState::dropletRoom->getTile(x + 1, y) % 16 == 0 || EditorState::dropletRoom->getTile(x + 1, y) % 16 == 2) ? 2 : 0;
		dir += (EditorState::dropletRoom->getTile(x, y - 1) % 16 == 0 || EditorState::dropletRoom->getTile(x, y - 1) % 16 == 2) ? 4 : 0;
		dir += (EditorState::dropletRoom->getTile(x, y + 1) % 16 == 0 || EditorState::dropletRoom->getTile(x, y + 1) % 16 == 2) ? 8 : 0;
		dir += (EditorState::dropletRoom->getTile(x - 1, y) & 128) > 0 ? 16 : 0;
		dir += (EditorState::dropletRoom->getTile(x + 1, y) & 128) > 0 ? 32 : 0;
		dir += (EditorState::dropletRoom->getTile(x, y - 1) & 128) > 0 ? 64 : 0;
		dir += (EditorState::dropletRoom->getTile(x, y + 1) & 128) > 0 ? 128 : 0;

		if (dir == 1 + 32 || dir == 2 + 16 || dir == 4 + 128 || dir == 8 + 64) {
			shorcutEntrance = true;
		}
	}

	if (shorcutEntrance) {
		EditorState::dropletRoom->geometry[x * EditorState::dropletRoom->height + y] = (EditorState::dropletRoom->geometry[x * EditorState::dropletRoom->height + y] & ~15) | 4;
	} else {
		if (EditorState::dropletRoom->geometry[x * EditorState::dropletRoom->height + y] % 16 == 4) {
			EditorState::dropletRoom->geometry[x * EditorState::dropletRoom->height + y] = (EditorState::dropletRoom->geometry[x * EditorState::dropletRoom->height + y] & ~15) | 1;
		}
	}
}

void applyTool(int x, int y, bool right) {
	if (DropletWindow::selectedTool == DropletWindow::GeometryTool::WALL) {
		EditorState::dropletRoom->geometry[x * EditorState::dropletRoom->height + y] = right ? 0 : 1;
	}
	else if (DropletWindow::selectedTool == DropletWindow::GeometryTool::SLOPE) {
		if (right) {
			EditorState::dropletRoom->geometry[x * EditorState::dropletRoom->height + y] = 0;
		} else {
			int bits = 0;
			bits += ((EditorState::dropletRoom->getTile(x - 1, y) & 15) == 1) ? 1 : 0;
			bits += ((EditorState::dropletRoom->getTile(x + 1, y) & 15) == 1) ? 2 : 0;
			bits += ((EditorState::dropletRoom->getTile(x, y - 1) & 15) == 1) ? 4 : 0;
			bits += ((EditorState::dropletRoom->getTile(x, y + 1) & 15) == 1) ? 8 : 0;
			int type = -1;
			if ((EditorState::dropletRoom->getTile(x - 1, y) & 15) == 2 || (EditorState::dropletRoom->getTile(x + 1, y) & 15) == 2 || (EditorState::dropletRoom->getTile(x, y - 1) & 15) == 2 || (EditorState::dropletRoom->getTile(x, y + 1) & 15) == 2) {
				bits = -1;
			}

			if (bits == 1 + 4) {
				type = 0;
			} else if (bits == 1 + 8) {
				type = 1;
			} else if (bits == 2 + 4) {
				type = 2;
			} else if (bits == 2 + 8) {
				type = 3;
			}

			if (type != -1) {
				EditorState::dropletRoom->geometry[x * EditorState::dropletRoom->height + y] = 2 + 1024 * type;
			}
		}
	}
	else if (DropletWindow::selectedTool == DropletWindow::GeometryTool::PLATFORM) {
		EditorState::dropletRoom->geometry[x * EditorState::dropletRoom->height + y] = right ? 0 : 3;
	}
	else if (DropletWindow::selectedTool == DropletWindow::GeometryTool::VERTICAL_POLE) {
		if (right) {
			EditorState::dropletRoom->geometry[x * EditorState::dropletRoom->height + y] &= ~16;
		} else {
			EditorState::dropletRoom->geometry[x * EditorState::dropletRoom->height + y] |= 16;
		}
	}
	else if (DropletWindow::selectedTool == DropletWindow::GeometryTool::HORIZONTAL_POLE) {
		if (right) {
			EditorState::dropletRoom->geometry[x * EditorState::dropletRoom->height + y] &= ~32;
		} else {
			EditorState::dropletRoom->geometry[x * EditorState::dropletRoom->height + y] |= 32;
		}
	}
	else if (DropletWindow::selectedTool == DropletWindow::GeometryTool::BACKGROUND_WALL) {
		if (right) {
			EditorState::dropletRoom->geometry[x * EditorState::dropletRoom->height + y] &= ~512;
		} else {
			EditorState::dropletRoom->geometry[x * EditorState::dropletRoom->height + y] |= 512;
		}
	}
	else if (DropletWindow::selectedTool == DropletWindow::GeometryTool::ROOM_EXIT) {
		if (right) {
			if (EditorState::dropletRoom->geometry[x * EditorState::dropletRoom->height + y] & 64) {
				EditorState::dropletRoom->geometry[x * EditorState::dropletRoom->height + y] &= ~64;
				EditorState::dropletRoom->geometry[x * EditorState::dropletRoom->height + y] &= ~128;

				verifyShortcut(x - 1, y);
				verifyShortcut(x + 1, y);
				verifyShortcut(x, y - 1);
				verifyShortcut(x, y + 1);
			}
		} else {
			EditorState::dropletRoom->geometry[x * EditorState::dropletRoom->height + y] |= 64 | 128;

			verifyShortcut(x, y);
			verifyShortcut(x - 1, y);
			verifyShortcut(x + 1, y);
			verifyShortcut(x, y - 1);
			verifyShortcut(x, y + 1);
		}
	}
	else if (DropletWindow::selectedTool == DropletWindow::GeometryTool::CREATURE_DEN) {
		if (right) {
			if (EditorState::dropletRoom->geometry[x * EditorState::dropletRoom->height + y] & 256) {
				EditorState::dropletRoom->geometry[x * EditorState::dropletRoom->height + y] &= ~256;
				EditorState::dropletRoom->geometry[x * EditorState::dropletRoom->height + y] &= ~128;

				verifyShortcut(x - 1, y);
				verifyShortcut(x + 1, y);
				verifyShortcut(x, y - 1);
				verifyShortcut(x, y + 1);
			}
		} else {
			EditorState::dropletRoom->geometry[x * EditorState::dropletRoom->height + y] |= 256 | 128;

			verifyShortcut(x, y);
			verifyShortcut(x - 1, y);
			verifyShortcut(x + 1, y);
			verifyShortcut(x, y - 1);
			verifyShortcut(x, y + 1);
		}
	}
	else if (DropletWindow::selectedTool == DropletWindow::GeometryTool::SCAVENGER_DEN) {
		if (right) {
			if (EditorState::dropletRoom->geometry[x * EditorState::dropletRoom->height + y] & 4096) {
				EditorState::dropletRoom->geometry[x * EditorState::dropletRoom->height + y] &= ~4096;
				EditorState::dropletRoom->geometry[x * EditorState::dropletRoom->height + y] &= ~128;

				verifyShortcut(x - 1, y);
				verifyShortcut(x + 1, y);
				verifyShortcut(x, y - 1);
				verifyShortcut(x, y + 1);
			}
		} else {
			EditorState::dropletRoom->geometry[x * EditorState::dropletRoom->height + y] |= 4096 | 128;

			verifyShortcut(x, y);
			verifyShortcut(x - 1, y);
			verifyShortcut(x + 1, y);
			verifyShortcut(x, y - 1);
			verifyShortcut(x, y + 1);
		}
	}
	else if (DropletWindow::selectedTool == DropletWindow::GeometryTool::WACK_A_MOLE) {
		if (right) {
			if (EditorState::dropletRoom->geometry[x * EditorState::dropletRoom->height + y] & 8192) {
				EditorState::dropletRoom->geometry[x * EditorState::dropletRoom->height + y] &= ~8192;
				EditorState::dropletRoom->geometry[x * EditorState::dropletRoom->height + y] &= ~128;

				verifyShortcut(x - 1, y);
				verifyShortcut(x + 1, y);
				verifyShortcut(x, y - 1);
				verifyShortcut(x, y + 1);
			}
		} else {
			EditorState::dropletRoom->geometry[x * EditorState::dropletRoom->height + y] |= 8192 | 128;

			verifyShortcut(x, y);
			verifyShortcut(x - 1, y);
			verifyShortcut(x + 1, y);
			verifyShortcut(x, y - 1);
			verifyShortcut(x, y + 1);
		}
	}
	else if (DropletWindow::selectedTool == DropletWindow::GeometryTool::GARBAGE_WORM_DEN) {
		if (right) {
			EditorState::dropletRoom->geometry[x * EditorState::dropletRoom->height + y] &= ~16384;
		} else {
			EditorState::dropletRoom->geometry[x * EditorState::dropletRoom->height + y] |= 16384;
		}
	}
	else if (DropletWindow::selectedTool == DropletWindow::GeometryTool::WORMGRASS) {
		if (right) {
			EditorState::dropletRoom->geometry[x * EditorState::dropletRoom->height + y] &= ~32768;
		} else {
			EditorState::dropletRoom->geometry[x * EditorState::dropletRoom->height + y] |= 32768;
		}
	}
	else if (DropletWindow::selectedTool == DropletWindow::GeometryTool::BATFLY_HIVE) {
		if (right) {
			EditorState::dropletRoom->geometry[x * EditorState::dropletRoom->height + y] &= ~65536;
		} else {
			EditorState::dropletRoom->geometry[x * EditorState::dropletRoom->height + y] |= 65536;
		}
	}
	else if (DropletWindow::selectedTool == DropletWindow::GeometryTool::SHORTCUT) {
		if (right) {
			if (EditorState::dropletRoom->geometry[x * EditorState::dropletRoom->height + y] & 128) {
				EditorState::dropletRoom->geometry[x * EditorState::dropletRoom->height + y] &= ~128;
				EditorState::dropletRoom->geometry[x * EditorState::dropletRoom->height + y] &= ~64;
				EditorState::dropletRoom->geometry[x * EditorState::dropletRoom->height + y] &= ~256;
				EditorState::dropletRoom->geometry[x * EditorState::dropletRoom->height + y] &= ~4096;
				EditorState::dropletRoom->geometry[x * EditorState::dropletRoom->height + y] &= ~8192;
				if (EditorState::dropletRoom->geometry[x * EditorState::dropletRoom->height + y] % 16 == 4) {
					EditorState::dropletRoom->geometry[x * EditorState::dropletRoom->height + y] = (EditorState::dropletRoom->geometry[x * EditorState::dropletRoom->height + y] & ~15) | 1;
				}

				verifyShortcut(x - 1, y);
				verifyShortcut(x + 1, y);
				verifyShortcut(x, y - 1);
				verifyShortcut(x, y + 1);
			}
		} else {
			EditorState::dropletRoom->geometry[x * EditorState::dropletRoom->height + y] |= 128;

			verifyShortcut(x, y);
			verifyShortcut(x - 1, y);
			verifyShortcut(x + 1, y);
			verifyShortcut(x, y - 1);
			verifyShortcut(x, y + 1);
		}
	}
	else if (DropletWindow::selectedTool == DropletWindow::GeometryTool::ROCK) {
		if (right) {
			EditorState::dropletRoom->geometry[x * EditorState::dropletRoom->height + y] &= ~262144;
		} else {
			EditorState::dropletRoom->geometry[x * EditorState::dropletRoom->height + y] |= 262144;
		}
	}
	else if (DropletWindow::selectedTool == DropletWindow::GeometryTool::SPEAR) {
		if (right) {
			EditorState::dropletRoom->geometry[x * EditorState::dropletRoom->height + y] &= ~524288;
		} else {
			EditorState::dropletRoom->geometry[x * EditorState::dropletRoom->height + y] |= 524288;
		}
	}
}

void UpdateGeometryTab() {
	if (!(UI::mouse.leftMouse || UI::mouse.rightMouse)) {
		int tool = (int) DropletWindow::selectedTool;

		if (UI::window->justPressed(GLFW_KEY_A)) {
			tool = ((tool + 3) % 4) + (tool & 0b1100);
		}
	
		if (UI::window->justPressed(GLFW_KEY_D)) {
			tool = ((tool + 1) % 4) + (tool & 0b1100);
		}
	
		if (UI::window->justPressed(GLFW_KEY_W)) {
			tool = ((tool + 12) % 16);
		}
	
		if (UI::window->justPressed(GLFW_KEY_S)) {
			tool = ((tool + 4) % 16);
		}

		DropletWindow::selectedTool = (DropletWindow::GeometryTool) tool;
	}

	static int rectDrawing = -1;
	static Vector2i rectStart;

	if (!DropletWindow::blockMouse) {
		if (rectDrawing != -1) {
			if ((rectDrawing == 0 && !UI::mouse.leftMouse) || (rectDrawing == 1 && !UI::mouse.rightMouse)) {
				for (int x = std::min(rectStart.x, DropletWindow::mouseTile.x); x <= std::max(rectStart.x, DropletWindow::mouseTile.x); x++) {
					for (int y = std::min(rectStart.y, DropletWindow::mouseTile.y); y <= std::max(rectStart.y, DropletWindow::mouseTile.y); y++) {
						applyTool(x, y, rectDrawing == 1);
					}
				}
				rectDrawing = -1;
			}
		}
		else if (UI::window->modifierPressed(GLFW_MOD_SHIFT) && (UI::mouse.leftMouse || UI::mouse.rightMouse)) {
			rectDrawing = UI::mouse.leftMouse ? 0 : 1;
			rectStart = DropletWindow::mouseTile;
		}
		else if (DropletWindow::selectedTool == DropletWindow::GeometryTool::WALL && UI::window->keyPressed(GLFW_KEY_Q) && (UI::mouse.leftMouse || UI::mouse.rightMouse) && EditorState::dropletRoom->InBounds(DropletWindow::mouseTile.x, DropletWindow::mouseTile.y)) {
			std::vector<Vector2i> items;
			std::vector<Vector2i> visited;
			int setTo = UI::mouse.leftMouse ? 1 : 0;
			int geoType = EditorState::dropletRoom->getTile(DropletWindow::mouseTile.x, DropletWindow::mouseTile.y) % 16;
			if (geoType == 1 || geoType == 0) {
				bool solid = geoType == 1;
				items.push_back({ DropletWindow::mouseTile.x, DropletWindow::mouseTile.y });

				while (items.size() > 0) {
					const Vector2i tile = items.back();
					items.pop_back();
					visited.push_back(tile);

					if (!EditorState::dropletRoom->InBounds(tile.x, tile.y)) continue;
					int geo = EditorState::dropletRoom->getTile(tile.x, tile.y);
					if ((geo % 16) != 1 && (geo % 16) != 0) continue;
					if ((geo % 16 == 1) != solid) continue;

					EditorState::dropletRoom->geometry[tile.x * EditorState::dropletRoom->height + tile.y] = (geo & ~15) | setTo;
					Vector2i v;
					v = { tile.x - 1, tile.y };
					if (std::find(visited.begin(), visited.end(), v) == visited.end()) items.push_back(v);
					v = { tile.x + 1, tile.y };
					if (std::find(visited.begin(), visited.end(), v) == visited.end()) items.push_back(v);
					v = { tile.x, tile.y - 1 };
					if (std::find(visited.begin(), visited.end(), v) == visited.end()) items.push_back(v);
					v = { tile.x, tile.y + 1 };
					if (std::find(visited.begin(), visited.end(), v) == visited.end()) items.push_back(v);
				}
			}
		}
		else {
			if ((UI::mouse.leftMouse || UI::mouse.rightMouse) && (DropletWindow::lastMouseTile.x != DropletWindow::mouseTile.x || DropletWindow::lastMouseTile.y != DropletWindow::mouseTile.y)) {
				std::vector<Vector2i> drawLine;
				drawLine = LevelUtils::line(DropletWindow::lastMouseTile.x, DropletWindow::lastMouseTile.y, DropletWindow::mouseTile.x, DropletWindow::mouseTile.y);

				for (Vector2i point : drawLine) {
					if (EditorState::dropletRoom->InBounds(point.x, point.y)) {
						applyTool(point.x, point.y, UI::mouse.rightMouse);
					}
				}
			} else if ((UI::mouse.leftMouse && !UI::mouse.lastLeftMouse) || (UI::mouse.rightMouse && !UI::mouse.lastRightMouse)) {
				if (EditorState::dropletRoom->InBounds(DropletWindow::mouseTile.x, DropletWindow::mouseTile.y)) {
					applyTool(DropletWindow::mouseTile.x, DropletWindow::mouseTile.y, UI::mouse.rightMouse);
				}
			}
		}

		setThemeColor(ThemeColour::RoomBorderHighlight);
		if (rectDrawing != -1) {
			int sx = std::min(rectStart.x, DropletWindow::mouseTile.x);
			int sy = std::min(rectStart.y, DropletWindow::mouseTile.y);
			int ex = std::max(rectStart.x, DropletWindow::mouseTile.x);
			int ey = std::max(rectStart.y, DropletWindow::mouseTile.y);

			strokeRect(Rect(DropletWindow::roomRect.x0 + sx, DropletWindow::roomRect.y1 - sy, DropletWindow::roomRect.x0 + ex + 1, DropletWindow::roomRect.y1 - ey - 1));
		}
		else {
			strokeRect(Rect::fromSize(DropletWindow::roomRect.x0 + DropletWindow::mouseTile.x, DropletWindow::roomRect.y1 - DropletWindow::mouseTile.y - 1, 1.0, 1.0));
		}
	}

	DropletWindow::lastMouseDrawing = UI::mouse.leftMouse || UI::mouse.rightMouse;
}

bool drawCameraAngle(double x, double y, Vector2 &angle, bool dragging) {
	bool hovered = !DropletWindow::blockMouse && (DropletWindow::transformedMouse.distanceTo(Vector2(x, y)) < 4.0 || DropletWindow::transformedMouse.distanceTo(Vector2(x + 4.0 * angle.x, y + 4.0 * angle.y)) < 0.05 * DropletWindow::cameraScale);
	Draw::color(Color(0.0, 1.0, 0.0, hovered ? 1.0 : 0.5));
	strokeCircle(x, y, 4.0, 20);
	drawLine(x, y, x + angle.x * 4.0, y + angle.y * 4.0);
	strokeCircle(x, y, angle.length() * 4.0, 20);
	fillCircle(x + angle.x * 4.0, y + angle.y * 4.0, 0.01 * DropletWindow::cameraScale, 8);

	if (dragging) {
		angle.x = (DropletWindow::transformedMouse.x - x) / 4.0;
		angle.y = (DropletWindow::transformedMouse.y - y) / 4.0;
		if (!UI::window->modifierPressed(GLFW_MOD_SHIFT)) {
			double len = angle.length();
			if (len > 1.0) {
				angle = angle / len;
			}
		}
		return true;
	}
	if (hovered && UI::mouse.justClicked()) {
		angle.x = (DropletWindow::transformedMouse.x - x) / 4.0;
		angle.y = (DropletWindow::transformedMouse.y - y) / 4.0;
		return true;
	}
	if (hovered && UI::mouse.rightMouse && !UI::mouse.lastRightMouse) {
		angle.x = 0.0;
		angle.y = 0.0;
	}

	return false;
}

void UpdateCameraTab() {
	static DropletWindow::Camera *draggingCamera = nullptr;
	static int draggingCameraAngle = -1;
	static Vector2 dragStart;

	if (!UI::mouse.leftMouse) {
		draggingCamera = nullptr;
		draggingCameraAngle = -1;
	}

	Vector2 cameraSizeTiles(70, 40);
	Vector2 cameraSizeLarge(68.3, 38.4);
	Vector2 cameraSizeSmall(51.2, 38.4);
	glEnable(GL_BLEND);
	int i = 1;
	bool newSelectedCamera = false;
	for (DropletWindow::Camera &camera : DropletWindow::cameras) {
		bool selected = DropletWindow::selectedCamera == &camera;
		Vector2 center(camera.position.x + cameraSizeTiles.x * 0.5, camera.position.y + cameraSizeTiles.y * 0.5);
		Draw::color(Color(0.0, 1.0, 0.0, selected ? 0.25 : 0.15));
		Draw::begin(Draw::QUADS);
		Draw::vertex(camera.position.x + camera.angle0.x * 4.0, -camera.position.y + camera.angle0.y * 4.0);
		Draw::vertex(camera.position.x + camera.angle1.x * 4.0 + cameraSizeTiles.x, -camera.position.y + camera.angle1.y * 4.0);
		Draw::vertex(camera.position.x + camera.angle2.x * 4.0 + cameraSizeTiles.x, -camera.position.y + camera.angle2.y * 4.0 - cameraSizeTiles.y);
		Draw::vertex(camera.position.x + camera.angle3.x * 4.0, -camera.position.y + camera.angle3.y * 4.0 - cameraSizeTiles.y);
		Draw::end();
		Draw::color(Color(0.0, 0.0, 0.0));
		strokeRect(Rect::fromSize(center.x - cameraSizeLarge.x * 0.5, -center.y - cameraSizeLarge.y * 0.5, cameraSizeLarge.x, cameraSizeLarge.y));
		drawLine(camera.position.x, -center.y, camera.position.x + cameraSizeTiles.x, -center.y);
		drawLine(center.x, -camera.position.y, center.x, -camera.position.y - cameraSizeTiles.y);
		Draw::color(Color(0.0, 1.0, 0.0));
		strokeRect(Rect::fromSize(center.x - cameraSizeSmall.x * 0.5, -center.y - cameraSizeSmall.y * 0.5, cameraSizeSmall.x, cameraSizeSmall.y));
		Draw::color(Color(1.0));
		Fonts::rainworld->writeCentered(std::to_string(i), center.x, -center.y, 0.0625 * DropletWindow::cameraScale, CENTER_XY);
		i++;

		if (selected) {
			if (drawCameraAngle(camera.position.x,                     -camera.position.y,                     camera.angle0, draggingCamera == &camera && draggingCameraAngle == 0)) { newSelectedCamera = true; draggingCamera = &camera; draggingCameraAngle = 0; }
			if (drawCameraAngle(camera.position.x + cameraSizeTiles.x, -camera.position.y,                     camera.angle1, draggingCamera == &camera && draggingCameraAngle == 1)) { newSelectedCamera = true; draggingCamera = &camera; draggingCameraAngle = 1; }
			if (drawCameraAngle(camera.position.x + cameraSizeTiles.x, -camera.position.y - cameraSizeTiles.y, camera.angle2, draggingCamera == &camera && draggingCameraAngle == 2)) { newSelectedCamera = true; draggingCamera = &camera; draggingCameraAngle = 2; }
			if (drawCameraAngle(camera.position.x,                     -camera.position.y - cameraSizeTiles.y, camera.angle3, draggingCamera == &camera && draggingCameraAngle == 3)) { newSelectedCamera = true; draggingCamera = &camera; draggingCameraAngle = 3; }
		}

		if (draggingCamera == &camera && draggingCameraAngle == -1) {
			camera.position += (DropletWindow::transformedMouse - dragStart) * Vector2(1, -1);
			dragStart = DropletWindow::transformedMouse;
		}

		if (!newSelectedCamera && !DropletWindow::blockMouse && UI::mouse.justClicked() && Rect(camera.position.x, -camera.position.y, camera.position.x + cameraSizeTiles.x, -camera.position.y - cameraSizeTiles.y).inside(DropletWindow::transformedMouse.x, DropletWindow::transformedMouse.y)) {
			newSelectedCamera = true;
			DropletWindow::selectedCamera = &camera;
			draggingCamera = &camera;
			draggingCameraAngle = -1;
			dragStart = DropletWindow::transformedMouse;
		}
	}
	glDisable(GL_BLEND);

	if (UI::mouse.justClicked() && !newSelectedCamera) {
		DropletWindow::selectedCamera = nullptr;
	}

	if (UI::window->justPressed(GLFW_KEY_C)) {
		DropletWindow::Camera camera;
		camera.position = DropletWindow::transformedMouse * Vector2(1, -1) - cameraSizeTiles * 0.5;
		DropletWindow::cameras.push_back(camera);
		DropletWindow::selectedCamera = &DropletWindow::cameras[DropletWindow::cameras.size() - 1];
	}

	if (UI::window->justPressed(GLFW_KEY_X) && DropletWindow::selectedCamera != nullptr) {
		if (DropletWindow::cameras.size() == 1) {
			Popups::addPopup(new InfoPopup("Cannot delete last camera"));
		} else {
			DropletWindow::cameras.erase(std::remove_if(DropletWindow::cameras.begin(), DropletWindow::cameras.end(), [](const DropletWindow::Camera &other) {
				return &other == DropletWindow::selectedCamera;
			}), DropletWindow::cameras.end());
			DropletWindow::selectedCamera = nullptr;
		}
	}
}

void drawWater(bool border) {
	if (border) {
		Rect water = Rect(DropletWindow::roomRect.x0, DropletWindow::roomRect.y0, DropletWindow::roomRect.x1, DropletWindow::roomRect.y0 + (EditorState::dropletRoom->water + 0.5));
		Draw::color(0.0, 0.0, 0.5, 1.0);
		drawLine(water.x0, water.y1, water.x1, water.y1);
		return;
	}

	glEnable(GL_BLEND);
	Draw::color(0.0, 0.0, 0.5, 0.5);
	for (WaterSpot &spot : water) {
		Rect waterRect = Rect::fromSize(DropletWindow::roomRect.x0 + spot.pos.x / 20.0, DropletWindow::roomRect.y0 + spot.pos.y / 20.0, spot.size.x / 20.0, spot.size.y / 20.0);
		fillRect(waterRect);
	}
	glDisable(GL_BLEND);
}

void DropletWindow::Draw() {
	if (!UI::mouse.leftMouse && !UI::mouse.rightMouse) {
		if (UI::window->justPressed(GLFW_KEY_1)) currentTab = DropletWindow::EditorTab::DETAILS;
		if (UI::window->justPressed(GLFW_KEY_2)) currentTab = DropletWindow::EditorTab::GEOMETRY;
		if (UI::window->justPressed(GLFW_KEY_3)) currentTab = DropletWindow::EditorTab::CAMERA;
	}

	UpdateCamera();

	applyFrustumToOrthographic(cameraOffset, 0.0f, cameraScale * UI::screenBounds);

	roomRect = Rect::fromSize(0.0, 0.0, EditorState::dropletRoom->width, -EditorState::dropletRoom->height);

	setThemeColor(ThemeColour::RoomAir);
	fillRect(roomRect);

	Draw::color(currentTheme[ThemeColour::RoomAir].mix(currentTheme[ThemeColour::RoomSolid], 0.25));
	Draw::begin(Draw::QUADS);
	for (int x = 0; x < EditorState::dropletRoom->width; x++) {
		for (int y = 0; y < EditorState::dropletRoom->height; y++) {
			float x0 = roomRect.x0 + x;
			float y0 = roomRect.y1 - y;
			float x1 = x0 + 1;
			float y1 = y0 - 1;

			if ((EditorState::dropletRoom->getTile(x, y) & 512) > 0) {
				Draw::vertex(x0, y0);
				Draw::vertex(x1, y0);
				Draw::vertex(x1, y1);
				Draw::vertex(x0, y1);
			}
		}
	}
	Draw::end();

	if (!waterInFront && EditorState::dropletRoom->water != -1) {
		drawWater(false);
	}

	Draw::begin(Draw::QUADS);
	for (int x = 0; x < EditorState::dropletRoom->width; x++) {
		for (int y = 0; y < EditorState::dropletRoom->height; y++) {
			float x0 = roomRect.x0 + x;
			float y0 = roomRect.y1 - y;
			float x1 = x0 + 1;
			float y1 = y0 - 1;
			int geo = EditorState::dropletRoom->getTile(x, y);

			if (geo % 16 == 1) {
				setThemeColor(ThemeColour::RoomSolid);
				Draw::vertex(x0, y0);
				Draw::vertex(x1, y0);
				Draw::vertex(x1, y1);
				Draw::vertex(x0, y1);
			}
			else if (geo % 16 == 3) {
				setThemeColor(ThemeColour::RoomPlatform);
				Draw::vertex(x0, y0);
				Draw::vertex(x1, y0);
				Draw::vertex(x1, y0 - 0.5);
				Draw::vertex(x0, y0 - 0.5);
			}
			else if (geo % 16 == 2) {
				int type = (EditorState::dropletRoom->getTile(x, y) & (1024 + 2048)) / 1024;

				setThemeColor(ThemeColour::RoomSolid);
				if (type == 0) {
					Draw::vertex(x0, y1);
					Draw::vertex(x1, y0);
					Draw::vertex(x0, y0);
					Draw::vertex(x0, y1);
				} else if (type == 1) {
					Draw::vertex(x0, y0);
					Draw::vertex(x1, y1);
					Draw::vertex(x0, y1);
					Draw::vertex(x0, y0);
				} else if (type == 2) {
					Draw::vertex(x0, y0);
					Draw::vertex(x1, y0);
					Draw::vertex(x1, y1);
					Draw::vertex(x0, y0);
				} else if (type == 3) {
					Draw::vertex(x0, y1);
					Draw::vertex(x1, y0);
					Draw::vertex(x1, y1);
					Draw::vertex(x0, y1);
				}
			}
			else if (geo % 16 == 4) {
				setThemeColor(ThemeColour::RoomShortcutEnterance);
				Draw::vertex(x0, y0);
				Draw::vertex(x1, y0);
				Draw::vertex(x1, y1);
				Draw::vertex(x0, y1);
			}
			if ((geo & 16) > 0) {
				setThemeColor(ThemeColour::RoomPole);
				Draw::vertex(x0 + 0.4, y0);
				Draw::vertex(x1 - 0.4, y0);
				Draw::vertex(x1 - 0.4, y1);
				Draw::vertex(x0 + 0.4, y1);
			}
			if ((geo & 32) > 0) {
				setThemeColor(ThemeColour::RoomPole);
				Draw::vertex(x0, y0 - 0.4);
				Draw::vertex(x1, y0 - 0.4);
				Draw::vertex(x1, y1 + 0.4);
				Draw::vertex(x0, y1 + 0.4);
			}
		}
	}
	Draw::end();

	glEnable(GL_BLEND);
	Draw::useTexture(shortcutsTexture->ID());
	for (int x = 0; x < EditorState::dropletRoom->width; x++) {
		for (int y = 0; y < EditorState::dropletRoom->height; y++) {
			float x0 = roomRect.x0 + x;
			float y0 = roomRect.y1 - y;
			float x1 = x0 + 1;
			float y1 = y0 - 1;
			int geo = EditorState::dropletRoom->getTile(x, y);

			if ((geo & 15) == 4) {
				setThemeColor(ThemeColour::RoomShortcutRoom);
				if (EditorState::dropletRoom->getTile(x, y + 1) & 128) {
					fillRect(UVRect(x0, y0, x1, y1).uv(0.0, 0.0, 0.25, 0.25));
				}
				else if (EditorState::dropletRoom->getTile(x - 1, y) & 128) {
					fillRect(UVRect(x0, y0, x1, y1).uv(0.25, 0.0, 0.5, 0.25));
				}
				else if (EditorState::dropletRoom->getTile(x + 1, y) & 128) {
					fillRect(UVRect(x0, y0, x1, y1).uv(0.5, 0.0, 0.75, 0.25));
				}
				else if (EditorState::dropletRoom->getTile(x, y - 1) & 128) {
					fillRect(UVRect(x0, y0, x1, y1).uv(0.75, 0.0, 1.0, 0.25));
				}
				else {
					fillRect(UVRect(x0, y0, x1, y1).uv(0.25, 0.25, 0.5, 0.5));
				}
			}
			else if ((geo & 64) > 0) {
				setThemeColor(ThemeColour::RoomShortcutRoom);
				fillRect(UVRect(x0, y0, x1, y1).uv(0.0, 0.75, 0.25, 1.0));
			}
			else if ((geo & 128) > 0) {
				setThemeColor(ThemeColour::RoomShortcutDot);
				fillRect(UVRect(x0, y0, x1, y1).uv(0.0, 0.25, 0.25, 0.5));
			}

			if ((geo & 256) > 0) {
				setThemeColor(ThemeColour::RoomShortcutDen);
				fillRect(UVRect(x0, y0, x1, y1).uv(0.25, 0.75, 0.5, 1.0));
			}
			if ((geo & 4096) > 0) {
				setThemeColor(ThemeColour::RoomShortcutDen);
				fillRect(UVRect(x0, y0, x1, y1).uv(0.5, 0.75, 0.75, 1.0));
			}
			if ((geo & 8192) > 0) {
				setThemeColor(ThemeColour::RoomShortcutDen);
				fillRect(UVRect(x0, y0, x1, y1).uv(0.75, 0.75, 1.0, 1.0));
			}
			if ((geo & 16384) > 0) {
				setThemeColor(ThemeColour::RoomShortcutDot);
				fillRect(UVRect(x0, y0, x1, y1).uv(0.5, 0.25, 0.75, 0.5));
			}
			if ((geo & 32768) > 0) {
				setThemeColor(ThemeColour::RoomShortcutDot);
				fillRect(UVRect(x0, y0, x1, y1).uv(0.5, 0.5, 0.75, 0.75));
			}
			if ((geo & 65536) > 0) {
				setThemeColor(ThemeColour::RoomShortcutDot);
				fillRect(UVRect(x0, y0, x1, y1).uv(0.75, 0.5, 1.0, 0.75));
			}
			if ((geo & 262144) > 0) {
				setThemeColor(ThemeColour::RoomShortcutDot);
				fillRect(UVRect(x0, y0, x1, y1).uv(0.25, 0.5, 0.5, 0.75));
			}
			if ((geo & 524288) > 0) {
				setThemeColor(ThemeColour::RoomShortcutDot);
				fillRect(UVRect(x0, y0, x1, y1).uv(0.0, 0.5, 0.25, 0.75));
			}
		}
	}
	Draw::useTexture(0);

	if (EditorState::dropletRoom->water != -1) {
		if (waterInFront) {
			drawWater(false);
		}

		drawWater(true);
	}

	glDisable(GL_BLEND);

	setThemeColor(ThemeColour::RoomBorder);
	strokeRect(roomRect);

	transformedMouse = Vector2(
		UI::mouse.x * cameraScale + cameraOffset.x,
		UI::mouse.y * cameraScale + cameraOffset.y
	);
	lastMouseTile = mouseTile;
	mouseTile = {
		int(std::floor(UI::mouse.x * cameraScale + cameraOffset.x)),
		-int(std::ceil(UI::mouse.y * cameraScale + cameraOffset.y))
	};

	blockMouse = UI::mouse.y >= (UI::screenBounds.y - 0.12) || UI::mouse.x >= (UI::screenBounds.x - 0.41);

	if (currentTab == DropletWindow::EditorTab::DETAILS) UpdateDetailsTab();
	if (currentTab == DropletWindow::EditorTab::GEOMETRY) UpdateGeometryTab();
	if (currentTab == DropletWindow::EditorTab::CAMERA) UpdateCameraTab();

	// Draw UI
	applyFrustumToOrthographic(Vector2(0.0f, 0.0f), 0.0f, UI::screenBounds);
	glLineWidth(1);

	//-- Sidebar
	Rect sidebar(UI::screenBounds.x - 0.41, UI::screenBounds.y - 0.12, UI::screenBounds.x, -UI::screenBounds.y);
	setThemeColor(ThemeColour::Popup);
	fillRect(sidebar);
	setThemeColor(ThemeColour::Border);
	drawLine(sidebar.x0, sidebar.y0, sidebar.x0, sidebar.y1);

	if (currentTab == DropletWindow::EditorTab::GEOMETRY) {
		for (int y = 0; y < 4; y++) {
			for (int x = 0; x < 4; x++) {
				int i = x + y * 4;
				UVRect toolRect = UVRect::fromSize(sidebar.x0 + 0.01 + x * 0.1, sidebar.y1 - (y + 1) * 0.1, 0.09, 0.09);
				toolRect.uv(x * 0.25, y * 0.2 + 0.2, x * 0.25 + 0.25, y * 0.2);

				bool selected = DropletWindow::selectedTool == (DropletWindow::GeometryTool) i;
				UI::ButtonResponse response =  UI::TextureButton(toolRect, UI::TextureButtonMods().TextureId(DropletWindow::toolsTexture->ID()).TextureScale(0.75).Selected(selected).TextureColor(selected ? Color(1.0) : Color(0.5)));
				if (response.clicked) {
					DropletWindow::selectedTool = (DropletWindow::GeometryTool) i;
				}
			}
		}
	} else if (currentTab == DropletWindow::EditorTab::DETAILS) {
		bool hasWater = EditorState::dropletRoom->water != -1;

		UI::CheckBox(Rect::fromSize(sidebar.x0 + 0.01, sidebar.y1 - 0.06, 0.05, 0.05), enclosedRoom);
		UI::CheckBox(Rect::fromSize(sidebar.x0 + 0.01, sidebar.y1 - 0.12, 0.05, 0.05), hasWater);
		UI::CheckBox(Rect::fromSize(sidebar.x0 + 0.01, sidebar.y1 - 0.18, 0.05, 0.05), waterInFront);

		if (!hasWater) {
			EditorState::dropletRoom->water = -1;
		} else {
			if (EditorState::dropletRoom->water == -1) {
				EditorState::dropletRoom->water = EditorState::dropletRoom->height / 2;
			}
		}

		Draw::color(1.0, 1.0, 1.0);
		Fonts::rainworld->writeCentered("Enclosed Room", sidebar.x0 + 0.07, sidebar.y1 - 0.035, 0.03, CENTER_Y);
		Fonts::rainworld->writeCentered("Water", sidebar.x0 + 0.07, sidebar.y1 - 0.095, 0.03, CENTER_Y);
		Fonts::rainworld->writeCentered("Water in Front", sidebar.x0 + 0.07, sidebar.y1 - 0.155, 0.03, CENTER_Y);

		double barY = sidebar.y1 - 0.2;
		setThemeColor(ThemeColour::Border);
		drawLine(sidebar.x0, barY, sidebar.x1, sidebar.y1 - 0.2);
		if (UI::TextButton(Rect::fromSize(sidebar.x0 + 0.01, barY - 0.06, 0.39, 0.05), "Add TerrainHandle")) {
			TerrainHandleObject *object = new TerrainHandleObject();
			DropletWindow::objects.push_back(object);
			terrainHandleObjects.push_back(object);
		}
		if (UI::TextButton(Rect::fromSize(sidebar.x0 + 0.01, barY - 0.12, 0.39, 0.05), "Add MudPit")) {
			MudPitObject *object = new MudPitObject();
			DropletWindow::objects.push_back(object);
			mudPitObjects.push_back(object);
		}
		if (UI::TextButton(Rect::fromSize(sidebar.x0 + 0.01, barY - 0.18, 0.39, 0.05), "Add AirPocket")) {
			AirPocketObject *object = new AirPocketObject();
			DropletWindow::objects.push_back(object);
			airPocketObjects.push_back(object);
		}

		if (trashCanState > 0) {
			if (trashCanState == 2) {
				Draw::color(1.0, 0.0, 0.0);
			} else {
				Draw::color(1.0, 1.0, 1.0);
			}

			strokeRect(Rect::fromSize(-UI::screenBounds.x + 0.01, -UI::screenBounds.y + 0.01, 0.1, 0.1));
			Fonts::rainworld->writeCentered("Trash", -UI::screenBounds.x + 0.06, -UI::screenBounds.y + 0.13, 0.03, CENTER_XY);
		}
	}

	//-- Tabs
	Rect tabPositions(-UI::screenBounds.x, UI::screenBounds.y - 0.06, UI::screenBounds.x, UI::screenBounds.y - 0.12);
	setThemeColor(ThemeColour::Popup);
	fillRect(tabPositions);
	setThemeColor(ThemeColour::Border);
	drawLine(tabPositions.x0, tabPositions.y0, tabPositions.x1, tabPositions.y0);

	Vector2 tabPosition = Vector2(-UI::screenBounds.x + 0.01, UI::screenBounds.y - 0.12);
	double tabHeight = 0.05;
	for (int i = 0; i < 3; i++) {
		double tabWidth = std::max(0.15, Fonts::rainworld->getTextWidth(TAB_NAMES[i], 0.03) + 0.04);
		Rect tab = Rect::fromSize(tabPosition, Vector2(tabWidth, tabHeight));
		bool selected = i == (int) DropletWindow::currentTab;

		if (selected) {
			setThemeColor(ThemeColour::PopupHeader);
			fillRect(tab);
		}

		setThemeColor(selected ? ThemeColour::BorderHighlight : ThemeColour::Border);
		strokeRect(tab);

		if (selected) {
			setThemeColor(ThemeColour::PopupHeader);
			drawLine(tab.x0, tab.y0, tab.x1, tab.y0);
		}

		setThemeColor(selected ? ThemeColour::TextHighlight : ThemeColour::Text);
		Fonts::rainworld->write(TAB_NAMES[i], tabPosition.x + 0.02, tabPosition.y + 0.04, 0.03);

		if (tab.inside(UI::mouse) && UI::mouse.justClicked()) {
			currentTab = (DropletWindow::EditorTab) i;
		}

		tabPosition.x += tabWidth + 0.01;
	}
}

void setCameraAngle(std::string from, Vector2 &angle) {
	try {
		double theta = std::stod(from.substr(0, from.find(','))) * (3.141592653589 / 180.0);
		double radius = std::stod(from.substr(from.find(',') + 1));

		angle.x = sin(theta) * radius;
		angle.y = cos(theta) * radius;
	} catch (std::invalid_argument) {
		Logger::warn("Failed parsing camera angle: ", from);
	}
}

void backup() {
	if (DropletWindow::backupGeometry != nullptr) delete[] DropletWindow::backupGeometry;

	DropletWindow::backupGeometry = new int[EditorState::dropletRoom->width * EditorState::dropletRoom->height];
	for (int i = 0; i < EditorState::dropletRoom->width * EditorState::dropletRoom->height; i++) {
		DropletWindow::backupGeometry[i] = EditorState::dropletRoom->geometry[i];
	}
	DropletWindow::backupWater = EditorState::dropletRoom->water;
}

void DropletWindow::loadRoom() {
	cameras.clear();

	std::fstream geometryFile(EditorState::dropletRoom->path);
	if (!geometryFile.is_open() || !std::filesystem::exists(EditorState::dropletRoom->path)) {
		Logger::error("Failed to open droplet room file: ", EditorState::dropletRoom->path);
		return;
	}

	std::string tempLine;
	std::getline(geometryFile, tempLine);
	std::getline(geometryFile, tempLine);
	std::vector<std::string> waterItems = split(tempLine, '|');
	if (waterItems.size() == 3) {
		waterInFront = waterItems[2] == "1";
	} else {
		waterInFront = false;
	}
	std::getline(geometryFile, tempLine);
	std::getline(geometryFile, tempLine);

	std::vector<std::string> camerasData = split(tempLine, '|');
	Logger::info("Found ", camerasData.size(), " camera(s)");
	for (std::string cameraData : camerasData) {
		std::string xStr = cameraData.substr(0, cameraData.find(','));
		std::string yStr = cameraData.substr(cameraData.find(',') + 1);
		int x = 0, y = 0;
		try {
			x = std::stoi(xStr);
			y = std::stoi(yStr);
		} catch (std::invalid_argument) {
			Logger::warn("Can't open droplet room due to invalid camera positions (", xStr, ", ", yStr, ")");
		}

		Camera camera;
		camera.position = Vector2(x / 20.0, y / 20.0);
		cameras.push_back(camera);
	}

	std::getline(geometryFile, tempLine);
	enclosedRoom = tempLine.substr(tempLine.find(' ') + 1) == "Solid";

	for (int i = 0; i < 8; i++) std::getline(geometryFile, tempLine);
	if (!tempLine.empty() && startsWith(tempLine, "camera angles:")) {
		std::vector<std::string> angleData = split(tempLine.substr(tempLine.find(':') + 1), '|');
		for (int i = 0; i < cameras.size(); i++) {
			if (i >= angleData.size()) break;

			Camera &camera = cameras[i];
			std::vector<std::string> angles = split(angleData[i], ';');
			if (angles.size() != 4) {
				Logger::warn("Failed to parse camera ", i, "; Not enough camera angles");
				continue;
			}
			setCameraAngle(angles[0], camera.angle0);
			setCameraAngle(angles[1], camera.angle1);
			setCameraAngle(angles[2], camera.angle2);
			setCameraAngle(angles[3], camera.angle3);
		}
	}

	geometryFile.close();

	for (Object *object : objects) {
		delete object;
	}
	objects.clear();
	terrainHandleObjects.clear();
	mudPitObjects.clear();
	airPocketObjects.clear();

	std::filesystem::path settingsFilePath = findFileCaseInsensitive(EditorState::dropletRoom->path.parent_path(), EditorState::dropletRoom->roomName + "_settings.txt");
	std::ifstream settingsFile(settingsFilePath);
	while (std::getline(settingsFile, tempLine)) {
		if (startsWith(tempLine, "PlacedObjects: ")) {
			std::string data = tempLine.substr(tempLine.find(' ') + 1);
			std::vector<std::string> poData = split(data, ", ");
			for (std::string po : poData) {
				size_t start = po.find('<');
				size_t next = po.find('>', start);
				size_t end = po.find('>', next + 1);
				std::string xStr = po.substr(start + 1, next - start - 1);
				std::string yStr = po.substr(next + 2, end - next - 2);
				std::string last = po.substr(end + 2);
				Vector2 pos;
				try {
					pos.x = std::stod(xStr);
					pos.y = std::stod(yStr);
				} catch (std::invalid_argument) {
					Logger::warn("Failed to parse Placed Object: ", po);
				}
	
				if (startsWith(po, "TerrainHandle>")) {
					TerrainHandleObject *obj = new TerrainHandleObject();
					obj->nodes[0]->pos = pos;
					std::vector<std::string> splits = split(last, '~');
					if (splits.size() >= 4) {
						try {
							obj->nodes[1]->pos.x = std::stod(splits[0]);
							obj->nodes[1]->pos.y = std::stod(splits[1]);
							obj->nodes[2]->pos.x = std::stod(splits[2]);
							obj->nodes[2]->pos.y = std::stod(splits[3]);
						} catch (std::invalid_argument) {}
					}
					objects.push_back(obj);
					terrainHandleObjects.push_back(obj);
				} else if (startsWith(po, "MudPit>")) {
					MudPitObject *obj = new MudPitObject();
					obj->nodes[0]->pos = pos;
					std::vector<std::string> splits = split(last, '~');
					if (splits.size() >= 2) {
						try {
							obj->nodes[1]->pos.x = std::stod(splits[0]);
							obj->nodes[1]->pos.y = std::stod(splits[1]);
						} catch (std::invalid_argument) {}
					}
					objects.push_back(obj);
					mudPitObjects.push_back(obj);
				} else if (startsWith(po, "AirPocket>")) {
					AirPocketObject *obj = new AirPocketObject();
					obj->nodes[0]->pos = pos;
					std::vector<std::string> splits = split(last, '~');
					if (splits.size() >= 6) {
						try {
							obj->nodes[1]->pos.x = std::stod(splits[0]);
							obj->nodes[1]->pos.y = std::stod(splits[1]);
							obj->nodes[2]->pos.y = std::stod(splits[5]);
						} catch (std::invalid_argument) {}
					}
					objects.push_back(obj);
					airPocketObjects.push_back(obj);
				}
			}
		}
	}
	settingsFile.close();

	backup();
	terrainNeedsRefresh = true;
	waterNeedsRefresh = true;
}

void DropletWindow::resetChanges() {
	if (DropletWindow::backupGeometry == nullptr) return;

	for (int i = 0; i < EditorState::dropletRoom->width * EditorState::dropletRoom->height; i++) {
		EditorState::dropletRoom->geometry[i] = DropletWindow::backupGeometry[i];
	}
	EditorState::dropletRoom->water = DropletWindow::backupWater;

	delete[] DropletWindow::backupGeometry;
	DropletWindow::backupGeometry = nullptr;
}

void DropletWindow::exportGeometry() {
	std::filesystem::path geoPath = EditorState::region.roomsDirectory / (EditorState::dropletRoom->roomName + ".txt");
	Backup::backup(geoPath);
	std::ofstream geo(geoPath);
	geo << EditorState::dropletRoom->roomName << "\n";
	geo << EditorState::dropletRoom->width << "*" << EditorState::dropletRoom->height;
	geo << (EditorState::dropletRoom->water == -1 ? "" : ("|" + std::to_string(EditorState::dropletRoom->water) + "|" + (waterInFront ? "1" : "0"))) << "\n";
	geo << "0.0000*1.0000|0|0\n";
	{
		bool first = true;
		for (Camera camera : DropletWindow::cameras) {
			if (!first) geo << "|";
			first = false;
	
			geo << int(std::round(camera.position.x * 20.0)) << "," << int(std::round(camera.position.y * 20.0));
		}
		geo << "\n";
	}
	geo << "Border: " << (enclosedRoom ? "Solid" : "Passable") << "\n";
	for (int x = 0; x < EditorState::dropletRoom->width; x++) {
		for (int y = 0; y < EditorState::dropletRoom->height; y++) {
			int tile = EditorState::dropletRoom->getTile(x, y);
			if ((tile & 262144) > 0) {
				geo << "0," << (x + 1) << "," << (y + 1) << "|";
			}
			if ((tile & 524288) > 0) {
				geo << "1," << (x + 1) << "," << (y + 1) << "|";
			}
		}
	}
	geo << "\n";
	geo << "\n";
	geo << "\n";
	geo << "\n";
	geo << "0\n";
	geo << "\n";
	for (int x = 0; x < EditorState::dropletRoom->width; x++) {
		for (int y = 0; y < EditorState::dropletRoom->height; y++) {
			int tile = EditorState::dropletRoom->getTile(x, y);

			geo << std::to_string(tile % 16);
			if ((tile & 16) > 0) { // Vertical Pole
				geo << ",1";
			}
			if ((tile & 32) > 0) { // Horizontal Pole
				geo << ",2";
			}
			if ((tile & 64) > 0) { // Room Exit
				geo << ",4";
			}
			if ((tile & 128) > 0) { // Shortcut
				geo << ",3";
			}
			if ((tile & 256) > 0) {
				geo << ",5";
			}
			if ((tile & 512) > 0) {
				geo << ",6";
			}
			if ((tile & 4096) > 0) {
				geo << ",12";
			}
			if ((tile & 8192) > 0) {
				geo << ",9";
			}
			if ((tile & 16384) > 0) {
				geo << ",10";
			}
			if ((tile & 32768) > 0) {
				geo << ",11";
			}
			if ((tile & 65536) > 0) {
				geo << ",7";
			}
			if ((tile & 131072) > 0) {
				geo << ",8";
			}
			geo << "|";
		}
	}
	geo << "\n";
	{
		geo << "camera angles:";
		bool first = true;
		for (Camera camera : DropletWindow::cameras) {
			if (!first) geo << "|";
			first = false;
	
			geo << (std::atan2(camera.angle0.x, camera.angle0.y) * (180.0 / 3.141592653589)) << "," << camera.angle0.length() << ";";
			geo << (std::atan2(camera.angle1.x, camera.angle1.y) * (180.0 / 3.141592653589)) << "," << camera.angle1.length() << ";";
			geo << (std::atan2(camera.angle2.x, camera.angle2.y) * (180.0 / 3.141592653589)) << "," << camera.angle2.length() << ";";
			geo << (std::atan2(camera.angle3.x, camera.angle3.y) * (180.0 / 3.141592653589)) << "," << camera.angle3.length();
		}
		geo << "\n";
	}
	geo.close();

	backup();
	terrainNeedsRefresh = true;
	waterNeedsRefresh = true;

	std::filesystem::path settingsPath = EditorState::region.roomsDirectory / (EditorState::dropletRoom->roomName + "_settings.txt");
	Backup::backup(settingsPath);
	std::string before;
	std::string placedObjects;
	std::string after;
	if (std::filesystem::exists(settingsPath)) {
		std::ifstream settings(settingsPath);
		std::string tempLine;
		bool isBefore = true;
		while (std::getline(settings, tempLine)) {
			if (startsWith(tempLine, "PlacedObjects: ")) {
				placedObjects = tempLine;
				isBefore = false;
			} else {
				if (isBefore) {
					before += tempLine + "\n";
				} else {
					after += tempLine + "\n";
				}
			}
		}
		settings.close();
	}

	{
		std::string outputPlacedObjects;
		std::string data = placedObjects.substr(placedObjects.find(' ') + 1);
		std::vector<std::string> poData = split(data, ", ");
		std::vector<TerrainHandleObject *>::iterator currentTerrainHandleObject = terrainHandleObjects.begin();
		std::vector<MudPitObject *>::iterator currentMudPitObject = mudPitObjects.begin();
		std::vector<AirPocketObject *>::iterator currentAirPocketObject = airPocketObjects.begin();

		for (std::string po : poData) {
			size_t start = po.find('<');
			size_t next = po.find('>', start);
			size_t end = po.find('>', next + 1);
			std::string xStr = po.substr(start + 1, next - start - 1);
			std::string yStr = po.substr(next + 2, end - next - 2);
			std::string last = po.substr(end + 2);
		
			if (startsWith(po, "TerrainHandle>")) {
				if (currentTerrainHandleObject == terrainHandleObjects.end()) continue;

				std::vector<std::string> splits = split(last, '~');
				std::string height = "20";
				if (splits.size() >= 5) {
					height = splits[4];
				}
				TerrainHandleObject *handle = *currentTerrainHandleObject;
				outputPlacedObjects += "TerrainHandle><" + std::to_string(handle->nodes[0]->pos.x) + "><" + std::to_string(handle->nodes[0]->pos.y) + "><";
				outputPlacedObjects += std::to_string(handle->nodes[1]->pos.x) + "~" + std::to_string(handle->nodes[1]->pos.y) + "~";
				outputPlacedObjects += std::to_string(handle->nodes[2]->pos.x) + "~" + std::to_string(handle->nodes[2]->pos.y) + "~";
				outputPlacedObjects += height;

				currentTerrainHandleObject = std::next(currentTerrainHandleObject);
			} else if (startsWith(po, "MudPit>")) {
				if (currentMudPitObject == mudPitObjects.end()) continue;

				std::vector<std::string> splits = split(last, '~');
				std::string decalSize = "15.0";
				if (splits.size() >= 3) {
					decalSize = splits[2];
				}
				MudPitObject *mud = *currentMudPitObject;
				outputPlacedObjects += "MudPit><" + std::to_string(mud->nodes[0]->pos.x) + "><" + std::to_string(mud->nodes[0]->pos.y) + "><";
				outputPlacedObjects += std::to_string(mud->nodes[1]->pos.x) + "~" + std::to_string(mud->nodes[1]->pos.y) + "~";
				outputPlacedObjects += decalSize;

				currentMudPitObject = std::next(currentMudPitObject);
			} else if (startsWith(po, "AirPocket>")) {
				if (currentAirPocketObject == airPocketObjects.end()) continue;

				std::vector<std::string> splits = split(last, '~');
				std::string panelPosX = "30.0";
				std::string panelPosY = "30.0";
				std::string flood = "Y";
				if (splits.size() >= 6) {
					panelPosX = splits[2];
					panelPosY = splits[3];
					flood = splits[4];
				}
				AirPocketObject *pocket = *currentAirPocketObject;
				outputPlacedObjects += "AirPocket><" + std::to_string(pocket->nodes[0]->pos.x) + "><" + std::to_string(pocket->nodes[0]->pos.y) + "><";
				outputPlacedObjects += std::to_string(pocket->nodes[1]->pos.x) + "~" + std::to_string(pocket->nodes[1]->pos.y) + "~";
				outputPlacedObjects += panelPosX + "~" + panelPosY + "~" + flood + "~" + std::to_string(pocket->nodes[2]->pos.y);

				currentAirPocketObject = std::next(currentAirPocketObject);
			} else {
				outputPlacedObjects += po;
			}

			outputPlacedObjects += ", ";
		}

		while (currentTerrainHandleObject != terrainHandleObjects.end()) {
			TerrainHandleObject *handle = *currentTerrainHandleObject;
			outputPlacedObjects += "TerrainHandle><" + std::to_string(handle->nodes[0]->pos.x) + "><" + std::to_string(handle->nodes[0]->pos.y) + "><";
			outputPlacedObjects += std::to_string(handle->nodes[1]->pos.x) + "~" + std::to_string(handle->nodes[1]->pos.y) + "~";
			outputPlacedObjects += std::to_string(handle->nodes[2]->pos.x) + "~" + std::to_string(handle->nodes[2]->pos.y) + "~";
			outputPlacedObjects += "20";
			outputPlacedObjects += ", ";

			currentTerrainHandleObject = std::next(currentTerrainHandleObject);
		}

		while (currentMudPitObject != mudPitObjects.end()) {
			MudPitObject *mud = *currentMudPitObject;
			outputPlacedObjects += "MudPit><" + std::to_string(mud->nodes[0]->pos.x) + "><" + std::to_string(mud->nodes[0]->pos.y) + "><";
			outputPlacedObjects += std::to_string(mud->nodes[1]->pos.x) + "~" + std::to_string(mud->nodes[1]->pos.y) + "~";
			outputPlacedObjects += "15";
			outputPlacedObjects += ", ";

			currentMudPitObject = std::next(currentMudPitObject);
		}

		while (currentAirPocketObject != airPocketObjects.end()) {
			AirPocketObject *pocket = *currentAirPocketObject;
			outputPlacedObjects += "AirPocket><" + std::to_string(pocket->nodes[0]->pos.x) + "><" + std::to_string(pocket->nodes[0]->pos.y) + "><";
			outputPlacedObjects += std::to_string(pocket->nodes[1]->pos.x) + "~" + std::to_string(pocket->nodes[1]->pos.y) + "~";
			outputPlacedObjects += "30.0~30.0~Y~" + std::to_string(pocket->nodes[2]->pos.y);

			currentAirPocketObject = std::next(currentAirPocketObject);
		}

		placedObjects = outputPlacedObjects;
	}

	std::ofstream settings(settingsPath);
	settings << before << "PlacedObjects: " << placedObjects << "\n" << after;
	settings.close();
}

#define CAMERA_TEXTURE_WIDTH 1400
#define CAMERA_TEXTURE_HEIGHT 800

bool validSlopePos(int geo, Vector2 tp) {
	int type = (geo & (1024 | 2048)) / 1024;

	double x = std::fmod(tp.x - 0.5, 1.0);
	double y = std::fmod(tp.y - 0.5, 1.0);
	switch (type) {
		case 0: return 1.0 - x > y;
		case 1: return 1.0 - x > 1.0 - y;
		case 2: return x > y;
		case 3: return x > 1.0 - y;
	}

	return false;
}

void renderCamera(DropletWindow::Camera &camera, std::filesystem::path outputPath) {
	std::vector<unsigned char> image(CAMERA_TEXTURE_WIDTH * CAMERA_TEXTURE_HEIGHT * 3);

	// (121, 0, 0) -> L1 solid
	// (131, 0, 0) -> L2 solid

	// (91, 0, 0) -> L1 dark
	// (151, 0, 0) -> L1 light
	// (101, 0, 0) -> L2 dark
	// (161, 0, 0) -> L2 light

	for (int x = 0; x < CAMERA_TEXTURE_WIDTH; x++) {
		for (int y = 0; y < CAMERA_TEXTURE_HEIGHT; y++) {
			int id = (x + y * CAMERA_TEXTURE_WIDTH) * 3;
			Vector2 tp = Vector2(
				camera.position.x + x * 1.0 / 20.0,
				camera.position.y + y * 1.0 / 20.0
			);
			Vector2i tile = Vector2i(std::round(tp.x), std::round(tp.y));
			int geo = EditorState::dropletRoom->getTile(tile.x, tile.y);
			if ((geo & 128) > 0 && (std::abs(std::fmod(tp.y + 0.5, 1.0) - 0.5) + std::abs(std::fmod(tp.x + 0.5, 1.0) - 0.5)) < 0.25) {
				image[id + 0] = 31;
				image[id + 1] = 8;
				image[id + 2] = 0;
			} else if (geo % 16 == 1 || geo % 16 == 4) {
				image[id + 0] = 121;
				image[id + 1] = 0;
				image[id + 2] = 0;
			} else if (geo % 16 == 3 && std::fmod(tp.y, 1.0) > 0.5) {
				image[id + 0] = 157;
				image[id + 1] = 16;
				image[id + 2] = 0;
			} else if (geo % 16 == 2 && validSlopePos(geo, tp)) {
				image[id + 0] = 121;
				image[id + 1] = 0;
				image[id + 2] = 0;
			} else if ((geo & 16) > 0 && std::abs(std::fmod(tp.x + 0.5, 1.0) - 0.5) < 0.1) {
				image[id + 0] = 95;
				image[id + 1] = 0;
				image[id + 2] = 0;
			} else if ((geo & 32) > 0 && std::abs(std::fmod(tp.y + 0.5, 1.0) - 0.5) < 0.1) {
				image[id + 0] = 95;
				image[id + 1] = 0;
				image[id + 2] = 0;
			} else {
				if ((geo & 512) > 0) {
					image[id + 0] = 131;
					image[id + 1] = 0;
					image[id + 2] = 0;
				} else {
					image[id + 0] = 255;
					image[id + 1] = 255;
					image[id + 2] = 255;
				}
			}
		}
	}

	Backup::backup(outputPath);
	if (stbi_write_png(outputPath.generic_u8string().c_str(), CAMERA_TEXTURE_WIDTH, CAMERA_TEXTURE_HEIGHT, 3, image.data(), CAMERA_TEXTURE_WIDTH * 3)) {
		Logger::info("Screen exported");
	} else {
		Logger::error("Exporting screen failed");
	}
}

void DropletWindow::render() {
	exportGeometry();

	for (int i = 0; i < cameras.size(); i++) {
		renderCamera(cameras[i], EditorState::region.roomsDirectory / (EditorState::dropletRoom->roomName + "_" + std::to_string(i + 1) + ".png"));
	}
}

void DropletWindow::exportProject(std::filesystem::path path) {
	std::ofstream project(path / (EditorState::dropletRoom->roomName + ".txt"));
	project << std::setprecision(0);
	project << "[";
	for (int x = -12; x < EditorState::dropletRoom->width + 12; x++) {
		if (x != -12) project << ", ";

		project << "[";
		for (int y = -3; y < EditorState::dropletRoom->height + 5; y++) {
			if (y != -3) project << ", ";
			int geo = EditorState::dropletRoom->getTile(x, y);
			int solidA = 0;
			std::vector<std::string> flags;
			if (geo % 16 == 1) solidA = 1;
			else if (geo % 16 == 3) solidA = 6;
			else if (geo % 16 == 2) solidA = 2 + ((geo & 2048) > 0) + ((geo & 1024) > 0 ? 0 : 2);
			else if (geo % 16 == 4) { solidA = 7; flags.push_back("4"); }
			if ((geo & 16) > 0) flags.push_back("2");
			if ((geo & 32) > 0) flags.push_back("1");
			if ((geo & 64) > 0) flags.push_back("6");
			if ((geo & 128) > 0) flags.push_back("5");
			if ((geo & 256) > 0) flags.push_back("7");
			if ((geo & 4096) > 0) flags.push_back("21");
			if ((geo & 8192) > 0) flags.push_back("19");
			if ((geo & 16384) > 0) flags.push_back("13");
			if ((geo & 32768) > 0) flags.push_back("20");
			if ((geo & 65536) > 0) flags.push_back("3");
			if ((geo & 131072) > 0) flags.push_back("18");
			if ((geo & 262144) > 0) flags.push_back("9");
			if ((geo & 524288) > 0) flags.push_back("10");

			project << "[";
			project << "[" << solidA << ", [";
			bool first = true;
			for (std::string flag : flags) {
				if (!first) project << ", ";
				first = false;
				project << flag;
			}
			project << "]], ";
			project << "[" << (((geo & 512) > 0) ? "1" : "0") << ", []], ";
			project << "[0, []]";
			project << "]";
		}
		project << "]";
	}
	project << "]\n";
	project << "[#lastKeys: [], #Keys: [], #workLayer: 1, #lstMsPs: point(0, 0), #tlMatrix: [], #defaultMaterial: \"Standard\", #toolType: \"material\", #toolData: \"Big Metal\", #tmPos: point(1, 1), #tmSavPosL: [], #specialEdit: 0]\n";
	project << "[#lastKeys: [], #Keys: [], #lstMsPs: point(0, 0), #effects: [], #emPos: point(1, 1), #editEffect: 0, #selectEditEffect: 0, #mode: \"createNew\", #brushSize: 5]\n";
	project << "[#pos: point(0, 0), #rot: 0, #sz: point(" << std::to_string(EditorState::dropletRoom->width) << ", " << std::to_string(EditorState::dropletRoom->height) << "), #col: 1, #Keys: [#m1: 0, #m2: 0, #w: 0, #a: 0, #s: 0, #d: 0, #r: 0, #f: 0, #z: 0, #m: 0], #lastKeys: [#m1: 0, #m2: 0, #w: 0, #a: 0, #s: 0, #d: 0, #r: 0, #f: 0, #z: 0, #m: 0], #lastTm: 0, #lightAngle: 180, #flatness: 1, #lightRect: rect(1000, 1000, -1000, -1000), #paintShape: \"pxl\"]\n";
	project << "[#timeLimit: 4800, #defaultTerrain: " << (enclosedRoom ? "1" : "0") << ", #maxFlies: 10, #flySpawnRate: 50, #lizards: [], #ambientSounds: [], #music: \"NONE\", #tags: [], #lightType: \"Static\", #waterDrips: 1, #lightRect: rect(0, 0, 1040, 800), #Matrix: []]\n";
	project << "[#mouse: 1, #lastMouse: 0, #mouseClick: 0, #pal: 1, #pals: [[#detCol: color( 255, 0, 0 )]], #eCol1: 1, #eCol2: 2, #totEcols: 5, #tileSeed: 225, #colGlows: [0, 0], #size: point(" << std::to_string(EditorState::dropletRoom->width + 24) << ", " << std::to_string(EditorState::dropletRoom->height + 8) << "), #extraTiles: [12, 3, 12, 5], #light: 1]\n";
	project << "[#cameras: [";
	{
		bool first = true;
		for (Camera &camera : cameras) {
			if (!first) project << ", ";
			first = false;
			project << "point(" << std::to_string((camera.position.x + 12.0) * 20.0) << ", " << std::to_string((camera.position.y + 3.0) * 20.0) << ")";
		}
	}
	project << "], #selectedCamera: 0, #quads: [";
	{
		bool first = true;
		for (Camera &camera : cameras) {
			if (!first) project << ", ";
			first = false;
			project << "[[" << (std::atan2(camera.angle0.x, camera.angle0.y) * (180.0 / 3.141592653589)) << ", " << camera.angle0.length() << "], ";
			project << "[" << (std::atan2(camera.angle1.x, camera.angle1.y) * (180.0 / 3.141592653589)) << ", " << camera.angle1.length() << "], ";
			project << "[" << (std::atan2(camera.angle2.x, camera.angle2.y) * (180.0 / 3.141592653589)) << ", " << camera.angle2.length() << "], ";
			project << "[" << (std::atan2(camera.angle3.x, camera.angle3.y) * (180.0 / 3.141592653589)) << ", " << camera.angle3.length() << "]]";
		}
	}
	project << "], #Keys: [#n: 0, #d: 0, #e: 0, #p: 0], #lastKeys: [#n: 0, #d: 0, #e: 0, #p: 0]]\n";
	project << "[#waterLevel: " << EditorState::dropletRoom->water << ", #waterInFront: " << (waterInFront ? "1" : "0") << ", #waveLength: 60, #waveAmplitude: 5, #waveSpeed: 10]\n";
	project << "[#props: [], #lastKeys: [#w: 0, #a: 0, #s: 0, #d: 0, #L: 0, #n: 0, #m1: 0, #m2: 0, #c: 0, #z: 0], #Keys: [#w: 0, #a: 0, #s: 0, #d: 0, #L: 0, #n: 0, #m1: 0, #m2: 0, #c: 0, #z: 0], #workLayer: 1, #lstMsPs: point(0, 0), #pmPos: point(1, 1), #pmSavPosL: [1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1], #propRotation: 0, #propStretchX: 1, #propStretchY: 1, #propFlipX: 1, #propFlipY: 1, #depth: 0, #color: 0]\n";
	project.close();
}