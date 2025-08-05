#include "../gl.h"

#include <iostream>
#include <fstream>
#include <cmath>
#include <vector>
#include <algorithm>
#include <csignal>

#include "../Constants.hpp"
#include "../Window.hpp"
#include "../Utils.hpp"
#include "../Texture.hpp"
#include "../math/Vector.hpp"
#include "../math/Rect.hpp"
#include "../font/Fonts.hpp"
#include "../Theme.hpp"
#include "../Draw.hpp"
#include "../Settings.hpp"

#include "../popup/Popups.hpp"
#include "../popup/MarkdownPopup.hpp"
#include "../popup/ConfirmPopup.hpp"
#include "SplashArtPopup.hpp"
#include "SubregionPopup.hpp"
#include "RoomTagPopup.hpp"
#include "DenPopup.hpp"
#include "RoomAttractivenessPopup.hpp"

#include "Shaders.hpp"
#include "Globals.hpp"
#include "Room.hpp"
#include "OffscreenRoom.hpp"
#include "Connection.hpp"
#include "MenuItems.hpp"
#include "DebugData.hpp"
#include "RecentFiles.hpp"

#define clamp(x, a, b) x >= b ? b : (x <= a ? a : x)
#define min(a, b) (a < b) ? a : b
#define max(a, b) (a > b) ? a : b

int transitionLayer(int layer) {
	return (layer + 1) % 3;
}





bool leftMouseDown;

std::set<int> previousKeys;
Vector2 lastMousePosition;

bool cameraPanning = false;
bool cameraPanningBlocked = false;
Vector2 cameraPanStartMouse = Vector2(0.0f, 0.0f);
Vector2 cameraPanStart = Vector2(0.0f, 0.0f);
Vector2 cameraPanTo = Vector2(0.0f, 0.0f);
double cameraScaleTo = EditorState::cameraScale;

Room *holdingRoom = nullptr;
Popup *holdingPopup = nullptr;
Vector2 holdingStart = Vector2(0.0f, 0.0f);
int holdingType = 0;

Vector2 selectionStart;
Vector2 selectionEnd;
int roomSnap = ROOM_SNAP_TILE;

Vector2 *connectionStart = nullptr;
Vector2 *connectionEnd = nullptr;
Connection *currentConnection = nullptr;
std::string connectionError = "";

int connectionState = 0;

Vector2 worldMouse;
Vector2 globalMouse;
Vector2 screenMouse;
bool mouseMoved;
double lineSize;

bool justPressed(int key) {
	if (EditorState::window->keyPressed(key)) {
		if (previousKeys.find(key) == previousKeys.end()) {
			previousKeys.insert(key);
			return true;
		}
		
		previousKeys.insert(key);
	} else {
		previousKeys.erase(key);
	}

	return false;
}

void updateCamera() {
	bool isHoveringPopup = false;
	for (Popup *popup : Popups::popups) {
		Rect bounds = popup->Bounds();

		if (bounds.inside(Vector2(screenMouse.x, screenMouse.y))) {
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
		screenMouse.x * EditorState::cameraScale + EditorState::cameraOffset.x,
		screenMouse.y * EditorState::cameraScale + EditorState::cameraOffset.y
	);

	cameraScaleTo *= zoom;
	EditorState::cameraScale += (cameraScaleTo - EditorState::cameraScale) * Settings::getSetting<double>(Settings::Setting::CameraZoomSpeed);

	worldMouse = Vector2(
		screenMouse.x * EditorState::cameraScale + EditorState::cameraOffset.x,
		screenMouse.y * EditorState::cameraScale + EditorState::cameraOffset.y
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
				cameraPanStartMouse.x = globalMouse.x;
				cameraPanStartMouse.y = globalMouse.y;
				cameraPanning = true;
			}
		}

		if (cameraPanning && !cameraPanningBlocked) {
			cameraPanTo.x = cameraPanStart.x + EditorState::cameraScale * (cameraPanStartMouse.x - globalMouse.x) / 512.0;
			cameraPanTo.y = cameraPanStart.y + EditorState::cameraScale * (cameraPanStartMouse.y - globalMouse.y) / -512.0;
		}
	} else {
		cameraPanning = false;
		cameraPanningBlocked = false;
	}

	EditorState::cameraOffset.x += (cameraPanTo.x - EditorState::cameraOffset.x) * Settings::getSetting<double>(Settings::Setting::CameraPanSpeed);
	EditorState::cameraOffset.y += (cameraPanTo.y - EditorState::cameraOffset.y) * Settings::getSetting<double>(Settings::Setting::CameraPanSpeed);
}

