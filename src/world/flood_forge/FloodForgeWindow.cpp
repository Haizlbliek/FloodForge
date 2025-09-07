#include "FloodForgeWindow.hpp"

#include <exception>
#include "../../gl.h"
#include "../../Logger.hpp"
#include "../Globals.hpp"
#include "../../ui/UI.hpp"
#include "../DebugData.hpp"

#include "../../popup/MarkdownPopup.hpp"
#include "../../popup/ConfirmPopup.hpp"
#include "../popup/SplashArtPopup.hpp"
#include "../popup/SubregionPopup.hpp"
#include "../popup/RoomTagPopup.hpp"
#include "../popup/DenPopup.hpp"
#include "../popup/RoomAttractivenessPopup.hpp"
#include "../popup/ConditionalPopup.hpp"
#include "../popup/CreateRoomPopup.hpp"

Vector2 FloodForgeWindow::worldMouse;

bool FloodForgeWindow::cameraPanning = false;
bool FloodForgeWindow::cameraPanningBlocked = false;
Vector2 FloodForgeWindow::cameraPanStartMouse = Vector2(0.0f, 0.0f);
Vector2 FloodForgeWindow::cameraPanStart = Vector2(0.0f, 0.0f);
Vector2 FloodForgeWindow::cameraPanTo = Vector2(0.0f, 0.0f);
double FloodForgeWindow::cameraScaleTo = EditorState::cameraScale;

Room *FloodForgeWindow::holdingRoom = nullptr;
Vector2 FloodForgeWindow::holdingStart = Vector2(0.0f, 0.0f);
int FloodForgeWindow::holdingType = 0;

Vector2 FloodForgeWindow::selectionStart;
Vector2 FloodForgeWindow::selectionEnd;
int FloodForgeWindow::roomSnap = ROOM_SNAP_TILE;

Vector2 *FloodForgeWindow::connectionStart = nullptr;
Vector2 *FloodForgeWindow::connectionEnd = nullptr;
Connection *FloodForgeWindow::currentConnection = nullptr;
bool FloodForgeWindow::currentConnectionValid = false;
std::string FloodForgeWindow::connectionError = "";
FloodForgeWindow::ConnectionState FloodForgeWindow::connectionState = FloodForgeWindow::ConnectionState::None;



void FloodForgeWindow::updateCamera() {
	bool isHoveringPopup = false;
	for (Popup *popup : Popups::popups) {
		Rect bounds = popup->Bounds();

		if (bounds.inside(UI::mouse)) {
			isHoveringPopup = true;
			break;
		}
	}

	/// Update Camera

	//// Zooming
	double scrollY = -EditorState::window->getMouseScrollY();
	if (isHoveringPopup) scrollY = 0.0;

	if (scrollY < -10.0) scrollY = -10.0;
	double zoom = std::pow(1.25, scrollY);

	Vector2 previousWorldMouse = Vector2(
		UI::mouse.x * EditorState::cameraScale + EditorState::cameraOffset.x,
		UI::mouse.y * EditorState::cameraScale + EditorState::cameraOffset.y
	);

	cameraScaleTo *= zoom;
	EditorState::cameraScale += (cameraScaleTo - EditorState::cameraScale) * Settings::getSetting<double>(Settings::Setting::CameraZoomSpeed);

	worldMouse = Vector2(
		UI::mouse.x * EditorState::cameraScale + EditorState::cameraOffset.x,
		UI::mouse.y * EditorState::cameraScale + EditorState::cameraOffset.y
	);

	EditorState::cameraOffset.x += previousWorldMouse.x - worldMouse.x;
	EditorState::cameraOffset.y += previousWorldMouse.y - worldMouse.y;
	cameraPanTo.x += previousWorldMouse.x - worldMouse.x;
	cameraPanTo.y += previousWorldMouse.y - worldMouse.y;

	//// Panning
	if (EditorState::mouse->Middle()) {
		if (!cameraPanningBlocked && !cameraPanning) {
			if (isHoveringPopup) cameraPanningBlocked = true;

			if (!cameraPanningBlocked) {
				cameraPanStart.x = EditorState::cameraOffset.x;
				cameraPanStart.y = EditorState::cameraOffset.y;
				cameraPanStartMouse.x = EditorState::globalMouse.x;
				cameraPanStartMouse.y = EditorState::globalMouse.y;
				cameraPanning = true;
			}
		}

		if (cameraPanning && !cameraPanningBlocked) {
			cameraPanTo.x = cameraPanStart.x + EditorState::cameraScale * (cameraPanStartMouse.x - EditorState::globalMouse.x) / 512.0;
			cameraPanTo.y = cameraPanStart.y + EditorState::cameraScale * (cameraPanStartMouse.y - EditorState::globalMouse.y) / -512.0;
		}
	} else {
		cameraPanning = false;
		cameraPanningBlocked = false;
	}

	EditorState::cameraOffset.x += (cameraPanTo.x - EditorState::cameraOffset.x) * Settings::getSetting<double>(Settings::Setting::CameraPanSpeed);
	EditorState::cameraOffset.y += (cameraPanTo.y - EditorState::cameraOffset.y) * Settings::getSetting<double>(Settings::Setting::CameraPanSpeed);
}

