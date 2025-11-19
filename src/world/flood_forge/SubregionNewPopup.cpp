#include "SubregionNewPopup.hpp"

#include "../MenuItems.hpp"

SubregionNewPopup::SubregionNewPopup(std::set<Room*> rooms, int editIndex) : AcronymPopup(), rooms(rooms), editIndex(editIndex) {
	bounds = Rect(-0.4, -0.08, 0.4, 0.25);
	setTo = editIndex == -1 ? "" : EditorState::subregions[editIndex];
	needsSet = true;
}

void SubregionNewPopup::submit(std::string acronym) {
	if (acronym.empty()) return;

	close();
	if (editIndex == -1) {
		if (std::find(EditorState::subregions.begin(), EditorState::subregions.end(), acronym) != EditorState::subregions.end()) {
			return;
		}

		EditorState::subregions.push_back(acronym);
		for (Room *room : rooms) {
			room->subregion = std::distance(EditorState::subregions.begin(), std::find(EditorState::subregions.begin(), EditorState::subregions.end(), acronym));
		}
	} else {
		if (std::find(EditorState::subregions.begin(), EditorState::subregions.end(), acronym) != EditorState::subregions.end()) {
			return;
		}

		EditorState::subregions[editIndex] = acronym;
	}
}