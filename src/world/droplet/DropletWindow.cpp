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

std::string DropletWindow::TAB_NAMES[4] = { "Environment", "Geometry", "Cameras", "Generator" };
std::string DropletWindow::GEOMETRY_TOOL_NAMES[16] = { "Wall", "Slope", "Platform", "Background Wall", "Horizontal Pole", "Vertical Pole", "Spear", "Rock", "Shortcut", "Room Exit", "Creature Den", "Wack a Mole Hole", "Scavenger Den", "Garbage Worm Den", "Wormgrass", "Batfly Hive" };

DropletWindow::GeometryTool DropletWindow::selectedTool = DropletWindow::GeometryTool::WALL;

std::vector<DropletWindow::Camera> DropletWindow::cameras;
DropletWindow::Camera *DropletWindow::selectedCamera = nullptr;

void DropletWindow::init() {
	toolsTexture = new Texture(BASE_PATH / "assets" / "tools.png");
	shortcutsTexture = new Texture(BASE_PATH / "assets" / "shortcuts.png");
}

void DropletWindow::cleanup() {
	delete toolsTexture;
	delete shortcutsTexture;
}

void DropletWindow::UpdateCamera() {
	bool isHoveringPopup = false;
	for (Popup *popup : Popups::popups) {
		Rect bounds = popup->Bounds();

		if (bounds.inside(UI::mouse)) {
			isHoveringPopup = true;
			break;
		}
	}

	double scrollY = -EditorState::window->getMouseScrollY();
	double zoom = std::pow(1.25, scrollY);

	Vector2 previousWorldMouse = Vector2(
		UI::mouse.x * cameraScale + cameraOffset.x,
		UI::mouse.y * cameraScale + cameraOffset.y
	);

	cameraScaleTo *= zoom;
	cameraScaleTo = std::clamp(cameraScaleTo, 2.5, 1.0 * std::max(EditorState::dropletRoom->width, EditorState::dropletRoom->height));
	cameraScale += (cameraScaleTo - cameraScale) * Settings::getSetting<double>(Settings::Setting::CameraZoomSpeed);

	Vector2 worldMouse = Vector2(
		UI::mouse.x * cameraScale + cameraOffset.x,
		UI::mouse.y * cameraScale + cameraOffset.y
	);

	cameraOffset.x += previousWorldMouse.x - worldMouse.x;
	cameraOffset.y += previousWorldMouse.y - worldMouse.y;
	cameraPanTo.x += previousWorldMouse.x - worldMouse.x;
	cameraPanTo.y += previousWorldMouse.y - worldMouse.y;

	// Panning
	if (EditorState::mouse->Middle()) {
		if (!cameraPanningBlocked && !cameraPanning) {
			if (isHoveringPopup) cameraPanningBlocked = true;

			if (!cameraPanningBlocked) {
				cameraPanStart.x = cameraOffset.x;
				cameraPanStart.y = cameraOffset.y;
				cameraPanStartMouse.x = EditorState::globalMouse.x;
				cameraPanStartMouse.y = EditorState::globalMouse.y;
				cameraPanning = true;
			}
		}

		if (cameraPanning && !cameraPanningBlocked) {
			cameraPanTo.x = cameraPanStart.x + cameraScale * (cameraPanStartMouse.x - EditorState::globalMouse.x) / 512.0;
			cameraPanTo.y = cameraPanStart.y + cameraScale * (cameraPanStartMouse.y - EditorState::globalMouse.y) / -512.0;
		}
	} else {
		cameraPanning = false;
		cameraPanningBlocked = false;
	}

	cameraOffset.x += (cameraPanTo.x - cameraOffset.x) * Settings::getSetting<double>(Settings::Setting::CameraPanSpeed);
	cameraOffset.y += (cameraPanTo.y - cameraOffset.y) * Settings::getSetting<double>(Settings::Setting::CameraPanSpeed);
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

void applyTool(int x, int y, DropletWindow::GeometryTool tool) {
	if (tool == DropletWindow::GeometryTool::WALL) {
		EditorState::dropletRoom->geometry[x * EditorState::dropletRoom->height + y] = UI::mouse.rightMouse ? 0 : 1;
	}
	else if (tool == DropletWindow::GeometryTool::SLOPE) {
		if (UI::mouse.rightMouse) {
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
	else if (tool == DropletWindow::GeometryTool::PLATFORM) {
		EditorState::dropletRoom->geometry[x * EditorState::dropletRoom->height + y] = UI::mouse.rightMouse ? 0 : 3;
	}
	else if (tool == DropletWindow::GeometryTool::VERTICAL_POLE) {
		if (UI::mouse.rightMouse) {
			EditorState::dropletRoom->geometry[x * EditorState::dropletRoom->height + y] &= ~16;
		} else {
			EditorState::dropletRoom->geometry[x * EditorState::dropletRoom->height + y] |= 16;
		}
	}
	else if (tool == DropletWindow::GeometryTool::HORIZONTAL_POLE) {
		if (UI::mouse.rightMouse) {
			EditorState::dropletRoom->geometry[x * EditorState::dropletRoom->height + y] &= ~32;
		} else {
			EditorState::dropletRoom->geometry[x * EditorState::dropletRoom->height + y] |= 32;
		}
	}
	else if (tool == DropletWindow::GeometryTool::BACKGROUND_WALL) {
		if (UI::mouse.rightMouse) {
			EditorState::dropletRoom->geometry[x * EditorState::dropletRoom->height + y] &= ~512;
		} else {
			EditorState::dropletRoom->geometry[x * EditorState::dropletRoom->height + y] |= 512;
		}
	}
	else if (tool == DropletWindow::GeometryTool::ROOM_EXIT) {
		if (UI::mouse.rightMouse) {
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
	else if (tool == DropletWindow::GeometryTool::CREATURE_DEN) {
		if (UI::mouse.rightMouse) {
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
	else if (tool == DropletWindow::GeometryTool::SCAVENGER_DEN) {
		if (UI::mouse.rightMouse) {
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
	else if (tool == DropletWindow::GeometryTool::WACK_A_MOLE) {
		if (UI::mouse.rightMouse) {
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
	else if (tool == DropletWindow::GeometryTool::GARBAGE_WORM_DEN) {
		if (UI::mouse.rightMouse) {
			EditorState::dropletRoom->geometry[x * EditorState::dropletRoom->height + y] &= ~16384;
		} else {
			EditorState::dropletRoom->geometry[x * EditorState::dropletRoom->height + y] |= 16384;
		}
	}
	else if (tool == DropletWindow::GeometryTool::WORMGRASS) {
		if (UI::mouse.rightMouse) {
			EditorState::dropletRoom->geometry[x * EditorState::dropletRoom->height + y] &= ~32768;
		} else {
			EditorState::dropletRoom->geometry[x * EditorState::dropletRoom->height + y] |= 32768;
		}
	}
	else if (tool == DropletWindow::GeometryTool::BATFLY_HIVE) {
		if (UI::mouse.rightMouse) {
			EditorState::dropletRoom->geometry[x * EditorState::dropletRoom->height + y] &= ~65536;
		} else {
			EditorState::dropletRoom->geometry[x * EditorState::dropletRoom->height + y] |= 65536;
		}
	}
	else if (tool == DropletWindow::GeometryTool::SHORTCUT) {
		if (UI::mouse.rightMouse) {
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
	else if (tool == DropletWindow::GeometryTool::ROCK) {
		if (UI::mouse.rightMouse) {
			EditorState::dropletRoom->geometry[x * EditorState::dropletRoom->height + y] &= ~262144;
		} else {
			EditorState::dropletRoom->geometry[x * EditorState::dropletRoom->height + y] |= 262144;
		}
	}
	else if (tool == DropletWindow::GeometryTool::SPEAR) {
		if (UI::mouse.rightMouse) {
			EditorState::dropletRoom->geometry[x * EditorState::dropletRoom->height + y] &= ~524288;
		} else {
			EditorState::dropletRoom->geometry[x * EditorState::dropletRoom->height + y] |= 524288;
		}
	}
}

void DropletWindow::UpdateGeometryTab() {
	if (!(UI::mouse.leftMouse || UI::mouse.rightMouse)) {
		if (EditorState::window->justPressed(GLFW_KEY_A)) {
			DropletWindow::selectedTool = (DropletWindow::GeometryTool) (((int) DropletWindow::selectedTool + 15) % 16);
		}
	
		if (EditorState::window->justPressed(GLFW_KEY_D)) {
			DropletWindow::selectedTool = (DropletWindow::GeometryTool) (((int) DropletWindow::selectedTool + 1) % 16);
		}
	
		if (EditorState::window->justPressed(GLFW_KEY_W)) {
			DropletWindow::selectedTool = (DropletWindow::GeometryTool) (((int) DropletWindow::selectedTool + 12) % 16);
		}
	
		if (EditorState::window->justPressed(GLFW_KEY_S)) {
			DropletWindow::selectedTool = (DropletWindow::GeometryTool) (((int) DropletWindow::selectedTool + 4) % 16);
		}
	}

	if (!blockMouse) {
		if ((UI::mouse.leftMouse || UI::mouse.rightMouse) && (lastMouseTile.x != mouseTile.x || lastMouseTile.y != mouseTile.y)) {
			std::vector<Vector2i> drawLine;
			drawLine = LevelUtils::line(lastMouseTile.x, lastMouseTile.y, mouseTile.x, mouseTile.y);
	
			for (Vector2i point : drawLine) {
				if (EditorState::dropletRoom->InBounds(point.x, point.y)) {
					applyTool(point.x, point.y, DropletWindow::selectedTool);
				}
			}
		} else if ((UI::mouse.leftMouse && !UI::mouse.lastLeftMouse) || (UI::mouse.rightMouse && !UI::mouse.lastRightMouse)) {
			if (EditorState::dropletRoom->InBounds(mouseTile.x, mouseTile.y)) {
				applyTool(mouseTile.x, mouseTile.y, DropletWindow::selectedTool);
			}
		}
	
		setThemeColor(ThemeColour::RoomBorderHighlight);
		strokeRect(Rect::fromSize(roomRect.x0 + mouseTile.x, roomRect.y1 - mouseTile.y - 1, 1.0, 1.0));
	}

	lastMouseDrawing = UI::mouse.leftMouse || UI::mouse.rightMouse;
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
		if (!EditorState::window->modifierPressed(GLFW_MOD_SHIFT)) {
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

void DropletWindow::UpdateCameraTab() {
	static Camera *draggingCamera = nullptr;
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
	for (Camera &camera : DropletWindow::cameras) {
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
		Fonts::rainworld->writeCentered(std::to_string(i), center.x, -center.y, 0.0625 * cameraScale, CENTER_XY);
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

		if (!newSelectedCamera && !DropletWindow::blockMouse && UI::mouse.justClicked() && Rect(camera.position.x, -camera.position.y, camera.position.x + cameraSizeTiles.x, -camera.position.y - cameraSizeTiles.y).inside(transformedMouse.x, transformedMouse.y)) {
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

	if (EditorState::window->justPressed(GLFW_KEY_C)) {
		Camera camera;
		camera.position = DropletWindow::transformedMouse * Vector2(1, -1) - cameraSizeTiles * 0.5;
		DropletWindow::cameras.push_back(camera);
		DropletWindow::selectedCamera = &DropletWindow::cameras[DropletWindow::cameras.size() - 1];
	}

	if (EditorState::window->justPressed(GLFW_KEY_X) && DropletWindow::selectedCamera != nullptr) {
		if (DropletWindow::cameras.size() == 1) {
			Popups::addPopup(new InfoPopup(EditorState::window, "Cannot delete last camera"));
		} else {
			DropletWindow::cameras.erase(std::remove_if(DropletWindow::cameras.begin(), DropletWindow::cameras.end(), [](const DropletWindow::Camera &other) {
				return &other == DropletWindow::selectedCamera;
			}), DropletWindow::cameras.end());
			DropletWindow::selectedCamera = nullptr;
		}
	}
}

void DropletWindow::Draw() {
	if (EditorState::window->justPressed(GLFW_KEY_1)) currentTab = DropletWindow::EditorTab::DETAILS;
	if (EditorState::window->justPressed(GLFW_KEY_2)) currentTab = DropletWindow::EditorTab::GEOMETRY;
	if (EditorState::window->justPressed(GLFW_KEY_3)) currentTab = DropletWindow::EditorTab::CAMERA;
	if (EditorState::window->justPressed(GLFW_KEY_4)) currentTab = DropletWindow::EditorTab::GENERATOR;

	UpdateCamera();

	applyFrustumToOrthographic(cameraOffset, 0.0f, cameraScale * EditorState::screenBounds);

	roomRect = Rect::fromSize(0.0, 0.0, EditorState::dropletRoom->width, -EditorState::dropletRoom->height);

	setThemeColor(ThemeColour::RoomSolid);
	fillRect(roomRect);
	int id = 0;
	Draw::begin(Draw::QUADS);
	for (int x = 0; x < EditorState::dropletRoom->width; x++) {
		for (int y = 0; y < EditorState::dropletRoom->height; y++) {
			float x0 = roomRect.x0 + x;
			float y0 = roomRect.y1 - y;
			float x1 = x0 + 1;
			float y1 = y0 - 1;
			int geo = EditorState::dropletRoom->geometry[id];

			Color airColor = currentTheme[ThemeColour::RoomAir];
			if ((geo & 512) > 0) {
				airColor = airColor.mix(currentTheme[ThemeColour::RoomSolid], 0.25);
			}

			if (geo % 16 == 0) {
				Draw::color(airColor);
				Draw::vertex(x0, y0);
				Draw::vertex(x1, y0);
				Draw::vertex(x1, y1);
				Draw::vertex(x0, y1);
			}
			else if (geo % 16 == 3) {
				Draw::color(airColor);
				Draw::vertex(x0, y1);
				Draw::vertex(x1, y1);
				Draw::vertex(x1, y1 + 0.5);
				Draw::vertex(x0, y1 + 0.5);

				setThemeColor(ThemeColour::RoomPlatform);
				Draw::vertex(x0, y0);
				Draw::vertex(x1, y0);
				Draw::vertex(x1, y0 - 0.5);
				Draw::vertex(x0, y0 - 0.5);
			}
			else if (geo % 16 == 2) {
				int type = (EditorState::dropletRoom->getTile(x, y) & (1024 + 2048)) / 1024;

				Draw::color(airColor);
				if (type == 0) {
					Draw::vertex(x0, y1);
					Draw::vertex(x1, y0);
					Draw::vertex(x1, y1);
					Draw::vertex(x0, y1);
				} else if (type == 1) {
					Draw::vertex(x0, y0);
					Draw::vertex(x1, y0);
					Draw::vertex(x1, y1);
					Draw::vertex(x0, y0);
				} else if (type == 2) {
					Draw::vertex(x0, y0);
					Draw::vertex(x1, y1);
					Draw::vertex(x0, y1);
					Draw::vertex(x0, y0);
				} else if (type == 3) {
					Draw::vertex(x0, y1);
					Draw::vertex(x1, y0);
					Draw::vertex(x0, y0);
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

			id++;
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

	blockMouse = UI::mouse.y >= (EditorState::screenBounds.y - 0.12) || UI::mouse.x >= (EditorState::screenBounds.x - 0.41);

	if (currentTab == DropletWindow::EditorTab::GEOMETRY) UpdateGeometryTab();
	if (currentTab == DropletWindow::EditorTab::CAMERA) UpdateCameraTab();

	// Draw UI
	applyFrustumToOrthographic(Vector2(0.0f, 0.0f), 0.0f, EditorState::screenBounds);
	glLineWidth(1);

	//-- Sidebar
	Rect sidebar(EditorState::screenBounds.x - 0.41, EditorState::screenBounds.y - 0.12, EditorState::screenBounds.x, -EditorState::screenBounds.y);
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
	}

	//-- Tabs
	Rect tabPositions(-EditorState::screenBounds.x, EditorState::screenBounds.y - 0.06, EditorState::screenBounds.x, EditorState::screenBounds.y - 0.12);
	setThemeColor(ThemeColour::Popup);
	fillRect(tabPositions);
	setThemeColor(ThemeColour::Border);
	drawLine(tabPositions.x0, tabPositions.y0, tabPositions.x1, tabPositions.y0);

	Vector2 tabPosition = Vector2(-EditorState::screenBounds.x + 0.01, EditorState::screenBounds.y - 0.12);
	double tabHeight = 0.05;
	for (int i = 0; i < 4; i++) {
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

void DropletWindow::loadRoom() {
	cameras.clear();

	std::fstream geometryFile(EditorState::dropletRoom->path);
	if (!geometryFile.is_open() || !std::filesystem::exists(EditorState::dropletRoom->path)) {
		Logger::error("Failed to open droplet room file: ", EditorState::dropletRoom->path);
		return;
	}

	std::string tempLine;
	for (int i = 0; i < 4; i++) std::getline(geometryFile, tempLine);
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

	for (int i = 0; i < 9; i++) std::getline(geometryFile, tempLine);
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
}

void DropletWindow::exportGeometry() {
	std::ofstream geo(EditorState::region.roomsDirectory / (EditorState::dropletRoom->roomName + ".txt"));
	geo << EditorState::dropletRoom->roomName << "\n";
	geo << EditorState::dropletRoom->width << "*" << EditorState::dropletRoom->height << "|-1|0\n";
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
	geo << "Border: Passable\n";
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
	project << "[#timeLimit: 4800, #defaultTerrain: 0, #maxFlies: 10, #flySpawnRate: 50, #lizards: [], #ambientSounds: [], #music: \"NONE\", #tags: [], #lightType: \"Static\", #waterDrips: 1, #lightRect: rect(0, 0, 1040, 800), #Matrix: []]\n";
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
	project << "[#waterLevel: -1, #waterInFront: 1, #waveLength: 60, #waveAmplitude: 5, #waveSpeed: 10]\n";
	project << "[#props: [], #lastKeys: [#w: 0, #a: 0, #s: 0, #d: 0, #L: 0, #n: 0, #m1: 0, #m2: 0, #c: 0, #z: 0], #Keys: [#w: 0, #a: 0, #s: 0, #d: 0, #L: 0, #n: 0, #m1: 0, #m2: 0, #c: 0, #z: 0], #workLayer: 1, #lstMsPs: point(0, 0), #pmPos: point(1, 1), #pmSavPosL: [1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1], #propRotation: 0, #propStretchX: 1, #propStretchY: 1, #propFlipX: 1, #propFlipY: 1, #depth: 0, #color: 0]\n";
	project.close();
}