void FloodForgeWindow::updateOriginalControls() {
	if (EditorState::mouse->Left()) {
		if (EditorState::mouse->JustLeft()) {
			if (EditorState::selectingState == 0) {
				for (auto it = EditorState::rooms.rbegin(); it != EditorState::rooms.rend(); it++) {
					Room *room = *it;
					if (!EditorState::visibleLayers[room->layer]) continue;

					if (room->inside(worldMouse)) {
						holdingRoom = room;
						holdingStart = worldMouse;
						EditorState::roomPossibleSelect = room;
						EditorState::selectingState = 3;
						break;
					}
				}
			}

			if (EditorState::selectingState == 0) {
				if (EditorState::window->modifierPressed(GLFW_MOD_SHIFT)) {
					EditorState::selectingState = 1;
					selectionStart = worldMouse;
					selectionEnd = worldMouse;
					if (!EditorState::window->modifierPressed(GLFW_MOD_CONTROL)) EditorState::selectedRooms.clear();
				} else {
					EditorState::selectingState = 5;
					selectionStart = EditorState::globalMouse;
					selectionEnd = EditorState::globalMouse;
				}
			}
		} else {
			if (EditorState::selectingState == 3 && EditorState::mouse->Moved() || EditorState::selectingState == 4) {
				if (EditorState::selectingState == 3) {
					if (EditorState::window->modifierPressed(GLFW_MOD_SHIFT) || EditorState::window->modifierPressed(GLFW_MOD_CONTROL)) {
						EditorState::selectedRooms.insert(EditorState::roomPossibleSelect);
					} else {
						if (EditorState::selectedRooms.find(holdingRoom) == EditorState::selectedRooms.end()) {
							EditorState::selectedRooms.clear();
							EditorState::selectedRooms.insert(EditorState::roomPossibleSelect);
						}
					}
					EditorState::rooms.erase(std::remove(EditorState::rooms.begin(), EditorState::rooms.end(), EditorState::roomPossibleSelect), EditorState::rooms.end());
					EditorState::rooms.push_back(EditorState::roomPossibleSelect);
					EditorState::selectingState = 4;
				}

				Vector2 offset = (worldMouse - holdingStart);
				if (roomSnap == ROOM_SNAP_TILE) offset.round();

				for (Room *room2 : EditorState::selectedRooms) {
					Vector2 &roomPosition = room2->currentPosition();
					if (roomSnap == ROOM_SNAP_TILE) {
						roomPosition.round();
					}

					roomPosition.add(offset);

					if (EditorState::window->modifierPressed(GLFW_MOD_ALT)) {
						room2->moveBoth();
					}
				}
				holdingStart = holdingStart + offset;
			}

			if (EditorState::selectingState == 1) {
				selectionEnd = worldMouse;
			}

			if (EditorState::selectingState == 5) {
				selectionEnd = EditorState::globalMouse;

				cameraPanTo.x += (selectionStart.x - selectionEnd.x) * EditorState::cameraScale / 512;
				cameraPanTo.y += (selectionStart.y - selectionEnd.y) * EditorState::cameraScale / -512;

				selectionStart = selectionEnd;
			}
		}
	} else {
		if (EditorState::selectingState == 3) {
			EditorState::rooms.erase(std::remove(EditorState::rooms.begin(), EditorState::rooms.end(), EditorState::roomPossibleSelect), EditorState::rooms.end());
			EditorState::rooms.push_back(EditorState::roomPossibleSelect);
			if (EditorState::window->modifierPressed(GLFW_MOD_SHIFT) || EditorState::window->modifierPressed(GLFW_MOD_CONTROL)) {
				if (EditorState::selectedRooms.find(EditorState::roomPossibleSelect) != EditorState::selectedRooms.end()) {
					EditorState::selectedRooms.erase(EditorState::roomPossibleSelect);
				} else {
					EditorState::selectedRooms.insert(EditorState::roomPossibleSelect);
				}
			} else {
				EditorState::selectedRooms.clear();
				EditorState::selectedRooms.insert(EditorState::roomPossibleSelect);
			}
			holdingType = 1;
			if (roomSnap == ROOM_SNAP_TILE) {
				for (Room *room2 : EditorState::selectedRooms) {
					room2->currentPosition().round();
				}
			}
		}

		holdingRoom = nullptr;

		if (EditorState::selectingState == 1) {
			for (Room *room : EditorState::rooms) {
				if (room->intersects(selectionStart, selectionEnd)) EditorState::selectedRooms.insert(room);
			}
		}
		EditorState::selectingState = 0;
	}
}

