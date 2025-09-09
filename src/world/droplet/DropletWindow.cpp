#include "DropletWindow.hpp"

#include "../../Utils.hpp"
#include "../../ui/UI.hpp"
#include "../Globals.hpp"
#include "../../popup/Popups.hpp"

#include "LevelUtils.hpp"

Vector2 DropletWindow::cameraOffset;
double DropletWindow::cameraScale = 40.0;
double DropletWindow::cameraScaleTo = 40.0;

bool DropletWindow::cameraPanning = false;
bool DropletWindow::cameraPanningBlocked = false;
Vector2 DropletWindow::cameraPanStartMouse = Vector2(0.0f, 0.0f);
Vector2 DropletWindow::cameraPanStart = Vector2(0.0f, 0.0f);
Vector2 DropletWindow::cameraPanTo = Vector2(0.0f, 0.0f);

Vector2i DropletWindow::mouseTile;
Vector2i DropletWindow::lastMouseTile;
bool DropletWindow::lastMouseDrawing;

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

void DropletWindow::Draw() {
	UpdateCamera();

	Room *room = EditorState::dropletRoom;

	applyFrustumToOrthographic(cameraOffset, 0.0f, cameraScale * EditorState::screenBounds);

	Rect rect = Rect::fromSize(0.0, 0.0, room->width, -room->height);
	setThemeColor(ThemeColour::RoomSolid);
	fillRect(rect);
	int id = 0;
	setThemeColor(ThemeColour::RoomAir);
	Draw::begin(Draw::QUADS);
	for (int x = 0; x < room->width; x++) {
		for (int y = 0; y < room->height; y++) {
			float x0 = rect.x0 + x;
			float y0 = rect.y1 - y;
			float x1 = rect.x0 + x + 1;
			float y1 = rect.y1 - y - 1;

			if (room->geometry[id] % 16 == 0) {
				Draw::vertex(x0, y0);
				Draw::vertex(x1, y0);
				Draw::vertex(x1, y1);
				Draw::vertex(x0, y1);
			} else if (room->geometry[id] % 16 == 2) {
				int bits = 0;
				bits += (room->getTile(x - 1, y) == 1) ? 1 : 0;
				bits += (room->getTile(x + 1, y) == 1) ? 2 : 0;
				bits += (room->getTile(x, y - 1) == 1) ? 4 : 0;
				bits += (room->getTile(x, y + 1) == 1) ? 8 : 0;

				if (bits == 1 + 4) {
					Draw::vertex(x0, y1);
					Draw::vertex(x1, y0);
					Draw::vertex(x1, y1);
					Draw::vertex(x0, y1);
				} else if (bits == 1 + 8) {
					Draw::vertex(x0, y0);
					Draw::vertex(x1, y0);
					Draw::vertex(x1, y1);
					Draw::vertex(x0, y0);
				} else if (bits == 2 + 4) {
					Draw::vertex(x0, y0);
					Draw::vertex(x1, y1);
					Draw::vertex(x1, y1);
					Draw::vertex(x0, y1);
				} else if (bits == 2 + 8) {
					Draw::vertex(x0, y1);
					Draw::vertex(x1, y0);
					Draw::vertex(x0, y0);
					Draw::vertex(x0, y1);
				} else {
					Draw::vertex(x0, y0);
					Draw::vertex(x1, y0);
					Draw::vertex(x0, y1);
					Draw::vertex(x1, y1);
				}
			}

			id++;
		}
	}
	Draw::end();
	setThemeColor(ThemeColour::RoomBorder);
	strokeRect(rect);

	mouseTile = {
		int(UI::mouse.x * cameraScale + cameraOffset.x),
		-int(UI::mouse.y * cameraScale + cameraOffset.y)
	};

	if ((UI::mouse.leftMouse || UI::mouse.rightMouse) && (lastMouseTile.x != mouseTile.x || lastMouseTile.y != mouseTile.y)) {
		std::vector<Vector2i> drawLine;
		drawLine = LevelUtils::line(lastMouseTile.x, lastMouseTile.y, mouseTile.x, mouseTile.y);

		for (Vector2i point : drawLine) {
			if (room->InBounds(point.x, point.y)) {
				if (UI::mouse.leftMouse) {
					room->geometry[point.x * room->height + point.y] = 1;
				}
		
				if (UI::mouse.rightMouse) {
					room->geometry[point.x * room->height + point.y] = 0;
				}
			}
		}
	} else if ((UI::mouse.leftMouse && !UI::mouse.lastLeftMouse) || (UI::mouse.rightMouse && !UI::mouse.lastRightMouse)) {
		if (room->InBounds(mouseTile.x, mouseTile.y)) {
			if (UI::mouse.leftMouse) {
				room->geometry[mouseTile.x * room->height + mouseTile.y] = 1;
			}
	
			if (UI::mouse.rightMouse) {
				room->geometry[mouseTile.x * room->height + mouseTile.y] = 0;
			}
		}
	}

	setThemeColor(ThemeColour::RoomBorderHighlight);
	strokeRect(Rect::fromSize(rect.x0 + mouseTile.x, rect.y1 - mouseTile.y - 1, 1.0, 1.0));

	lastMouseTile = mouseTile;
	lastMouseDrawing = UI::mouse.leftMouse || UI::mouse.rightMouse;
}