void updateOriginalControls() {
	if (EditorState::mouse->Left()) {
		if (!leftMouseDown) {
			for (Popup *popup : Popups::popups) {
				Rect bounds = popup->Bounds();

				if (bounds.inside(screenMouse)) {
					popup->mouseClick(screenMouse.x, screenMouse.y);
					if (popup->drag(screenMouse.x, screenMouse.y)) {
						holdingPopup = popup;
						holdingStart = screenMouse;
					}
					EditorState::selectingState = 2;
					break;
				}
			}

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
					selectionStart = globalMouse;
					selectionEnd = globalMouse;
				}
			}
		} else {
			if (EditorState::selectingState == 3 && mouseMoved || EditorState::selectingState == 4) {
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

			if (holdingPopup != nullptr) {
				holdingPopup->offset(screenMouse - holdingStart);
				holdingStart = screenMouse;
			}

			if (EditorState::selectingState == 1) {
				selectionEnd = worldMouse;
			}

			if (EditorState::selectingState == 5) {
				selectionEnd = globalMouse;

				cameraPanTo.x += (selectionStart.x - selectionEnd.x) * EditorState::cameraScale / 512;
				cameraPanTo.y += (selectionStart.y - selectionEnd.y) * EditorState::cameraScale / -512;

				selectionStart = selectionEnd;
			}
		}

		leftMouseDown = true;
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

		leftMouseDown = false;
		holdingRoom = nullptr;
		holdingPopup = nullptr;

		if (EditorState::selectingState == 1) {
			for (Room *room : EditorState::rooms) {
				if (room->intersects(selectionStart, selectionEnd)) EditorState::selectedRooms.insert(room);
			}
		}
		EditorState::selectingState = 0;
	}
}