void FloodForgeWindow::updateFloodForgeControls() {
	if (EditorState::mouse->Left()) {
		if (EditorState::mouse->JustLeft()) {
			if (EditorState::selectingState == 0) {
				for (auto it = EditorState::rooms.rbegin(); it != EditorState::rooms.rend(); it++) {
					Room *room = *it;
					if (!EditorState::visibleLayers[room->layer]) continue;

					if (room->inside(worldMouse)) {
						holdingRoom = room;
						holdingStart = worldMouse;
						EditorState::roomPossibleSelect = room;
						EditorState::selectingState = 3;
						break;
					}
				}
			}

			if (EditorState::selectingState == 0) {
				EditorState::selectingState = 1;
				selectionStart = worldMouse;
				selectionEnd = worldMouse;
				if (!EditorState::window->modifierPressed(GLFW_MOD_SHIFT) && !EditorState::window->modifierPressed(GLFW_MOD_CONTROL)) {
					EditorState::selectedRooms.clear();
				}
			}
		} else {
			if (EditorState::selectingState == 3 && EditorState::mouse->Moved() || EditorState::selectingState == 4) {
				if (EditorState::selectingState == 3) {
					if (EditorState::window->modifierPressed(GLFW_MOD_SHIFT) || EditorState::window->modifierPressed(GLFW_MOD_CONTROL)) {
						EditorState::selectedRooms.insert(EditorState::roomPossibleSelect);
					} else {
						if (EditorState::selectedRooms.find(holdingRoom) == EditorState::selectedRooms.end()) {
							EditorState::selectedRooms.clear();
							EditorState::selectedRooms.insert(EditorState::roomPossibleSelect);
						}
					}
					EditorState::rooms.erase(std::remove(EditorState::rooms.begin(), EditorState::rooms.end(), EditorState::roomPossibleSelect), EditorState::rooms.end());
					EditorState::rooms.push_back(EditorState::roomPossibleSelect);
					EditorState::selectingState = 4;
				}

				Vector2 offset = (worldMouse - holdingStart);
				if (roomSnap == ROOM_SNAP_TILE) offset.round();

				for (Room *room2 : EditorState::selectedRooms) {
					Vector2 &roomPosition = room2->currentPosition();
					if (roomSnap == ROOM_SNAP_TILE) {
						roomPosition.round();
					}

					roomPosition.add(offset);
					if (EditorState::window->modifierPressed(GLFW_MOD_ALT)) {
						room2->moveBoth();
					}
				}
				holdingStart = holdingStart + offset;
			}

			if (EditorState::selectingState == 1) {
				selectionEnd = worldMouse;
				// selectedRooms.clear();
			}
		}
	} else {
		if (EditorState::selectingState == 3) {
			EditorState::rooms.erase(std::remove(EditorState::rooms.begin(), EditorState::rooms.end(), EditorState::roomPossibleSelect), EditorState::rooms.end());
			EditorState::rooms.push_back(EditorState::roomPossibleSelect);
			if (EditorState::window->modifierPressed(GLFW_MOD_SHIFT) || EditorState::window->modifierPressed(GLFW_MOD_CONTROL)) {
				if (EditorState::selectedRooms.find(EditorState::roomPossibleSelect) != EditorState::selectedRooms.end()) {
					EditorState::selectedRooms.erase(EditorState::roomPossibleSelect);
				} else {
					EditorState::selectedRooms.insert(EditorState::roomPossibleSelect);
				}
			} else {
				EditorState::selectedRooms.clear();
				EditorState::selectedRooms.insert(EditorState::roomPossibleSelect);
			}
			holdingType = 1;
			if (roomSnap == ROOM_SNAP_TILE) {
				for (Room *room2 : EditorState::selectedRooms) {
					room2->currentPosition().round();
				}
			}
		}

		holdingRoom = nullptr;

		if (EditorState::selectingState == 1) {
			for (Room *room : EditorState::rooms) {
				if (room->intersects(selectionStart, selectionEnd)) EditorState::selectedRooms.insert(room);
			}
		}
		EditorState::selectingState = 0;
	}
}

