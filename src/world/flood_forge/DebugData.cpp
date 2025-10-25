#include "DebugData.hpp"
#include "UndoRedo.hpp"

namespace DebugData {
	void draw(Window *window, Vector2 mouse, Vector2 screenBounds) {
		Connection *hoveringConnection = nullptr;
		Room *hoveringRoom = nullptr;

		for (auto it = EditorState::connections.rbegin(); it != EditorState::connections.rend(); it++) {
			Connection *connection = *it;
			if (!EditorState::visibleLayers[connection->roomA->layer]) continue;
			if (!EditorState::visibleLayers[connection->roomB->layer]) continue;

			if (connection->hovered(mouse, EditorState::lineSize)) {
				hoveringConnection = connection;

				break;
			}
		}

		for (auto it = EditorState::rooms.rbegin(); it != EditorState::rooms.rend(); it++) {
			Room *room = *it;
			if (!EditorState::visibleLayers[room->layer]) continue;

			if (room->inside(mouse)) {
				hoveringRoom = room;
				break;
			}
		}

		std::vector<std::string> debugText;

		// Información del sistema Undo/Redo
		debugText.push_back("    Undo/Redo:");
		debugText.push_back("Undo Stack: " + std::to_string(UndoRedo::getUndoStackSize()));
		debugText.push_back("Redo Stack: " + std::to_string(UndoRedo::getRedoStackSize()));
		debugText.push_back("Last Action: " + UndoRedo::getLastAction());
		debugText.push_back("");

		debugText.push_back("    Count:");
		debugText.push_back("Rooms: " + std::to_string(EditorState::rooms.size()));
		debugText.push_back("Screens: " + std::to_string(EditorState::screenCount));

		if (hoveringConnection != nullptr) {
			debugText.push_back("    Connection:");
			debugText.push_back("Room A: " + hoveringConnection->roomA->roomName);
			debugText.push_back("Room B: " + hoveringConnection->roomB->roomName);
			debugText.push_back("Connection A: " + std::to_string(hoveringConnection->connectionA));
			debugText.push_back("Connection B: " + std::to_string(hoveringConnection->connectionB));
		}

		if (hoveringRoom != nullptr) {
			std::string tags = "";
			for (std::string tag : hoveringRoom->Tags()) tags += " " + tag;
			debugText.push_back("    Room:");
			if (!hoveringRoom->valid) {
				debugText.push_back("INVALID - Check 'xx-rooms' folder");
				debugText.push_back("Name: " + hoveringRoom->roomName);
			} else {
				debugText.push_back("Name: " + hoveringRoom->roomName);
				debugText.push_back("Tags:" + tags);
				debugText.push_back("Width: " + std::to_string(hoveringRoom->Width()));
				debugText.push_back("Height: " + std::to_string(hoveringRoom->Height()));
				debugText.push_back("Dens: " + std::to_string(hoveringRoom->DenCount()));
				debugText.push_back(hoveringRoom->data.merge ? "Merge: true" : "Merge: false");
				debugText.push_back("Dev Items: " + std::to_string(hoveringRoom->data.devItems.size()));
				if (hoveringRoom->data.hidden) {
					debugText.push_back("Layer: Hidden - " + std::to_string(hoveringRoom->layer));
				} else {
					debugText.push_back("Layer: " + std::to_string(hoveringRoom->layer));
				}
				if (hoveringRoom->subregion == -1) {
					debugText.push_back("Subregion: <<NONE>>");
				} else {
					debugText.push_back("Subregion: " + EditorState::subregions[hoveringRoom->subregion]);
				}
			}
		}
		
		for (auto it = EditorState::rooms.rbegin(); it != EditorState::rooms.rend(); it++) {
			Room *room = *it;
			
			Vector2 roomMouse = mouse - room->currentPosition();
			Vector2 shortcutPosition;
			
			if (room->isOffscreen()) {
				shortcutPosition = Vector2(room->Width() * 0.5 + 0.5, -room->Height() * 0.25 - 0.5);

				if (roomMouse.distanceTo(shortcutPosition) < EditorState::selectorScale) {
					debugText.push_back("   Shortcut - Offscreen:");
					debugText.push_back("Den:");
					EditorState::offscreenDen->getDen();
					for (DenLineage &lineage : room->CreatureDen01(0).creatures) {
						DenCreature *creature = &lineage;
						std::string line = "";
						line += creature->type + " x " + std::to_string(creature->count);
						while (creature->lineageTo != nullptr) {
							creature = creature->lineageTo;
							line += " --" + std::to_string(int(creature->lineageChance * 100)) + "%-> ";
							line += creature->type + " x " + std::to_string(creature->count);
						}
						debugText.push_back(line);
					}
				}
			} else {
				for (Vector2i shortcut : room->DenEntrances()) {
					shortcutPosition = Vector2(shortcut.x + 0.5, -1 - shortcut.y + 0.5);

					if (roomMouse.distanceTo(shortcutPosition) < EditorState::selectorScale) {
						debugText.push_back("   Shortcut:");
						debugText.push_back("Den: " + std::to_string(shortcut.x) + ", " + std::to_string(shortcut.y));
						for (DenLineage &lineage : room->CreatureDen(room->DenId(shortcut)).creatures) {
							DenCreature *creature = &lineage;
							std::string line = "";
							line += creature->type + " x " + std::to_string(creature->count);
							while (creature->lineageTo != nullptr) {
								creature = creature->lineageTo;
								line += " --" + std::to_string(int(creature->lineageChance * 100)) + "%-> ";
								line += creature->type + " x " + std::to_string(creature->count);
							}
							debugText.push_back(line);
						}
					}
				}
			}
		}

		int i = 1;

		Draw::color(0.0f, 0.0f, 0.0f, 1.0f);
		for (auto it = debugText.rbegin(); it != debugText.rend(); it++) {
			std::string line = *it;

			Fonts::rainworld->write(line, -screenBounds.x, -screenBounds.y + i*0.04 - 0.003, 0.03);
			Fonts::rainworld->write(line, -screenBounds.x, -screenBounds.y + i*0.04 + 0.003, 0.03);
			Fonts::rainworld->write(line, -screenBounds.x - 0.003, -screenBounds.y + i*0.04, 0.03);
			Fonts::rainworld->write(line, -screenBounds.x + 0.003, -screenBounds.y + i*0.04, 0.03);
			i++;
		}
		
		i = 1;

		setThemeColour(ThemeColour::Text);
		for (auto it = debugText.rbegin(); it != debugText.rend(); it++) {
			std::string line = *it;

			Fonts::rainworld->write(line, -screenBounds.x, -screenBounds.y + i*0.04, 0.03);
			i++;
		}
	}
}