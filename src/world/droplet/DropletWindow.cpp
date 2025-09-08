#include "DropletWindow.hpp"

#include "../../Utils.hpp"
#include "../../math/Vector.hpp"
#include "../../ui/UI.hpp"
#include "../Globals.hpp"

void DropletWindow::Draw() {
	Room *room = EditorState::dropletRoom;

	double cameraScale = 40.0;
	Vector2 cameraOffset = Vector2(0.0, 0.0);//Vector2(-room->width * 0.5, room->height * 0.5);
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

	Vector2i mouseTile = {
		int(UI::mouse.x * cameraScale + cameraOffset.x),
		-int(UI::mouse.y * cameraScale + cameraOffset.y)
	};
	if (mouseTile.x >= 0 && mouseTile.x < room->width && mouseTile.y >= 0 && mouseTile.y < room->height) {
		setThemeColor(ThemeColour::RoomBorderHighlight);
		strokeRect(Rect::fromSize(rect.x0 + mouseTile.x, rect.y1 - mouseTile.y - 1, 1.0, 1.0));

		if (UI::mouse.leftMouse) {
			room->geometry[mouseTile.x * room->height + mouseTile.y] = 1;
		}

		if (UI::mouse.rightMouse) {
			room->geometry[mouseTile.x * room->height + mouseTile.y] = 0;
		}
	}
}