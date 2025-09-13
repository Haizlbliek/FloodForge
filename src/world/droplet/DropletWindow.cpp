#include "DropletWindow.hpp"

#include "../../Utils.hpp"
#include "../../ui/UI.hpp"
#include "../Globals.hpp"
#include "../../popup/Popups.hpp"

#include "LevelUtils.hpp"

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

Rect DropletWindow::roomRect;
Vector2i DropletWindow::mouseTile;
Vector2i DropletWindow::lastMouseTile;
bool DropletWindow::lastMouseDrawing;
bool DropletWindow::blockMouse = false;

DropletWindow::EditorTab DropletWindow::currentTab;

std::string DropletWindow::TAB_NAMES[4] = { "Environment", "Geometry", "Cameras", "Generator" };
std::string DropletWindow::GEOMETRY_TOOL_NAMES[16] = { "Wall", "Slope", "Platform", "Background Wall", "Horizontal Pole", "Vertical Pole", "Spear", "Rock", "Shortcut", "Room Exit", "Creature Den", "Wack a Mole Hole", "Scavenger Den", "Garbage Worm Den", "Wormgrass", "Batfly Hive" };

DropletWindow::GeometryTool DropletWindow::selectedTool = DropletWindow::GeometryTool::WALL;

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
	cameraScaleTo = std::clamp(cameraScaleTo, 2.5, 40.0);
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
		dir += (EditorState::dropletRoom->getTile(x - 1, y) == 0) ? 1 : 0;
		dir += (EditorState::dropletRoom->getTile(x + 1, y) == 0) ? 2 : 0;
		dir += (EditorState::dropletRoom->getTile(x, y - 1) == 0) ? 4 : 0;
		dir += (EditorState::dropletRoom->getTile(x, y + 1) == 0) ? 8 : 0;
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
		EditorState::dropletRoom->geometry[x * EditorState::dropletRoom->height + y] = (EditorState::dropletRoom->geometry[x * EditorState::dropletRoom->height + y] & ~15) | 1;
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
				EditorState::dropletRoom->geometry[x * EditorState::dropletRoom->height + y] = (EditorState::dropletRoom->geometry[x * EditorState::dropletRoom->height + y] & ~15) | 1;
	
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
}

void DropletWindow::UpdateGeometry() {
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
		}
	}
	Draw::useTexture(0);
	glDisable(GL_BLEND);

	setThemeColor(ThemeColour::RoomBorder);
	strokeRect(roomRect);

	mouseTile = {
		int(std::floor(UI::mouse.x * cameraScale + cameraOffset.x)),
		-int(std::ceil(UI::mouse.y * cameraScale + cameraOffset.y))
	};

	blockMouse = UI::mouse.y >= (EditorState::screenBounds.y - 0.12) || UI::mouse.x >= (EditorState::screenBounds.x - 0.41);

	if (currentTab == DropletWindow::EditorTab::GEOMETRY) UpdateGeometry();

	lastMouseTile = mouseTile;

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

void DropletWindow::exportGeometry() {
	std::ofstream geo(EditorState::region.roomsDirectory / (EditorState::dropletRoom->roomName + ".txt"));
	geo << EditorState::dropletRoom->roomName << "\n";
	geo << EditorState::dropletRoom->width << "*" << EditorState::dropletRoom->height << "|-1|0\n";
	geo << "0.0000*1.0000|0|0\n";
	geo << "-220,-50\n";
	geo << "Border: Passable\n";
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
	geo.close();
}

void DropletWindow::render() {
	exportGeometry();
	// TODO
}