#include "FloodForgeWindow.hpp"

#include <exception>
#include "../../gl.h"

#include "../droplet/DropletWindow.hpp"
#include "../../Logger.hpp"
#include "../Globals.hpp"
#include "../../ui/UI.hpp"
#include "DebugData.hpp"

#include "../../popup/MarkdownPopup.hpp"
#include "../../popup/ConfirmPopup.hpp"
#include "../popup/SplashArtPopup.hpp"
#include "../popup/RoomTagPopup.hpp"
#include "../popup/DenPopup.hpp"
#include "../popup/RoomAttractivenessPopup.hpp"
#include "../popup/ConditionalPopup.hpp"
#include "SubregionPopup.hpp"
#include "CreateRoomPopup.hpp"

Vector2 FloodForgeWindow::worldMouse;

namespace {

	bool cameraPanning = false;
	bool cameraPanningBlocked = false;
	Vector2 cameraPanStartMouse = Vector2(0.0f, 0.0f);
	Vector2 cameraPanStart = Vector2(0.0f, 0.0f);
	Vector2 cameraPanTo = Vector2(0.0f, 0.0f);
	double cameraScaleTo = EditorState::cameraScale;
	
	int roomSnap = ROOM_SNAP_TILE;
	Vector2 selectionStart;
	Vector2 selectionEnd;
	
	Room *holdingRoom = nullptr;
	Vector2 holdingStart = Vector2(0.0f, 0.0f);
	int holdingType = 0;
	
	enum class ConnectionState {
		None,
		NoConnection,
		Connection
	};
	