void FloodForgeWindow::updateMain() {
	updateCamera();
	UI::update();

	EditorState::selectorScale = Settings::getSetting<bool>(Settings::Setting::SelectorScale) ? EditorState::cameraScale / 16.0 : 1.0;

	/// Update Inputs

	if (EditorState::window->modifierPressed(GLFW_MOD_ALT)) {
		roomSnap = ROOM_SNAP_NONE;
	} else {
		roomSnap = ROOM_SNAP_TILE;
	}

	if (EditorState::window->justPressed(GLFW_KEY_F11)) {
		EditorState::window->toggleFullscreen();
	}

	if (EditorState::window->justPressed(GLFW_KEY_ESCAPE)) {
		if (Popups::popups.size() > 0) {
			Popups::popups[Popups::popups.size() - 1]->reject();
		} else {
			Popups::addPopup((new ConfirmPopup(EditorState::window, "Exit FloodForge?"))->OnOkay([&]() {
				EditorState::window->close();
			}));
		}
	}

	if (EditorState::window->justPressed(GLFW_KEY_ENTER)) {
		if (Popups::popups.size() > 0) {
			Popups::popups[0]->accept();
		}
	}

	if (EditorState::window->modifierPressed(GLFW_MOD_ALT) && EditorState::window->justPressed(GLFW_KEY_T)) {
		Popups::addPopup(new MarkdownPopup(EditorState::window, BASE_PATH / "docs" / "controls.md"));
	}

	//// Connections
	connectionError = "";
	if (EditorState::mouse->Right()) {
		Room *hoveringRoom = nullptr;
		for (auto it = EditorState::rooms.rbegin(); it != EditorState::rooms.rend(); it++) {
			Room *room = (*it);
			if (!EditorState::visibleLayers[room->layer]) continue;

			if (room->inside(worldMouse)) {
				hoveringRoom = room;
				break;
			}
		}

		Vector2i tilePosition;

		if (hoveringRoom != nullptr) {
			Vector2 &roomPosition = hoveringRoom->currentPosition();
			tilePosition = Vector2i(
				floor(worldMouse.x - roomPosition.x),
				-1 - floor(worldMouse.y - roomPosition.y)
			);
		} else {
			tilePosition = Vector2i(-1, -1);
		}

		if (connectionState == ConnectionState::None) {
			if (connectionStart != nullptr) { delete connectionStart; connectionStart = nullptr; }
			if (connectionEnd != nullptr) { delete connectionEnd; connectionEnd = nullptr; }

			if (hoveringRoom != nullptr) {
				Vector2 &roomPosition = hoveringRoom->currentPosition();
				int connectionId = hoveringRoom->getRoomEntranceId(tilePosition);

				if (connectionId != -1) {
					connectionStart = new Vector2(floor(worldMouse.x - roomPosition.x) + 0.5 + roomPosition.x, floor(worldMouse.y - roomPosition.y) + 0.5 + roomPosition.y);
					connectionEnd = new Vector2(connectionStart);
					currentConnection = new Connection(hoveringRoom, connectionId, nullptr, 0);
					currentConnectionValid = false;
				}
			}

			connectionState = (connectionStart == nullptr) ? ConnectionState::NoConnection : ConnectionState::Connection;
		} else if (connectionState == ConnectionState::Connection) {
			int connectionId = -1;

			if (hoveringRoom != nullptr) {
				connectionId = hoveringRoom->getRoomEntranceId(tilePosition);
			}

			if (connectionId != -1) {
				Vector2 &roomPosition = hoveringRoom->currentPosition();
				connectionEnd->x = floor(worldMouse.x - roomPosition.x) + 0.5 + roomPosition.x;
				connectionEnd->y = floor(worldMouse.y - roomPosition.y) + 0.5 + roomPosition.y;
				currentConnection->roomB = hoveringRoom;
				currentConnection->connectionB = connectionId;
				currentConnectionValid = true;

				if (currentConnection->roomA == currentConnection->roomB) {
					connectionError = "Can't connect to same room";
					currentConnectionValid = false;
				} else {
					for (Connection *other : currentConnection->roomB->connections) {
						if (
							(other->roomA == currentConnection->roomB && other->connectionA == currentConnection->connectionB) &&
							(other->roomB == currentConnection->roomA && other->connectionB == currentConnection->connectionA)
						) {
							connectionError = "Can't perform same connection twice";
							currentConnectionValid = false;
							break;
						}
					}
				}
			} else {
				connectionEnd->x = worldMouse.x;
				connectionEnd->y = worldMouse.y;
				currentConnection->roomB = nullptr;
				currentConnection->connectionB = 0;
				currentConnectionValid = false;
				connectionError = "Needs to connect";
			}
		}
	} else {
		if (currentConnection != nullptr) {
			if (currentConnectionValid) {
				EditorState::connections.push_back(currentConnection);
				currentConnection->roomA->connect(currentConnection);
				currentConnection->roomB->connect(currentConnection);
			} else {
				delete currentConnection;
			}

			currentConnection = nullptr;
		}

		if (connectionStart != nullptr) { delete connectionStart; connectionStart = nullptr; }
		if (connectionEnd != nullptr) { delete connectionEnd; connectionEnd = nullptr; }

		connectionState = ConnectionState::None;
	}

	//// Holding
	if (Settings::getSetting<bool>(Settings::Setting::OrignalControls)) {
		updateOriginalControls();
	} else {
		updateFloodForgeControls();
	}

	if (!Popups::popups.empty()) return;

	if (EditorState::window->justPressed(GLFW_KEY_I)) {
		for (auto it = EditorState::rooms.rbegin(); it != EditorState::rooms.rend(); it++) {
			Room *room = *it;
			if (!EditorState::visibleLayers[room->layer]) continue;

			if (room->inside(worldMouse)) {
				EditorState::rooms.erase(std::remove(EditorState::rooms.begin(), EditorState::rooms.end(), room), EditorState::rooms.end());
				EditorState::rooms.insert(EditorState::rooms.begin(), room);
				break;
			}
		}
	}

	if (EditorState::window->justPressed(GLFW_KEY_X)) {
		bool deleted = false;

		for (auto it = EditorState::connections.rbegin(); it != EditorState::connections.rend(); it++) {
			Connection *connection = *it;
			if (!EditorState::visibleLayers[connection->roomA->layer]) continue;
			if (!EditorState::visibleLayers[connection->roomB->layer]) continue;

			if (connection->hovered(worldMouse, EditorState::lineSize)) {
				EditorState::connections.erase(std::remove(EditorState::connections.begin(), EditorState::connections.end(), connection), EditorState::connections.end());

				connection->roomA->disconnect(connection);
				connection->roomB->disconnect(connection);

				delete connection;

				deleted = true;

				break;
			}
		}

		if (!deleted) {
			Room *hoveredRoom = nullptr;
			for (auto it = EditorState::rooms.rbegin(); it != EditorState::rooms.rend(); it++) {
				Room *room = *it;
				if (!EditorState::visibleLayers[room->layer]) continue;

				if (room->inside(worldMouse)) {
					if (room != EditorState::offscreenDen) hoveredRoom = room;
					break;
				}
			}

			if (hoveredRoom != nullptr) {
				if (EditorState::selectedRooms.find(hoveredRoom) != EditorState::selectedRooms.end()) {
					for (Room *room : EditorState::selectedRooms) {
						if (room == EditorState::offscreenDen) continue;

						EditorState::rooms.erase(std::remove(EditorState::rooms.begin(), EditorState::rooms.end(), room), EditorState::rooms.end());

						EditorState::connections.erase(std::remove_if(EditorState::connections.begin(), EditorState::connections.end(),
							[room](Connection *connection) {
								if (connection->roomA == room || connection->roomB == room) {
									connection->roomA->disconnect(connection);
									connection->roomB->disconnect(connection);

									delete connection;
									return true;
								}

								return false;
							}
						), EditorState::connections.end());

						delete room;
					}

					EditorState::selectedRooms.clear();
				} else {
					for (auto it = EditorState::rooms.rbegin(); it != EditorState::rooms.rend(); it++) {
						Room *room = *it;
						if (!EditorState::visibleLayers[room->layer]) continue;

						if (room->inside(worldMouse)) {
							EditorState::rooms.erase(std::remove(EditorState::rooms.begin(), EditorState::rooms.end(), room), EditorState::rooms.end());

							EditorState::connections.erase(std::remove_if(EditorState::connections.begin(), EditorState::connections.end(),
								[room](Connection *connection) {
									if (connection->roomA == room || connection->roomB == room) {
										connection->roomA->disconnect(connection);
										connection->roomB->disconnect(connection);

										delete connection;
										return true;
									}

									return false;
								}
							), EditorState::connections.end());

							delete room;

							break;
						}
					}
				}
			}
		}
	}

	if (EditorState::window->justPressed(GLFW_KEY_S)) {
		if (EditorState::selectedRooms.size() >= 1) {
			Popups::addPopup(new SubregionPopup(EditorState::window, EditorState::selectedRooms));
		} else {
			for (auto it = EditorState::rooms.rbegin(); it != EditorState::rooms.rend(); it++) {
				Room *room = *it;
				if (!EditorState::visibleLayers[room->layer]) continue;

				if (room->inside(worldMouse)) {
					std::set<Room*> roomGroup;
					roomGroup.insert(room);
					Popups::addPopup(new SubregionPopup(EditorState::window, roomGroup));

					break;
				}
			}
		}
	}

	if (EditorState::window->justPressed(GLFW_KEY_T)) {
		if (EditorState::selectedRooms.size() >= 1) {
			Popups::addPopup(new RoomTagPopup(EditorState::window, EditorState::selectedRooms));
		} else {
			for (auto it = EditorState::rooms.rbegin(); it != EditorState::rooms.rend(); it++) {
				Room *room = *it;
				if (!EditorState::visibleLayers[room->layer]) continue;

				if (room->inside(worldMouse)) {
					if (room->isOffscreen()) break;

					std::set<Room*> roomGroup;
					roomGroup.insert(room);
					Popups::addPopup(new RoomTagPopup(EditorState::window, roomGroup));

					break;
				}
			}
		}
	}

	if (EditorState::window->justPressed(GLFW_KEY_L)) {
		if (EditorState::selectedRooms.size() >= 1) {
			int minimumLayer = 3;

			for (Room *room : EditorState::selectedRooms)
				minimumLayer = std::min(minimumLayer, room->layer);

			minimumLayer = (minimumLayer + 1) % LAYER_COUNT;

			for (Room *room : EditorState::selectedRooms)
				room->layer = minimumLayer;

		} else {
			Room *hoveringRoom = nullptr;
			for (auto it = EditorState::rooms.rbegin(); it != EditorState::rooms.rend(); it++) {
				Room *room = (*it);
				if (!EditorState::visibleLayers[room->layer]) continue;

				if (room->inside(worldMouse)) {
					hoveringRoom = room;
					break;
				}
			}

			if (hoveringRoom != nullptr) {
				hoveringRoom->layer = (hoveringRoom->layer + 1) % LAYER_COUNT;
			}
		}
	}

	if (EditorState::window->justPressed(GLFW_KEY_G)) {
		if (EditorState::selectedRooms.size() >= 1) {
			bool setMerge = true;

			for (Room *room : EditorState::selectedRooms)
				if (room->data.merge) { setMerge = false; break; }

			for (Room *room : EditorState::selectedRooms)
				room->data.merge = setMerge;
		} else {
			Room *hoveringRoom = nullptr;
			for (auto it = EditorState::rooms.rbegin(); it != EditorState::rooms.rend(); it++) {
				Room *room = (*it);

				if (!EditorState::visibleLayers[room->layer]) continue;

				if (room->inside(worldMouse)) {
					hoveringRoom = room;
					break;
				}
			}

			if (hoveringRoom != nullptr) {
				hoveringRoom->data.merge = !hoveringRoom->data.merge;
			}
		}
	}

	if (EditorState::window->justPressed(GLFW_KEY_H)) {
		if (EditorState::selectedRooms.size() >= 1) {
			bool setHidden = true;

			for (Room *room : EditorState::selectedRooms)
				if (room->data.hidden) { setHidden = false; break; }

			for (Room *room : EditorState::selectedRooms)
				room->data.hidden = setHidden;

		} else {
			Room *hoveringRoom = nullptr;
			for (auto it = EditorState::rooms.rbegin(); it != EditorState::rooms.rend(); it++) {
				Room *room = (*it);

				if (!EditorState::visibleLayers[room->layer]) continue;

				if (room->inside(worldMouse)) {
					hoveringRoom = room;
					break;
				}
			}

			if (hoveringRoom != nullptr) {
				hoveringRoom->data.hidden = !hoveringRoom->data.hidden;
			}
		}
	}

	if (EditorState::window->justPressed(GLFW_KEY_C)) {
		bool found = false;

		for (auto it = EditorState::rooms.rbegin(); it != EditorState::rooms.rend(); it++) {
			Room *room = *it;

			Vector2 roomMouse = worldMouse - room->currentPosition();
			Vector2 shortcutPosition;

			if (room->isOffscreen()) {
				for (int i = 0; i < room->DenCount(); i++) {
					shortcutPosition = Vector2(room->Width() * 0.5 - room->DenCount() * 2.0 + i * 4.0 + 2.5, -room->Height() * 0.25 - 0.5);

					if (roomMouse.distanceTo(shortcutPosition) < EditorState::selectorScale) {
						Popups::addPopup(new DenPopup(EditorState::window, room->CreatureDen01(i)));

						found = true;
						break;
					}
				}
			} else {
				for (Vector2i shortcut : room->DenEntrances()) {
					shortcutPosition = Vector2(shortcut.x + 0.5, -1 - shortcut.y + 0.5);

					if (roomMouse.distanceTo(shortcutPosition) < EditorState::selectorScale) {
						Popups::addPopup(new DenPopup(EditorState::window, room->CreatureDen(room->DenId(shortcut))));

						found = true;
						break;
					}
				}
			}

			if (found) break;
		}

		if (!found) {
			for (auto it = EditorState::rooms.rbegin(); it != EditorState::rooms.rend(); it++) {
				Room *room = *it;

				if (room->inside(worldMouse)) {
					if (!room->isOffscreen()) break;

					Popups::addPopup(new DenPopup(EditorState::window, EditorState::offscreenDen->getDen()));
				}
			}
		}
	}

	if (EditorState::window->justPressed(GLFW_KEY_A)) {
		if (EditorState::selectedRooms.size() >= 1) {
			Popups::addPopup(new RoomAttractivenessPopup(EditorState::window, EditorState::selectedRooms));
		} else {
			Room *hoveringRoom = nullptr;
			for (auto it = EditorState::rooms.rbegin(); it != EditorState::rooms.rend(); it++) {
				Room *room = (*it);

				if (!EditorState::visibleLayers[room->layer]) continue;

				if (room->inside(worldMouse)) {
					hoveringRoom = room;
					break;
				}
			}

			if (hoveringRoom != nullptr && !hoveringRoom->isOffscreen()) {
				std::set<Room *> set;
				set.insert(hoveringRoom);
				Popups::addPopup(new RoomAttractivenessPopup(EditorState::window, set));
			}
		}
	}

	if (EditorState::window->justPressed(GLFW_KEY_D)) {
		Connection *openForConnection = nullptr;
		for (auto it = EditorState::connections.rbegin(); it != EditorState::connections.rend(); it++) {
			Connection *connection = *it;
			if (!EditorState::visibleLayers[connection->roomA->layer]) continue;
			if (!EditorState::visibleLayers[connection->roomB->layer]) continue;

			if (connection->hovered(worldMouse, EditorState::lineSize)) {
				openForConnection = connection;

				break;
			}
		}

		if (openForConnection != nullptr) {
			Popups::addPopup(new ConditionalPopup(EditorState::window, openForConnection));
		} else {
			if (EditorState::selectedRooms.size() >= 1) {
				Popups::addPopup(new ConditionalPopup(EditorState::window, EditorState::selectedRooms));
			} else {
				Room *hoveringRoom = nullptr;
				for (auto it = EditorState::rooms.rbegin(); it != EditorState::rooms.rend(); it++) {
					Room *room = (*it);

					if (!EditorState::visibleLayers[room->layer]) continue;

					if (room->inside(worldMouse)) {
						hoveringRoom = room;
						break;
					}
				}

				if (hoveringRoom != nullptr && !hoveringRoom->isOffscreen()) {
					std::set<Room *> set;
					set.insert(hoveringRoom);
					Popups::addPopup(new ConditionalPopup(EditorState::window, set));
				}
			}
		}
	}

	if (EditorState::window->justPressed(GLFW_KEY_R)) {
		if (EditorState::region.acronym == "") {
			Popups::addPopup(new InfoPopup(EditorState::window, "You must create or import a region\nbefore creating or editing a room."));
		} else {
			Room *hoveringRoom = nullptr;
			for (auto it = EditorState::rooms.rbegin(); it != EditorState::rooms.rend(); it++) {
				Room *room = (*it);
	
				if (!EditorState::visibleLayers[room->layer]) continue;
	
				if (room->inside(worldMouse)) {
					hoveringRoom = room;
					break;
				}
			}
	
			if (hoveringRoom == nullptr || hoveringRoom->isOffscreen()) {
				Popups::addPopup(new CreateRoomPopup(EditorState::window));
				EditorState::placingRoom = true;
				EditorState::placingRoomPosition = worldMouse;
			}
		}
	}

	bool found = false;
	for (auto it = EditorState::rooms.rbegin(); it != EditorState::rooms.rend(); it++) {
		Room *room = *it;
		room->hoveredDen = -1;

		Vector2 roomMouse = worldMouse - room->currentPosition();
		Vector2 shortcutPosition;

		if (room->isOffscreen()) {
			for (int i = 0; i < room->DenCount(); i++) {
				shortcutPosition = Vector2(room->Width() * 0.5 - room->DenCount() * 2.0 + i * 4.0 + 2.5, -room->Height() * 0.25 - 0.5);

				if (roomMouse.distanceTo(shortcutPosition) < EditorState::selectorScale) {
					room->hoveredDen = i;

					found = true;
					break;
				}
			}
		} else {
			for (Vector2i shortcut : room->DenEntrances()) {
				shortcutPosition = Vector2(shortcut.x + 0.5, -1 - shortcut.y + 0.5);

				if (roomMouse.distanceTo(shortcutPosition) < EditorState::selectorScale) {
					room->hoveredDen = room->DenId(shortcut) - room->RoomEntranceCount();

					found = true;
					break;
				}
			}
		}

		if (found) break;
	}
}

