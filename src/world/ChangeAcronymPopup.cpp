#include "ChangeAcronymPopup.hpp"

ChangeAcronymPopup::ChangeAcronymPopup(Window *window) : AcronymPopup(window) {
}

void ChangeAcronymPopup::accept() {
	if (text.length() < 2) return;

	close();
	if (EditorState::offscreenDen != nullptr) {
		EditorState::rooms.erase(std::remove(EditorState::rooms.begin(), EditorState::rooms.end(), EditorState::offscreenDen), EditorState::rooms.end());
		OffscreenRoom *newOffscreenDen = new OffscreenRoom("offscreenden" + toLower(text), "OffscreenDen" + text);
		newOffscreenDen->position = EditorState::offscreenDen->position;
		newOffscreenDen->layer = EditorState::offscreenDen->layer;
		newOffscreenDen->data.hidden = EditorState::offscreenDen->data.hidden;

		for (const Den &oldDen : EditorState::offscreenDen->Dens()) {
			Den &newDen = newOffscreenDen->CreatureDen01(newOffscreenDen->AddDen());
			newDen.count = oldDen.count;
			newDen.data = oldDen.data;
			newDen.tag = oldDen.tag;
			newDen.type = oldDen.type;
		}

		EditorState::rooms.push_back(newOffscreenDen);
		delete EditorState::offscreenDen;
		EditorState::offscreenDen = newOffscreenDen;
	}

	for (Room *room : EditorState::rooms) {
		if (room == EditorState::offscreenDen) continue;

		room->roomName = toLower(text) + room->roomName.substr(room->roomName.find('_'));
	}

	EditorState::region.acronym = toLower(text);
}