void updateFloodForgeControls() {
	if (EditorState::mouse->Left()) {
		if (!leftMouseDown) {
			for (Popup *popup : Popups::popups) {
				Rect bounds = popup->Bounds();

				if (bounds.inside(screenMouse)) {
					popup->mouseClick(screenMouse.x, screenMouse.y);
					if (popup->drag(screenMouse.x, screenMouse.y)) {
						holdingPopup = popup;
						holdingStart = screenMouse;
					}
					EditorState::selectingState = 2;
					break;
				}
			}

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
			if (EditorState::selectingState == 3 && mouseMoved || EditorState::selectingState == 4) {
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

			if (holdingPopup != nullptr) {
				holdingPopup->offset(screenMouse - holdingStart);
				holdingStart = screenMouse;
			}

			if (EditorState::selectingState == 1) {
				selectionEnd = worldMouse;
				// selectedRooms.clear();
			}
		}

		leftMouseDown = true;
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

		leftMouseDown = false;
		holdingRoom = nullptr;
		holdingPopup = nullptr;

		if (EditorState::selectingState == 1) {
			for (Room *room : EditorState::rooms) {
				if (room->intersects(selectionStart, selectionEnd)) EditorState::selectedRooms.insert(room);
			}
		}
		EditorState::selectingState = 0;
	}
}

void updateMain() {
	updateCamera();

	EditorState::selectorScale = Settings::getSetting<bool>(Settings::Setting::SelectorScale) ? EditorState::cameraScale / 16.0 : 1.0;

	/// Update Inputs

	if (EditorState::window->modifierPressed(GLFW_MOD_ALT)) {
		roomSnap = ROOM_SNAP_NONE;
	} else {
		roomSnap = ROOM_SNAP_TILE;
	}

	if (justPressed(GLFW_KEY_F11)) {
		EditorState::window->toggleFullscreen();
	}

	if (justPressed(GLFW_KEY_ESCAPE)) {
		if (Popups::popups.size() > 0) {
			Popups::popups[0]->reject();
		} else {
			Popups::addPopup((new ConfirmPopup(EditorState::window, "Exit FloodForge?"))->OnOkay([&]() {
				EditorState::window->close();
			}));
		}
	}

	if (justPressed(GLFW_KEY_ENTER)) {
		if (Popups::popups.size() > 0) {
			Popups::popups[0]->accept();
		}
	}
	
	if (EditorState::window->modifierPressed(GLFW_MOD_ALT) && justPressed(GLFW_KEY_T)) {
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

		if (connectionState == 0) {
			if (connectionStart != nullptr) { delete connectionStart; connectionStart = nullptr; }
			if (connectionEnd   != nullptr) { delete connectionEnd;   connectionEnd   = nullptr; }

			if (hoveringRoom != nullptr) {
				Vector2 &roomPosition = hoveringRoom->currentPosition();
				int connectionId = hoveringRoom->getRoomEntranceId(tilePosition);

				if (connectionId != -1 && !hoveringRoom->ConnectionUsed(connectionId)) {
					connectionStart = new Vector2(floor(worldMouse.x - roomPosition.x) + 0.5 + roomPosition.x, floor(worldMouse.y - roomPosition.y) + 0.5 + roomPosition.y);
					connectionEnd   = new Vector2(connectionStart);
					currentConnection = new Connection(hoveringRoom, connectionId, nullptr, 0);
				}
			}

			connectionState = (connectionStart == nullptr) ? 2 : 1;
		} else if (connectionState == 1) {
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

				if (currentConnection->roomA == currentConnection->roomB) {
					connectionError = "Can't connect to same room";
				} else if (currentConnection->roomB != nullptr && currentConnection->roomB->ConnectionUsed(currentConnection->connectionB)) {
					connectionError = "Already connected";
				} else if (currentConnection->roomA->RoomUsed(currentConnection->roomB) || currentConnection->roomB->RoomUsed(currentConnection->roomA)) {
					connectionError = "Can't connect to room already connected to";
				}
			} else {
				connectionEnd->x = worldMouse.x;
				connectionEnd->y = worldMouse.y;
				currentConnection->roomB = nullptr;
				currentConnection->connectionB = 0;
				connectionError = "Needs to connect";
			}
		}
	} else {
		if (currentConnection != nullptr) {
			bool valid = true;

			if (currentConnection->roomA == currentConnection->roomB) valid = false;
			if (currentConnection->roomA == nullptr) valid = false;
			if (currentConnection->roomB == nullptr) valid = false;

			if (currentConnection->roomA != nullptr && currentConnection->roomB != nullptr) {
				if (currentConnection->roomA->ConnectionUsed(currentConnection->connectionA)) valid = false;
				if (currentConnection->roomB->ConnectionUsed(currentConnection->connectionB)) valid = false;
				if (currentConnection->roomA->RoomUsed(currentConnection->roomB)) valid = false;
				if (currentConnection->roomB->RoomUsed(currentConnection->roomA)) valid = false;
			}

			if (valid) {
				EditorState::connections.push_back(currentConnection);
				currentConnection->roomA->connect(currentConnection->roomB, currentConnection->connectionA);
				currentConnection->roomB->connect(currentConnection->roomA, currentConnection->connectionB);
			} else {
				delete currentConnection;
			}

			currentConnection = nullptr;
		}

		if (connectionStart != nullptr) { delete connectionStart; connectionStart = nullptr; }
		if (connectionEnd   != nullptr) { delete connectionEnd;   connectionEnd   = nullptr; }

		connectionState = 0;
	}

	//// Holding
	if (Settings::getSetting<bool>(Settings::Setting::OrignalControls)) {
		updateOriginalControls();
	} else {
		updateFloodForgeControls();
	}

	if (justPressed(GLFW_KEY_I)) {
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

	if (justPressed(GLFW_KEY_X)) {
		bool deleted = false;

		for (auto it = EditorState::connections.rbegin(); it != EditorState::connections.rend(); it++) {
			Connection *connection = *it;
			if (!EditorState::visibleLayers[connection->roomA->layer]) continue;
			if (!EditorState::visibleLayers[connection->roomB->layer]) continue;

			if (connection->hovered(worldMouse, lineSize)) {
				EditorState::connections.erase(std::remove(EditorState::connections.begin(), EditorState::connections.end(), connection), EditorState::connections.end());

				connection->roomA->disconnect(connection->roomB, connection->connectionA);
				connection->roomB->disconnect(connection->roomA, connection->connectionB);

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
						EditorState::rooms.erase(std::remove(EditorState::rooms.begin(), EditorState::rooms.end(), room), EditorState::rooms.end());

						EditorState::connections.erase(std::remove_if(EditorState::connections.begin(), EditorState::connections.end(),
							[room](Connection *connection) {
								if (connection->roomA == room || connection->roomB == room) {
									connection->roomA->disconnect(connection->roomB, connection->connectionA);
									connection->roomB->disconnect(connection->roomA, connection->connectionB);

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
										connection->roomA->disconnect(connection->roomB, connection->connectionA);
										connection->roomB->disconnect(connection->roomA, connection->connectionB);

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

	if (justPressed(GLFW_KEY_S)) {
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

	if (justPressed(GLFW_KEY_T)) {
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

	if (justPressed(GLFW_KEY_L)) {
		if (EditorState::selectedRooms.size() >= 1) {
			int minimumLayer = 3;

			for (Room *room : EditorState::selectedRooms)
				minimumLayer = min(minimumLayer, room->layer);

			minimumLayer = transitionLayer(minimumLayer);

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
				hoveringRoom->layer = (transitionLayer(hoveringRoom->layer));
			}
		}
	}

	if (justPressed(GLFW_KEY_G)) {
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

	if (justPressed(GLFW_KEY_H)) {
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

	if (justPressed(GLFW_KEY_C)) {
		bool found = false;

		for (auto it = EditorState::rooms.rbegin(); it != EditorState::rooms.rend(); it++) {
			Room *room = *it;

			Vector2 roomMouse = worldMouse - room->currentPosition();
			Vector2 shortcutPosition;

			if (room->isOffscreen()) {
				for (int i = 0; i < room->DenCount(); i++) {
					shortcutPosition = Vector2(room->Width() * 0.5 - room->DenCount() * 2.0 + i * 4.0 + 2.5, -room->Height() * 0.25 - 0.5);

					if (roomMouse.distanceTo(shortcutPosition) < EditorState::selectorScale) {
						Popups::addPopup(new DenPopup(EditorState::window, room, i));

						found = true;
						break;
					}
				}
			} else {
				for (Vector2i shortcut : room->DenEntrances()) {
					shortcutPosition = Vector2(shortcut.x + 0.5, -1 - shortcut.y + 0.5);

					if (roomMouse.distanceTo(shortcutPosition) < EditorState::selectorScale) {
						Popups::addPopup(new DenPopup(EditorState::window, room, room->DenId(shortcut)));

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

					Popups::addPopup(new DenPopup(EditorState::window, room, EditorState::offscreenDen->AddDen()));
				}
			}
		}
	}

	if (justPressed(GLFW_KEY_A)) {
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

	if (!Popups::hasPopup("DenPopup") && EditorState::offscreenDen != nullptr) {
		EditorState::offscreenDen->cleanup();
	}
}

void signalHandler(int signal) {
	Logger::logError("Signal caught: ", signal);
	std::exit(1);
}

int main() {
	std::signal(SIGSEGV, signalHandler); // Segmentation fault
	std::signal(SIGABRT, signalHandler); // Abort signal
	std::signal(SIGFPE, signalHandler);  // Floating-point error
	std::signal(SIGILL, signalHandler);  // Illegal instruction
	std::signal(SIGINT, signalHandler);  // Ctrl+C
	std::signal(SIGTERM, signalHandler); // Termination request

	EditorState::window = new Window(1024, 1024);
	Logger::log("Main icon path: ", BASE_PATH / "assets" / "mainIcon.png");
	EditorState::window->setIcon(BASE_PATH / "assets" / "mainIcon.png");
	EditorState::window->setTitle("FloodForge World Editor");
	EditorState::mouse = EditorState::window->GetMouse();

	if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
		Logger::logError("Failed to initialize GLAD!");
		return -1;
	}

	Settings::init();
	Fonts::init();
	MenuItems::init(EditorState::window);
	Popups::init();
	Shaders::init();
	Draw::init();
	CreatureTextures::init();
	RecentFiles::init();
	RoomHelpers::loadColours();

	Popups::addPopup(new SplashArtPopup(EditorState::window));

	while (EditorState::window->isOpen()) {
		EditorState::mouse->updateLastPressed();
		glfwPollEvents();

		EditorState::window->ensureFullscreen();

		int width;
		int height;
		glfwGetWindowSize(EditorState::window->getGLFWWindow(), &width, &height);
		if (width == 0 || height == 0) continue;
		float size = min(width, height);
		float offsetX = (width * 0.5) - size * 0.5;
		float offsetY = (height * 0.5) - size * 0.5;

		mouseMoved = (EditorState::mouse->X() != lastMousePosition.x || EditorState::mouse->Y() != lastMousePosition.y);

		globalMouse = Vector2(
			(EditorState::mouse->X() - offsetX) / size * 1024,
			(EditorState::mouse->Y() - offsetY) / size * 1024
		);
		screenMouse = Vector2(
			(globalMouse.x / 1024.0) *  2.0 - 1.0,
			(globalMouse.y / 1024.0) * -2.0 + 1.0
		);

		Mouse globalMouseObj = Mouse(EditorState::window->getGLFWWindow(), globalMouse.x, globalMouse.y);
		globalMouseObj.copyPressed(*EditorState::mouse);

		lineSize = 64.0 / EditorState::cameraScale;

		// Update

		try {
			updateMain();
		} catch (const std::exception &e) {
			Logger::logError("An exception was thrown during updateMain: ", e.what());
			exit(1);
		} catch (...) {
			Logger::logError("An unknown exception was trown during updateMain");
			exit(1);
		}


		// Draw

		glViewport(0, 0, width, height);

		EditorState::window->clear();
		glDisable(GL_DEPTH_TEST);

		setThemeColour(ThemeColour::Background);
		Vector2 screenBounds = Vector2(width, height) / size;
		fillRect(-screenBounds.x, -screenBounds.y, screenBounds.x, screenBounds.y);

		applyFrustumToOrthographic(EditorState::cameraOffset, 0.0f, EditorState::cameraScale * screenBounds);

		/// Draw Grid
		glLineWidth(1);
		setThemeColor(ThemeColour::Grid);
		double gridStep = max(EditorState::cameraScale / 16.0, 1.0);
		gridStep = std::pow(2, std::ceil(std::log2(gridStep - 0.01)));
		Draw::begin(Draw::LINES);
		Vector2 offset = (EditorState::cameraOffset / gridStep).rounded() * gridStep;
		Vector2 extraOffset = Vector2(fmod((screenBounds.x - 1.0) * gridStep * 16.0, gridStep), 0);
		Vector2 gridScale = gridStep * 16.0 * screenBounds;
		for (float x = -gridScale.x + offset.x; x < gridScale.x + offset.x; x += gridStep) {
			Draw::vertex(x + extraOffset.x, -EditorState::cameraScale * screenBounds.y + offset.y + extraOffset.y - gridStep);
			Draw::vertex(x + extraOffset.x,  EditorState::cameraScale * screenBounds.y + offset.y + extraOffset.y + gridStep);
		}
		for (float y = -gridScale.y + offset.y; y < gridScale.y + offset.y; y += gridStep) {
			Draw::vertex(-EditorState::cameraScale * screenBounds.x + offset.x + extraOffset.x - gridStep, y + extraOffset.y);
			Draw::vertex( EditorState::cameraScale * screenBounds.x + offset.x + extraOffset.x + gridStep, y + extraOffset.y);
		}
		Draw::end();

		glLineWidth(lineSize);

		/// Draw Rooms
		glEnable(GL_BLEND);
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
		for (Room *room : EditorState::rooms) {
			if (!EditorState::visibleLayers[room->layer]) continue;
			if (!room->data.merge) continue;

			room->drawBlack(worldMouse, lineSize, screenBounds, EditorState::roomPositionType);
		}
		for (Room *room : EditorState::rooms) {
			if (!EditorState::visibleLayers[room->layer]) continue;

			if (!room->data.merge) {
				room->drawBlack(worldMouse, lineSize, screenBounds, EditorState::roomPositionType);
			}

			room->draw(worldMouse, lineSize, screenBounds, EditorState::roomPositionType);
			if (EditorState::window->modifierPressed(GLFW_MOD_ALT)) {
				room->draw(worldMouse, lineSize, screenBounds, (EditorState::roomPositionType == CANON_POSITION) ? DEV_POSITION : CANON_POSITION);
			}
			if (EditorState::selectedRooms.find(room) != EditorState::selectedRooms.end()) {
				setThemeColour(ThemeColour::SelectionBorder);
				Vector2 &roomPosition = room->currentPosition();
				strokeRect(roomPosition.x, roomPosition.y, roomPosition.x + room->Width(), roomPosition.y - room->Height(), 16.0f / lineSize);
			}
		}
		glDisable(GL_BLEND);

		/// Draw Connections
		for (Connection *connection : EditorState::connections) {
			if (!EditorState::visibleLayers[connection->roomA->layer]) continue;
			if (!EditorState::visibleLayers[connection->roomB->layer]) continue;

			connection->draw(worldMouse, lineSize);
		}

		if (connectionStart != nullptr && connectionEnd != nullptr) {
			bool valid = true;

			if (currentConnection->roomA == currentConnection->roomB) valid = false;
			if (currentConnection->roomA == nullptr) valid = false;
			if (currentConnection->roomB == nullptr) valid = false;
			if (currentConnection->roomA != nullptr && currentConnection->roomA->ConnectionUsed(currentConnection->connectionA)) valid = false;
			if (currentConnection->roomB != nullptr && currentConnection->roomB->ConnectionUsed(currentConnection->connectionB)) valid = false;

			if (valid) {
				Draw::color(1.0f, 1.0f, 0.0f);
			} else {
				Draw::color(1.0f, 0.0f, 0.0f);
			}

			Vector2 pointA = connectionStart;
			Vector2 pointB = connectionEnd;

			int segments = int(pointA.distanceTo(pointB) / 2.0);
			segments = clamp(segments, 4, 100);
			double directionStrength = pointA.distanceTo(pointB);
			if (directionStrength > 300.0) directionStrength = (directionStrength - 300.0) * 0.5 + 300.0;

			if (Settings::getSetting<int>(Settings::Setting::ConnectionType) == 0) {
				drawLine(pointA.x, pointA.y, pointB.x, pointB.y, 16.0 / lineSize);
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

					drawLine(lastPoint.x, lastPoint.y, point.x, point.y, 16.0 / lineSize);

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
			strokeRect(selectionStart.x, selectionStart.y, selectionEnd.x, selectionEnd.y, 16.0 / lineSize);
		}

		/// Draw UI
		applyFrustumToOrthographic(Vector2(0.0f, 0.0f), 0.0f, screenBounds);

		if (connectionError != "") {
			Draw::color(1.0, 0.0, 0.0);
			Fonts::rainworld->write(connectionError, EditorState::mouse->X() / 512.0f - screenBounds.x, -EditorState::mouse->Y() / 512.0f + screenBounds.y, 0.05);
		}

		MenuItems::draw(&globalMouseObj, screenBounds);

		Popups::draw(screenMouse, screenBounds);

		DebugData::draw(EditorState::window, worldMouse, lineSize, screenBounds);

		EditorState::window->render();

		Popups::cleanup();

		lastMousePosition.x = EditorState::mouse->X();
		lastMousePosition.y = EditorState::mouse->Y();
	}

	for (Room *room : EditorState::rooms)
		delete room;

	EditorState::rooms.clear();

	for (Connection *connection : EditorState::connections)
		delete connection;

	EditorState::connections.clear();

	Fonts::cleanup();
	MenuItems::cleanup();
	Shaders::cleanup();
	Draw::cleanup();
	Settings::cleanup();

	return 0;
}