void FloodForgeWindow::Draw() {
	try {
		updateMain();
	} catch (const std::exception &e) {
		Logger::error("An exception was thrown during updateMain: ", e.what());
		exit(1);
	} catch (...) {
		Logger::error("An unknown exception was thrown during updateMain");
		exit(1);
	}


	// Draw
	applyFrustumToOrthographic(EditorState::cameraOffset, 0.0f, EditorState::cameraScale * EditorState::screenBounds);

	/// Draw Grid
	glLineWidth(1);
	setThemeColor(ThemeColour::Grid);
	double gridStep = std::max(EditorState::cameraScale / 16.0, 1.0);
	gridStep = std::pow(2, std::ceil(std::log2(gridStep - 0.01)));
	Draw::begin(Draw::LINES);
	Vector2 offset = (EditorState::cameraOffset / gridStep).rounded() * gridStep;
	Vector2 extraOffset = Vector2(fmod((EditorState::screenBounds.x - 1.0) * gridStep * 16.0, gridStep), 0);
	Vector2 gridScale = gridStep * 16.0 * EditorState::screenBounds;
	for (float x = -gridScale.x + offset.x; x < gridScale.x + offset.x; x += gridStep) {
		Draw::vertex(x + extraOffset.x, -EditorState::cameraScale * EditorState::screenBounds.y + offset.y + extraOffset.y - gridStep);
		Draw::vertex(x + extraOffset.x,  EditorState::cameraScale * EditorState::screenBounds.y + offset.y + extraOffset.y + gridStep);
	}
	for (float y = -gridScale.y + offset.y; y < gridScale.y + offset.y; y += gridStep) {
		Draw::vertex(-EditorState::cameraScale * EditorState::screenBounds.x + offset.x + extraOffset.x - gridStep, y + extraOffset.y);
		Draw::vertex( EditorState::cameraScale * EditorState::screenBounds.x + offset.x + extraOffset.x + gridStep, y + extraOffset.y);
	}
	Draw::end();

	glLineWidth(EditorState::lineSize);

	/// Draw Rooms
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	for (Room *room : EditorState::rooms) {
		if (!EditorState::visibleLayers[room->layer]) continue;
		if (!room->data.merge) continue;

		room->drawBlack(worldMouse, EditorState::roomPositionType);
	}
	for (Room *room : EditorState::rooms) {
		if (!EditorState::visibleLayers[room->layer]) continue;

		if (!room->data.merge) {
			room->drawBlack(worldMouse, EditorState::roomPositionType);
		}

		room->draw(worldMouse, EditorState::roomPositionType);
		if (EditorState::window->modifierPressed(GLFW_MOD_ALT)) {
			room->draw(worldMouse, (EditorState::roomPositionType == CANON_POSITION) ? DEV_POSITION : CANON_POSITION);
		}
		if (EditorState::selectedRooms.find(room) != EditorState::selectedRooms.end()) {
			setThemeColour(ThemeColour::SelectionBorder);
			Vector2 &roomPosition = room->currentPosition();
			strokeRect(Rect::fromSize(roomPosition.x, roomPosition.y, room->Width(), -room->Height()), 16.0f / EditorState::lineSize);
		}
	}

	if (EditorState::placingRoom) {
		Draw::color(Color(1.0, 1.0, 1.0, 0.5));
		fillRect(Rect(
			EditorState::placingRoomPosition.x - EditorState::placingRoomSize.x * 0.5, EditorState::placingRoomPosition.y - EditorState::placingRoomSize.y * 0.5,
			EditorState::placingRoomPosition.x + EditorState::placingRoomSize.x * 0.5, EditorState::placingRoomPosition.y + EditorState::placingRoomSize.y * 0.5
		));
	}
	glDisable(GL_BLEND);

	/// Draw Connections
	for (Connection *connection : EditorState::connections) {
		if (!EditorState::visibleLayers[connection->roomA->layer]) continue;
		if (!EditorState::visibleLayers[connection->roomB->layer]) continue;

		connection->draw(worldMouse, EditorState::lineSize);
	}

	if (connectionStart != nullptr && connectionEnd != nullptr) {
		if (currentConnectionValid) {
			Draw::color(1.0f, 1.0f, 0.0f);
		} else {
			Draw::color(1.0f, 0.0f, 0.0f);
		}

		Vector2 pointA = connectionStart;
		Vector2 pointB = connectionEnd;

		int segments = int(pointA.distanceTo(pointB) / 2.0);
		segments = std::clamp(segments, 4, 100);
		double directionStrength = pointA.distanceTo(pointB);
		if (directionStrength > 300.0) directionStrength = (directionStrength - 300.0) * 0.5 + 300.0;

		if (Settings::getSetting<int>(Settings::Setting::ConnectionType) == 0) {
			drawLine(pointA.x, pointA.y, pointB.x, pointB.y, 16.0 / EditorState::lineSize);
		} else {
			Vector2 directionA = currentConnection->roomA->getRoomEntranceDirectionVector(currentConnection->connectionA);
			Vector2 directionB = Vector2(0, 0);

			if (currentConnection->roomB != nullptr) directionB = currentConnection->roomB->getRoomEntranceDirectionVector(currentConnection->connectionB);

			if (directionA.x == -directionB.x || directionA.y == -directionB.y) {
				directionStrength *= 0.3333;
			} else {
				directionStrength *= 0.6666;
			}

			directionA *= directionStrength;
			directionB *= directionStrength;

			Vector2 lastPoint = bezierCubic(0.0, pointA, pointA + directionA, pointB + directionB, pointB);
			for (double t = 1.0 / segments; t <= 1.01; t += 1.0 / segments) {
				Vector2 point = bezierCubic(t, pointA, pointA + directionA, pointB + directionB, pointB);

				drawLine(lastPoint.x, lastPoint.y, point.x, point.y, 16.0 / EditorState::lineSize);

				lastPoint = point;
			}
		}
	}

	if (EditorState::selectingState == 1) {
		glEnable(GL_BLEND);
		Draw::color(0.1f, 0.1f, 0.1f, 0.125f);
		fillRect(selectionStart.x, selectionStart.y, selectionEnd.x, selectionEnd.y);
		glDisable(GL_BLEND);
		setThemeColour(ThemeColour::SelectionBorder);
		strokeRect(selectionStart.x, selectionStart.y, selectionEnd.x, selectionEnd.y, 16.0 / EditorState::lineSize);
	}

	/// Draw UI
	applyFrustumToOrthographic(Vector2(0.0f, 0.0f), 0.0f, EditorState::screenBounds);

	if (connectionError != "") {
		Draw::color(1.0, 0.0, 0.0);
		Fonts::rainworld->writeCentered(connectionError, EditorState::mouse->X() / 512.0f - EditorState::screenBounds.x, -EditorState::mouse->Y() / 512.0f + EditorState::screenBounds.y, 0.05, CENTER_X);
	}

	DebugData::draw(EditorState::window, Vector2(
		UI::mouse.x * EditorState::cameraScale + EditorState::cameraOffset.x,
		UI::mouse.y * EditorState::cameraScale + EditorState::cameraOffset.y
	), EditorState::screenBounds);
}