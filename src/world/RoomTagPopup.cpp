#include "RoomTagPopup.hpp"

#include "Globals.hpp"

RoomTagPopup::RoomTagPopup(Window *window, std::set<Room*> newRooms) : Popup(window) {
	for (Room *room : newRooms) rooms.insert(room);
}

void RoomTagPopup::draw(double mouseX, double mouseY, bool mouseInside, Vector2 screenBounds) {
	Popup::draw(mouseX, mouseY, mouseInside, screenBounds);
	
	if (minimized) return;

	mouseX -= bounds.X0() + 0.5;
	mouseY -= bounds.Y0() + 0.5;

	Draw::pushMatrix();

	Draw::translate(bounds.X0() + 0.5, bounds.Y0() + 0.5);

	if (rooms.size() > 0) {
		setThemeColour(ThemeColour::Text);
		if (rooms.size() == 1) {
			Fonts::rainworld->writeCentred((*rooms.begin())->roomName, 0.0, 0.4, 0.04, CENTRE_XY);
		} else {
			Fonts::rainworld->writeCentred("Selected Rooms", 0.0, 0.4, 0.04, CENTRE_XY);
		}

		double y = bounds.Y1() - 0.15;
		drawTagButton("None", "", y, mouseX, mouseY);
		y -= 0.075;

		for (int i = 0; i < 9; i++) {
			drawTagButton(ROOM_TAG_NAMES[i], ROOM_TAGS[i], y, mouseX, mouseY);

			y -= 0.075;
		}
	}

	Draw::popMatrix();
}

void RoomTagPopup::setTag(std::string tag) {
	for (Room *room : rooms) {
		if (room->isOffscreen()) continue;

		room->SetTag(tag);
	}
}

void RoomTagPopup::toggleTag(std::string tag) {
	for (Room *room : rooms) {
		if (room->isOffscreen()) continue;

		room->ToggleTag(tag);
	}
}

void RoomTagPopup::mouseClick(double mouseX, double mouseY) {
	Popup::mouseClick(mouseX, mouseY);

	mouseX -= bounds.X0() + 0.5;
	mouseY -= bounds.Y0() + 0.5;

	int button = getButtonIndex(mouseX, mouseY);

	if (button == -1) return;

	if (window->modifierPressed(GLFW_MOD_SHIFT)) {
		if (button != 0) toggleTag(ROOM_TAGS[button - 1]);
	} else {
		if (button == 0) {
			setTag("");
		} else {
			setTag(ROOM_TAGS[button - 1]);
		}
	}
}

int RoomTagPopup::getButtonIndex(double mouseX, double mouseY) {
	if (mouseX < -0.4 || mouseX > 0.4) return -1;
	if (mouseY > 0.35) return -1;
	if (std::fmod(-mouseY + 0.35, 0.075) > 0.05) return -1;

	return floor((-mouseY + 0.35) / 0.075);
}

void RoomTagPopup::drawTagButton(std::string tag, std::string tagId, double y, double mouseX, double mouseY) {
	setThemeColour(ThemeColour::Button);
	fillRect(-0.4, y, 0.4, y - 0.05);

	if (rooms.size() == 1) {
		const std::vector<std::string> tags = (*rooms.begin())->Tags();

		if ((tagId == "" && tags.size() == 0) || std::find(tags.begin(), tags.end(), tagId) != tags.end()) {
			setThemeColour(ThemeColour::BorderHighlight);
		} else {
			setThemeColour(ThemeColour::Text);
		}
	} else {
		setThemeColour(ThemeColour::Text);
	}
	Fonts::rainworld->writeCentred(tag, 0, y - 0.02, 0.04, CENTRE_XY);

	if (Rect(-0.4, y, 0.4, y - 0.05).inside(mouseX, mouseY)) {
		setThemeColour(ThemeColour::BorderHighlight);
		strokeRect(-0.4, y, 0.4, y - 0.05);
	} else {
		setThemeColour(ThemeColour::Border);
		strokeRect(-0.4, y, 0.4, y - 0.05);
	}
}