	Vector2 *connectionStart = nullptr;
	Vector2 *connectionEnd = nullptr;
	Connection *currentConnection = nullptr;
	bool currentConnectionValid = false;
	ConnectionState connectionState = ConnectionState::None;

}

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
	double scrollY = -UI::window->getMouseScrollY();
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
	if (UI::mouse.middleMouse) {
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

void updateConnectionControls() {
	Room *hoveringRoom = nullptr;
	int hoveringConnection = 0;
	double maxDist = EditorState::selectorScale;
	for (auto it = EditorState::rooms.rbegin(); it != EditorState::rooms.rend(); it++) {
		Room *room = *it;
		if (!EditorState::visibleLayers[room->layer]) continue;
		room->hoveredRoomExit = -1;

		for (int i = 0; i < room->RoomEntranceCount(); i++) {
			Vector2 spot = room->getRoomEntranceOffsetPosition(i);
			double dist = FloodForgeWindow::worldMouse.distanceTo(spot);
			if (dist < maxDist) {
				maxDist = dist;
				hoveringRoom = room;
				hoveringConnection = i;
			}
		}
	}
	if (hoveringRoom != nullptr) {
		hoveringRoom->hoveredRoomExit = hoveringConnection;
	}

	if (UI::mouse.rightMouse) {
		if (connectionState == ConnectionState::None) {
			if (hoveringRoom == nullptr) {
				connectionState = ConnectionState::NoConnection;
				return;
			}
	
			connectionStart = new Vector2(hoveringRoom->getRoomEntranceOffsetPosition(hoveringConnection));
			connectionEnd = new Vector2(connectionStart);
			currentConnection = new Connection(hoveringRoom, hoveringConnection, nullptr, 0);
			currentConnectionValid = false;
			connectionState = ConnectionState::Connection;
		} else if (connectionState == ConnectionState::Connection) {
			if (hoveringRoom != nullptr) {
				Vector2 &roomPosition = hoveringRoom->currentPosition();
				connectionEnd->x = hoveringRoom->getRoomEntranceOffsetPosition(hoveringConnection).x;
				connectionEnd->y = hoveringRoom->getRoomEntranceOffsetPosition(hoveringConnection).y;
				currentConnection->roomB = hoveringRoom;
				currentConnection->connectionB = hoveringConnection;
				currentConnectionValid = true;
	
				if (currentConnection->roomA == currentConnection->roomB) {
					currentConnectionValid = false;
				} else {
					for (Connection *other : currentConnection->roomB->connections) {
						if (
							(other->roomA == currentConnection->roomB && other->connectionA == currentConnection->connectionB) &&
							(other->roomB == currentConnection->roomA && other->connectionB == currentConnection->connectionA)
						) {
							currentConnectionValid = false;
							break;
						}
					}
				}
			} else {
				connectionEnd->x = FloodForgeWindow::worldMouse.x;
				connectionEnd->y = FloodForgeWindow::worldMouse.y;
				currentConnection->roomB = nullptr;
				currentConnection->connectionB = 0;
				currentConnectionValid = false;
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
}

void updateOriginalControls() {
	if (UI::mouse.leftMouse) {
		if (!UI::mouse.lastLeftMouse) {
			if (EditorState::selectingState == 0) {
				for (auto it = EditorState::rooms.rbegin(); it != EditorState::rooms.rend(); it++) {
					Room *room = *it;
					if (!EditorState::visibleLayers[room->layer]) continue;

					if (room->inside(FloodForgeWindow::worldMouse)) {
						holdingRoom = room;
						holdingStart = FloodForgeWindow::worldMouse;
						EditorState::roomPossibleSelect = room;
						EditorState::selectingState = 3;
						break;
					}
				}
			}

			if (EditorState::selectingState == 0) {
				if (UI::window->modifierPressed(GLFW_MOD_SHIFT)) {
					EditorState::selectingState = 1;
					selectionStart = FloodForgeWindow::worldMouse;
					selectionEnd = FloodForgeWindow::worldMouse;
					if (!UI::window->modifierPressed(GLFW_MOD_CONTROL)) EditorState::selectedRooms.clear();
				} else {
					EditorState::selectingState = 5;
					selectionStart = EditorState::globalMouse;
					selectionEnd = EditorState::globalMouse;
				}
			}
		} else {
			if (EditorState::selectingState == 3 && UI::mouse.moved() || EditorState::selectingState == 4) {
				if (EditorState::selectingState == 3) {
					if (UI::window->modifierPressed(GLFW_MOD_SHIFT) || UI::window->modifierPressed(GLFW_MOD_CONTROL)) {
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

				Vector2 offset = (FloodForgeWindow::worldMouse - holdingStart);
				if (roomSnap == ROOM_SNAP_TILE) offset.round();

				for (Room *room2 : EditorState::selectedRooms) {
					Vector2 &roomPosition = room2->currentPosition();
					if (roomSnap == ROOM_SNAP_TILE) {
						roomPosition.round();
					}

					roomPosition.add(offset);

					if (UI::window->modifierPressed(GLFW_MOD_ALT) || EditorState::positionType == PositionType::BOTH) {
						room2->moveBoth();
					}
				}
				holdingStart = holdingStart + offset;
			}

			if (EditorState::selectingState == 1) {
				selectionEnd = FloodForgeWindow::worldMouse;
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
			if (UI::window->modifierPressed(GLFW_MOD_SHIFT) || UI::window->modifierPressed(GLFW_MOD_CONTROL)) {
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

void updateFloodForgeControls() {
	if (UI::mouse.leftMouse) {
		if (!UI::mouse.lastLeftMouse) {
			if (EditorState::selectingState == 0) {
				for (auto it = EditorState::rooms.rbegin(); it != EditorState::rooms.rend(); it++) {
					Room *room = *it;
					if (!EditorState::visibleLayers[room->layer]) continue;

					if (room->inside(FloodForgeWindow::worldMouse)) {
						holdingRoom = room;
						holdingStart = FloodForgeWindow::worldMouse;
						EditorState::roomPossibleSelect = room;
						EditorState::selectingState = 3;
						break;
					}
				}
			}

			if (EditorState::selectingState == 0) {
				EditorState::selectingState = 1;
				selectionStart = FloodForgeWindow::worldMouse;
				selectionEnd = FloodForgeWindow::worldMouse;
				if (!UI::window->modifierPressed(GLFW_MOD_SHIFT) && !UI::window->modifierPressed(GLFW_MOD_CONTROL)) {
					EditorState::selectedRooms.clear();
				}
			}
		} else {
			if (EditorState::selectingState == 3 && UI::mouse.moved() || EditorState::selectingState == 4) {
				if (EditorState::selectingState == 3) {
					if (UI::window->modifierPressed(GLFW_MOD_SHIFT) || UI::window->modifierPressed(GLFW_MOD_CONTROL)) {
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

				Vector2 offset = (FloodForgeWindow::worldMouse - holdingStart);
				if (roomSnap == ROOM_SNAP_TILE) offset.round();

				for (Room *room2 : EditorState::selectedRooms) {
					Vector2 &roomPosition = room2->currentPosition();
					if (roomSnap == ROOM_SNAP_TILE) {
						roomPosition.round();
					}

					roomPosition.add(offset);
					if (UI::window->modifierPressed(GLFW_MOD_ALT) || EditorState::positionType == PositionType::BOTH) {
						room2->moveBoth();
					}
				}
				holdingStart = holdingStart + offset;
			}

			if (EditorState::selectingState == 1) {
				selectionEnd = FloodForgeWindow::worldMouse;
				// selectedRooms.clear();
			}
		}
	} else {
		if (EditorState::selectingState == 3) {
			EditorState::rooms.erase(std::remove(EditorState::rooms.begin(), EditorState::rooms.end(), EditorState::roomPossibleSelect), EditorState::rooms.end());
			EditorState::rooms.push_back(EditorState::roomPossibleSelect);
			if (UI::window->modifierPressed(GLFW_MOD_SHIFT) || UI::window->modifierPressed(GLFW_MOD_CONTROL)) {
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

	double scale = Settings::getSetting<double>(Settings::Setting::WorldIconScale);
	EditorState::selectorScale = (scale < 0.0) ? EditorState::cameraScale / 16.0 : scale;

	/// Update Inputs

	if (UI::window->modifierPressed(GLFW_MOD_ALT)) {
		roomSnap = ROOM_SNAP_NONE;
	} else {
		roomSnap = ROOM_SNAP_TILE;
	}

	if (UI::window->justPressed(GLFW_KEY_F11)) {
		UI::window->toggleFullscreen();
	}

	if (UI::window->justPressed(GLFW_KEY_ESCAPE)) {
		if (Popups::popups.size() > 0) {
			Popups::popups[Popups::popups.size() - 1]->reject();
		} else {
			Popups::addPopup((new ConfirmPopup("Exit FloodForge?"))->OnOkay([&]() {
				UI::window->close();
			}));
		}
	}

	if (UI::window->justPressed(GLFW_KEY_ENTER)) {
		if (Popups::popups.size() > 0) {
			Popups::popups[0]->accept();
		}
	}

	if (UI::window->modifierPressed(GLFW_MOD_ALT) && UI::window->justPressed(GLFW_KEY_T)) {
		Popups::addPopup(new MarkdownPopup(BASE_PATH / "docs" / "controls.md"));
	}

	// Connections
	updateConnectionControls();

	// Holding
	if (Settings::getSetting<bool>(Settings::Setting::OrignalControls)) {
		updateOriginalControls();
	} else {
		updateFloodForgeControls();
	}

	if (!Popups::popups.empty()) return;

	if (UI::window->justPressed(GLFW_KEY_I)) {
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

	if (UI::window->justPressed(GLFW_KEY_X)) {
		bool deleted = false;

		for (auto it = EditorState::connections.rbegin(); it != EditorState::connections.rend(); it++) {
			Connection *connection = *it;
			if (!EditorState::visibleLayers[connection->roomA->layer]) continue;
			if (!EditorState::visibleLayers[connection->roomB->layer]) continue;

			if (connection->hovered(worldMouse)) {
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

	if (UI::window->justPressed(GLFW_KEY_S)) {
		if (EditorState::selectedRooms.size() >= 1) {
			Popups::addPopup(new SubregionPopup(EditorState::selectedRooms));
		} else {
			for (auto it = EditorState::rooms.rbegin(); it != EditorState::rooms.rend(); it++) {
				Room *room = *it;
				if (!EditorState::visibleLayers[room->layer]) continue;

				if (room->inside(worldMouse)) {
					std::set<Room*> roomGroup;
					roomGroup.insert(room);
					Popups::addPopup(new SubregionPopup(roomGroup));

					break;
				}
			}
		}
	}

	if (UI::window->justPressed(GLFW_KEY_T)) {
		if (EditorState::selectedRooms.size() >= 1) {
			Popups::addPopup(new RoomTagPopup(EditorState::selectedRooms));
		} else {
			for (auto it = EditorState::rooms.rbegin(); it != EditorState::rooms.rend(); it++) {
				Room *room = *it;
				if (!EditorState::visibleLayers[room->layer]) continue;

				if (room->inside(worldMouse)) {
					if (room->isOffscreen()) break;

					std::set<Room*> roomGroup;
					roomGroup.insert(room);
					Popups::addPopup(new RoomTagPopup(roomGroup));

					break;
				}
			}
		}
	}

	if (UI::window->justPressed(GLFW_KEY_L)) {
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

	if (UI::window->justPressed(GLFW_KEY_G)) {
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

	if (UI::window->justPressed(GLFW_KEY_H)) {
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

	if (UI::window->justPressed(GLFW_KEY_C)) {
		bool found = false;

		for (auto it = EditorState::rooms.rbegin(); it != EditorState::rooms.rend(); it++) {
			Room *room = *it;

			Vector2 roomMouse = worldMouse - room->currentPosition();
			Vector2 shortcutPosition;

			if (room->isOffscreen()) {
				for (int i = 0; i < room->DenCount(); i++) {
					shortcutPosition = Vector2(room->Width() * 0.5 - room->DenCount() * 2.0 + i * 4.0 + 2.5, -room->Height() * 0.25 - 0.5);

					if (roomMouse.distanceTo(shortcutPosition) < EditorState::selectorScale) {
						Popups::addPopup(new DenPopup(room->CreatureDen01(i)));

						found = true;
						break;
					}
				}
			} else {
				for (Vector2i shortcut : room->DenEntrances()) {
					shortcutPosition = Vector2(shortcut.x + 0.5, -1 - shortcut.y + 0.5);

					if (roomMouse.distanceTo(shortcutPosition) < EditorState::selectorScale) {
						Popups::addPopup(new DenPopup(room->CreatureDen(room->DenId(shortcut))));

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

					Popups::addPopup(new DenPopup(EditorState::offscreenDen->getDen()));
				}
			}
		}
	}

	if (UI::window->justPressed(GLFW_KEY_A)) {
		if (EditorState::selectedRooms.size() >= 1) {
			Popups::addPopup(new RoomAttractivenessPopup(EditorState::selectedRooms));
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
				Popups::addPopup(new RoomAttractivenessPopup(set));
			}
		}
	}

	if (UI::window->justPressed(GLFW_KEY_D)) {
		Connection *openForConnection = nullptr;
		for (auto it = EditorState::connections.rbegin(); it != EditorState::connections.rend(); it++) {
			Connection *connection = *it;
			if (!EditorState::visibleLayers[connection->roomA->layer]) continue;
			if (!EditorState::visibleLayers[connection->roomB->layer]) continue;

			if (connection->hovered(worldMouse)) {
				openForConnection = connection;

				break;
			}
		}

		if (openForConnection != nullptr) {
			Popups::addPopup(new ConditionalPopup(openForConnection));
		} else {
			if (EditorState::selectedRooms.size() >= 1) {
				Popups::addPopup(new ConditionalPopup(EditorState::selectedRooms));
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
					Popups::addPopup(new ConditionalPopup(set));
				}
			}
		}
	}

	if (UI::window->justPressed(GLFW_KEY_R)) {
		if (EditorState::region.acronym.empty()) {
			Popups::addPopup(new InfoPopup("You must create or import a region\nbefore creating or editing a room."));
		} else if (EditorState::region.exportDirectory.empty()) {
			Popups::addPopup(new InfoPopup("You must export your region\nbefore creating or editing a room."));
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
				Popups::addPopup(new CreateRoomPopup());
				EditorState::placingRoom = true;
				EditorState::placingRoomPosition = worldMouse;
			} else {
				EditorState::dropletOpen = true;
				DropletWindow::room = hoveringRoom;
				DropletWindow::loadRoom();
			}
		}
	}

	bool found = false;
	for (auto it = EditorState::rooms.rbegin(); it != EditorState::rooms.rend(); it++) {
		Room *room = *it;
		room->hoveredDen = -1;

		Vector2 roomMouse = worldMouse - room->currentPosition();
		Vector2 shortcutPosition;
		double closestDistance = EditorState::selectorScale;

		if (room->isOffscreen()) {
			for (int i = 0; i < room->DenCount(); i++) {
				shortcutPosition = Vector2(room->Width() * 0.5 - room->DenCount() * 2.0 + i * 4.0 + 2.5, -room->Height() * 0.25 - 0.5);

				if (roomMouse.distanceTo(shortcutPosition) < closestDistance) {
					room->hoveredDen = i;
					closestDistance = roomMouse.distanceTo(shortcutPosition);

					found = true;
				}
			}
		} else {
			for (Vector2i shortcut : room->DenEntrances()) {
				shortcutPosition = Vector2(shortcut.x + 0.5, -1 - shortcut.y + 0.5);

				if (roomMouse.distanceTo(shortcutPosition) < closestDistance) {
					room->hoveredDen = room->DenId(shortcut) - room->RoomEntranceCount();
					closestDistance = roomMouse.distanceTo(shortcutPosition);

					found = true;
				}
			}
		}

		if (found) break;
	}
}

void FloodForgeWindow::Draw() {
	try {
		updateMain();
	} catch (std::exception e) {
		Logger::error("An exception was thrown during updateMain: ", e.what());
		exit(1);
	} catch (...) {
		Logger::error("An unknown exception was thrown during updateMain");
		exit(1);
	}


	// Draw
	applyFrustumToOrthographic(EditorState::cameraOffset, 0.0f, EditorState::cameraScale * UI::screenBounds);

	/// Draw Grid
	glLineWidth(1);
	setThemeColor(ThemeColor::Grid);
	double gridStep = std::max(EditorState::cameraScale / 16.0, 1.0);
	gridStep = std::pow(2, std::ceil(std::log2(gridStep - 0.01)));
	Draw::begin(Draw::LINES);
	Vector2 offset = (EditorState::cameraOffset / gridStep).rounded() * gridStep;
	Vector2 extraOffset = Vector2(fmod((UI::screenBounds.x - 1.0) * gridStep * 16.0, gridStep), 0);
	Vector2 gridScale = gridStep * 16.0 * UI::screenBounds;
	for (float x = -gridScale.x + offset.x; x < gridScale.x + offset.x; x += gridStep) {
		Draw::vertex(x + extraOffset.x, -EditorState::cameraScale * UI::screenBounds.y + offset.y + extraOffset.y - gridStep);
		Draw::vertex(x + extraOffset.x,  EditorState::cameraScale * UI::screenBounds.y + offset.y + extraOffset.y + gridStep);
	}
	for (float y = -gridScale.y + offset.y; y < gridScale.y + offset.y; y += gridStep) {
		Draw::vertex(-EditorState::cameraScale * UI::screenBounds.x + offset.x + extraOffset.x - gridStep, y + extraOffset.y);
		Draw::vertex( EditorState::cameraScale * UI::screenBounds.x + offset.x + extraOffset.x + gridStep, y + extraOffset.y);
	}
	Draw::end();

	glLineWidth(EditorState::lineSize);

	/// Draw Rooms
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	for (Room *room : EditorState::rooms) {
		if (!EditorState::visibleLayers[room->layer]) continue;
		if (!room->data.merge) continue;

		if (EditorState::positionType == PositionType::BOTH) {
			room->drawBlack(worldMouse, PositionType::CANON);
			room->drawBlack(worldMouse, PositionType::DEV);
		} else {
			room->drawBlack(worldMouse, EditorState::positionType);
		}
	}
	for (Room *room : EditorState::rooms) {
		if (!EditorState::visibleLayers[room->layer]) continue;

		if (!room->data.merge) {
			if (EditorState::positionType == PositionType::BOTH) {
				room->drawBlack(worldMouse, PositionType::CANON);
				room->drawBlack(worldMouse, PositionType::DEV);
			} else {
				room->drawBlack(worldMouse, EditorState::positionType);
			}
		}

		if (EditorState::positionType == PositionType::BOTH) {
			room->draw(worldMouse, PositionType::CANON);
			room->draw(worldMouse, PositionType::DEV);
		} else {
			room->draw(worldMouse, EditorState::positionType);
			if (UI::window->modifierPressed(GLFW_MOD_ALT)) {
				room->draw(worldMouse, (EditorState::positionType == PositionType::CANON) ? PositionType::DEV : PositionType::CANON);
			}
		}

		if (EditorState::selectedRooms.find(room) != EditorState::selectedRooms.end()) {
			setThemeColor(ThemeColor::SelectionBorder);
			if (EditorState::positionType == PositionType::DEV || EditorState::positionType == PositionType::BOTH) {
				strokeRect(Rect::fromSize(room->devPosition.x, room->devPosition.y, room->Width(), -room->Height()), 16.0f / EditorState::lineSize);
			}
			if (EditorState::positionType == PositionType::CANON || EditorState::positionType == PositionType::BOTH) {
				strokeRect(Rect::fromSize(room->canonPosition.x, room->canonPosition.y, room->Width(), -room->Height()), 16.0f / EditorState::lineSize);
			}
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
		connection->draw(worldMouse);
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
		setThemeColor(ThemeColor::SelectionBorder);
		strokeRect(selectionStart.x, selectionStart.y, selectionEnd.x, selectionEnd.y, 16.0 / EditorState::lineSize);
	}

	/// Draw UI
	applyFrustumToOrthographic(Vector2(0.0f, 0.0f), 0.0f, UI::screenBounds);

	DebugData::draw(UI::window, Vector2(
		UI::mouse.x * EditorState::cameraScale + EditorState::cameraOffset.x,
		UI::mouse.y * EditorState::cameraScale + EditorState::cameraOffset.y
	), UI::screenBounds);
}