#include "ChangeAcronymPopup.hpp"

ChangeAcronymPopup::ChangeAcronymPopup() : AcronymPopup() {
}

void ChangeAcronymPopup::submit(std::string acronym) {
	if (acronym.length() < 2) return;

	close();
	if (EditorState::offscreenDen != nullptr) {
		EditorState::rooms.erase(std::remove(EditorState::rooms.begin(), EditorState::rooms.end(), EditorState::offscreenDen), EditorState::rooms.end());
		OffscreenRoom *newOffscreenDen = new OffscreenRoom("offscreenden" + toLower(acronym), "OffscreenDen" + toUpper(acronym));
		newOffscreenDen->canonPosition = EditorState::offscreenDen->canonPosition;
		newOffscreenDen->devPosition = EditorState::offscreenDen->devPosition;
		newOffscreenDen->layer = EditorState::offscreenDen->layer;
		newOffscreenDen->data.hidden = EditorState::offscreenDen->data.hidden;

		{
			Den &oldDen = EditorState::offscreenDen->getDen();
	
			Den &newDen = newOffscreenDen->getDen();
			newDen.creatures.push_back(DenLineage(oldDen.creatures[0].type, oldDen.creatures[0].count, oldDen.creatures[0].tag, oldDen.creatures[0].data));
			DenCreature *creature = &newDen.creatures[0];
			const DenCreature *oldCreature = &oldDen.creatures[0];
			while (oldCreature->lineageTo != nullptr) {
				creature->lineageTo = new DenCreature(oldCreature->lineageTo->type, oldCreature->lineageTo->count, oldCreature->lineageTo->tag, oldCreature->lineageTo->data);
				creature = creature->lineageTo;
				oldCreature = oldCreature->lineageTo;
			}
		}

		EditorState::rooms.push_back(newOffscreenDen);
		delete EditorState::offscreenDen;
		EditorState::offscreenDen = newOffscreenDen;
	}

	for (Room *room : EditorState::rooms) {
		if (room == EditorState::offscreenDen) continue;

		room->roomName = toLower(acronym) + room->roomName.substr(room->roomName.find('_'));
	}

	EditorState::region.acronym = toLower(acronym);
}