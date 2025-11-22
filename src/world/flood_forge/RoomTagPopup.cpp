#include "RoomTagPopup.hpp"

#include "../Globals.hpp"
#include "../../ui/UI.hpp"

#include "FloodForgeWindow.hpp"

RoomTagPopup::RoomTagPopup(std::set<Room*> newRooms) : Popup() {
	for (Room *room : newRooms) {
		rooms.insert(room);
	}
}

void RoomTagPopup::draw() {
	Popup::draw();
	
	if (minimized) return;

	if (rooms.size() > 0) {
		setThemeColor(ThemeColor::Text);
		if (rooms.size() == 1) {
			Fonts::rainworld->writeCentered((*rooms.begin())->roomName, 0.0, 0.4, 0.04, CENTER_XY);
		} else {
			Fonts::rainworld->writeCentered("Selected Rooms", 0.0, 0.4, 0.04, CENTER_XY);
		}

		double y = bounds.y1 - 0.15;
		drawTagButton("None", "", y);
		y -= 0.075;

		for (int i = 0; i < ROOM_TAG_COUNT; i++) {
			drawTagButton(ROOM_TAG_NAMES[i], ROOM_TAGS[i], y);

			y -= 0.075;
		}
	}
}

void RoomTagPopup::setTag(std::string tag) {
	TagChange *change = new TagChange();
	std::vector<std::string> to;
	if (!tag.empty()) to.push_back(tag);

	for (Room *room : rooms) {
		if (room->isOffscreen()) continue;

		change->addRoom(room, to);
		std::vector<std::string> n;
		room->tags = n;
	}

	FloodForgeWindow::history.change(change);
}

void RoomTagPopup::toggleTag(std::string tag) {
	TagChange *change = new TagChange();

	for (Room *room : rooms) {
		if (room->isOffscreen()) continue;

		std::vector<std::string> to;
		bool add = true;
		for (std::string &otherTag : room->tags) {
			if (otherTag == tag) {
				add = false;
				continue;
			}

			to.push_back(otherTag);
		}
		if (add) {
			to.push_back(tag);
		}

		change->addRoom(room, to);

		std::vector<std::string> n;
		room->tags = n;
	}

	FloodForgeWindow::history.change(change);
}

void RoomTagPopup::drawTagButton(std::string tag, std::string tagId, double y) {
	Rect rect(-0.4, y, 0.4, y - 0.05);
	bool selected = false;
	if (rooms.size() == 1) {
		const std::vector<std::string> &tags = (*rooms.begin())->tags;
		selected = (tagId == "" && tags.size() == 0) || (std::find(tags.begin(), tags.end(), tagId) != tags.end());
	}

	if (UI::TextButton(rect, tag, UI::TextButtonMods().Selected(selected))) {
		if (UI::window->modifierPressed(GLFW_MOD_SHIFT)) {
			if (!tagId.empty()) toggleTag(tagId);
		} else {
			setTag(tagId);
		}
	}
}