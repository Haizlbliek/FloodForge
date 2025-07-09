#include "Region.hpp"

#include "Globals.hpp"

void Region::reset() {
	if (std::find(EditorState::rooms.begin(), EditorState::rooms.end(), EditorState::offscreenDen) != EditorState::rooms.end()) {
		EditorState::offscreenDen = nullptr;
	}
	
	for (Room *room : EditorState::rooms) {
		delete room;
	}
	EditorState::rooms.clear();
	for (Connection *connection : EditorState::connections) delete connection;
	EditorState::connections.clear();
	EditorState::subregions.clear();
	if (EditorState::offscreenDen != nullptr) delete EditorState::offscreenDen;
	EditorState::offscreenDen = nullptr;
	extraProperties = "";
	extraWorld = "";
	exportDirectory = "";
	acronym = "";
	EditorState::selectedRooms.clear();
	EditorState::roomPossibleSelect = nullptr;
	EditorState::selectingState = 0;
}