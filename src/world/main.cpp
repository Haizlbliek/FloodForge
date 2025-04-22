#include "../gl.h"

#include <iostream>
#include <fstream>
#include <cmath>
#include <vector>
#include <algorithm>

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
#include "../popup/SplashArtPopup.hpp"
#include "../popup/QuitConfirmationPopup.hpp"
#include "SubregionPopup.hpp"
#include "RoomTagPopup.hpp"
#include "DenPopup.hpp"

#include "Shaders.hpp"
#include "Globals.hpp"
#include "Room.hpp"
#include "OffscreenRoom.hpp"
#include "Connection.hpp"
#include "MenuItems.hpp"
#include "DebugData.hpp"

#define TEXTURE_PATH (BASE_PATH + "assets/")

#define clamp(x, a, b) x >= b ? b : (x <= a ? a : x)
#define min(a, b) (a < b) ? a : b
#define max(a, b) (a > b) ? a : b

OffscreenRoom* offscreenDen = nullptr;
std::vector<Room*> rooms;
std::vector<Connection*> connections;
std::vector<std::string> subregions;

Vector2 cameraOffset = Vector2(0.0, 0.0);
double cameraScale = 32.0;
double selectorScale = 1.0;

std::string ROOM_TAGS[9] = { "SHELTER", "ANCIENTSHELTER", "GATE", "SWARMROOM", "PERF_HEAVY", "SCAVOUTPOST", "SCAVTRADER", "NOTRACKERS", "ARENA" };
std::string ROOM_TAG_NAMES[9] = { "Shelter", "Ancient Shelter", "Gate", "Swarm Room", "Performance Heavy", "Scavenger Outpost", "Scavenger Trader", "No Trackers", "Arena (MSC)" };

int roomColours = 0;
bool visibleLayers[] = { true, true, true };
bool visibleDevItems = false;

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
double cameraScaleTo = cameraScale;

Room *holdingRoom = nullptr;
Popup *holdingPopup = nullptr;
Vector2 holdingStart = Vector2(0.0f, 0.0f);
int holdingType = 0;

int selectingState = 0;
Room *roomPossibleSelect = nullptr;
Vector2 selectionStart;
Vector2 selectionEnd;
std::set<Room*> selectedRooms;
int roomSnap = ROOM_SNAP_TILE;

std::string line;

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

Mouse *mouse = nullptr;
Window *window = nullptr;

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
	double scrollY = -window->getMouseScrollY();
	if (isHoveringPopup) scrollY = 0.0;

	if (scrollY < -10.0) scrollY = -10.0;
	double zoom = std::pow(1.25, scrollY);

	Vector2 previousWorldMouse = Vector2(
		screenMouse.x * cameraScale + cameraOffset.x,
		screenMouse.y * cameraScale + cameraOffset.y
	);

	cameraScaleTo *= zoom;
	cameraScale += (cameraScaleTo - cameraScale) * Settings::getSetting<double>(Settings::Setting::CameraZoomSpeed);

	worldMouse = Vector2(
		screenMouse.x * cameraScale + cameraOffset.x,
		screenMouse.y * cameraScale + cameraOffset.y
	);

	cameraOffset.x += previousWorldMouse.x - worldMouse.x;
	cameraOffset.y += previousWorldMouse.y - worldMouse.y;
	cameraPanTo.x += previousWorldMouse.x - worldMouse.x;
	cameraPanTo.y += previousWorldMouse.y - worldMouse.y;

	//// Panning
	if (mouse->Middle()) {
		if (!cameraPanningBlocked && !cameraPanning) {
			if (isHoveringPopup) cameraPanningBlocked = true;

			if (!cameraPanningBlocked) {
				cameraPanStart.x = cameraOffset.x;
				cameraPanStart.y = cameraOffset.y;
				cameraPanStartMouse.x = globalMouse.x;
				cameraPanStartMouse.y = globalMouse.y;
				cameraPanning = true;
			}
		}

		if (cameraPanning && !cameraPanningBlocked) {
			cameraPanTo.x = cameraPanStart.x + cameraScale * (cameraPanStartMouse.x - globalMouse.x) / 512.0;
			cameraPanTo.y = cameraPanStart.y + cameraScale * (cameraPanStartMouse.y - globalMouse.y) / -512.0;
		}
	} else {
		cameraPanning = false;
		cameraPanningBlocked = false;
	}

	cameraOffset.x += (cameraPanTo.x - cameraOffset.x) * Settings::getSetting<double>(Settings::Setting::CameraPanSpeed);
	cameraOffset.y += (cameraPanTo.y - cameraOffset.y) * Settings::getSetting<double>(Settings::Setting::CameraPanSpeed);
}

void updateOriginalControls() {
	if (mouse->Left()) {
		if (!leftMouseDown) {
			for (Popup *popup : Popups::popups) {
				Rect bounds = popup->Bounds();

				if (bounds.inside(screenMouse)) {
					popup->mouseClick(screenMouse.x, screenMouse.y);
					if (popup->drag(screenMouse.x, screenMouse.y)) {
						holdingPopup = popup;
						holdingStart = screenMouse;
					}
					selectingState = 2;
					break;
				}
			}

			if (selectingState == 0) {
				for (auto it = rooms.rbegin(); it != rooms.rend(); it++) {
					Room *room = *it;
					if (!visibleLayers[room->layer]) continue;

					if (room->inside(worldMouse)) {
						holdingRoom = room;
						holdingStart = worldMouse;
						roomPossibleSelect = room;
						selectingState = 3;
						break;
					}
				}
			}

			if (selectingState == 0) {
				if (window->modifierPressed(GLFW_MOD_SHIFT)) {
					selectingState = 1;
					selectionStart = worldMouse;
					selectionEnd = worldMouse;
					if (!window->modifierPressed(GLFW_MOD_CONTROL)) selectedRooms.clear();
				} else {
					selectingState = 5;
					selectionStart = globalMouse;
					selectionEnd = globalMouse;
				}
			}
		} else {
			if (selectingState == 3 && mouseMoved || selectingState == 4) {
				if (selectingState == 3) {
					if (window->modifierPressed(GLFW_MOD_SHIFT) || window->modifierPressed(GLFW_MOD_CONTROL)) {
						selectedRooms.insert(roomPossibleSelect);
					} else {
						if (selectedRooms.find(holdingRoom) == selectedRooms.end()) {
							selectedRooms.clear();
							selectedRooms.insert(roomPossibleSelect);
						}
					}
					rooms.erase(std::remove(rooms.begin(), rooms.end(), roomPossibleSelect), rooms.end());
					rooms.push_back(roomPossibleSelect);
					selectingState = 4;
				}

				Vector2 offset = (worldMouse - holdingStart);
				if (roomSnap == ROOM_SNAP_TILE) offset.round();

				for (Room *room2 : selectedRooms) {
					if (roomSnap == ROOM_SNAP_TILE) {
						room2->Position().round();
					}

					room2->Position().add(offset);
				}
				holdingStart = holdingStart + offset;
			}

			if (holdingPopup != nullptr) {
				holdingPopup->offset(screenMouse - holdingStart);
				holdingStart = screenMouse;
			}

			if (selectingState == 1) {
				selectionEnd = worldMouse;
			}

			if (selectingState == 5) {
				selectionEnd = globalMouse;

				cameraPanTo.x += (selectionStart.x - selectionEnd.x) * cameraScale / 512;
				cameraPanTo.y += (selectionStart.y - selectionEnd.y) * cameraScale / -512;

				selectionStart = selectionEnd;
			}
		}

		leftMouseDown = true;
	} else {
		if (selectingState == 3) {
			rooms.erase(std::remove(rooms.begin(), rooms.end(), roomPossibleSelect), rooms.end());
			rooms.push_back(roomPossibleSelect);
			if (window->modifierPressed(GLFW_MOD_SHIFT) || window->modifierPressed(GLFW_MOD_CONTROL)) {
				if (selectedRooms.find(roomPossibleSelect) != selectedRooms.end()) {
					selectedRooms.erase(roomPossibleSelect);
				} else {
					selectedRooms.insert(roomPossibleSelect);
				}
			} else {
				selectedRooms.clear();
				selectedRooms.insert(roomPossibleSelect);
			}
			holdingType = 1;
			if (roomSnap == ROOM_SNAP_TILE) {
				for (Room *room2 : selectedRooms) {
					room2->Position().x = round(room2->Position().x);
					room2->Position().y = round(room2->Position().y);
				}
			}
		}

		leftMouseDown = false;
		holdingRoom = nullptr;
		holdingPopup = nullptr;

		if (selectingState == 1) {
			for (Room *room : rooms) {
				if (room->intersects(selectionStart, selectionEnd)) selectedRooms.insert(room);
			}
		}
		selectingState = 0;
	}
}

void updateFloodForgeControls() {
	if (mouse->Left()) {
		if (!leftMouseDown) {
			for (Popup *popup : Popups::popups) {
				Rect bounds = popup->Bounds();

				if (bounds.inside(screenMouse)) {
					popup->mouseClick(screenMouse.x, screenMouse.y);
					if (popup->drag(screenMouse.x, screenMouse.y)) {
						holdingPopup = popup;
						holdingStart = screenMouse;
					}
					selectingState = 2;
					break;
				}
			}

			if (selectingState == 0) {
				for (auto it = rooms.rbegin(); it != rooms.rend(); it++) {
					Room *room = *it;
					if (!visibleLayers[room->layer]) continue;

					if (room->inside(worldMouse)) {
						holdingRoom = room;
						holdingStart = worldMouse;
						roomPossibleSelect = room;
						selectingState = 3;
						break;
					}
				}
			}

			if (selectingState == 0) {
				selectingState = 1;
				selectionStart = worldMouse;
				selectionEnd = worldMouse;
				if (!window->modifierPressed(GLFW_MOD_SHIFT) && !window->modifierPressed(GLFW_MOD_CONTROL)) selectedRooms.clear();
			}
		} else {
			if (selectingState == 3 && mouseMoved || selectingState == 4) {
				if (selectingState == 3) {
					if (window->modifierPressed(GLFW_MOD_SHIFT) || window->modifierPressed(GLFW_MOD_CONTROL)) {
						selectedRooms.insert(roomPossibleSelect);
					} else {
						if (selectedRooms.find(holdingRoom) == selectedRooms.end()) {
							selectedRooms.clear();
							selectedRooms.insert(roomPossibleSelect);
						}
					}
					rooms.erase(std::remove(rooms.begin(), rooms.end(), roomPossibleSelect), rooms.end());
					rooms.push_back(roomPossibleSelect);
					selectingState = 4;
				}

				Vector2 offset = (worldMouse - holdingStart);
				if (roomSnap == ROOM_SNAP_TILE) offset.round();

				for (Room *room2 : selectedRooms) {
					if (roomSnap == ROOM_SNAP_TILE) {
						room2->Position().round();
					}

					room2->Position().add(offset);
				}
				holdingStart = holdingStart + offset;
			}

			if (holdingPopup != nullptr) {
				holdingPopup->offset(screenMouse - holdingStart);
				holdingStart = screenMouse;
			}

			if (selectingState == 1) {
				selectionEnd = worldMouse;
				// selectedRooms.clear();
			}
		}

		leftMouseDown = true;
	} else {
		if (selectingState == 3) {
			rooms.erase(std::remove(rooms.begin(), rooms.end(), roomPossibleSelect), rooms.end());
			rooms.push_back(roomPossibleSelect);
			if (window->modifierPressed(GLFW_MOD_SHIFT) || window->modifierPressed(GLFW_MOD_CONTROL)) {
				if (selectedRooms.find(roomPossibleSelect) != selectedRooms.end()) {
					selectedRooms.erase(roomPossibleSelect);
				} else {
					selectedRooms.insert(roomPossibleSelect);
				}
			} else {
				selectedRooms.clear();
				selectedRooms.insert(roomPossibleSelect);
			}
			holdingType = 1;
			if (roomSnap == ROOM_SNAP_TILE) {
				for (Room *room2 : selectedRooms) {
					room2->Position().x = round(room2->Position().x);
					room2->Position().y = round(room2->Position().y);
				}
			}
		}

		leftMouseDown = false;
		holdingRoom = nullptr;
		holdingPopup = nullptr;

		if (selectingState == 1) {
			for (Room *room : rooms) {
				if (room->intersects(selectionStart, selectionEnd)) selectedRooms.insert(room);
			}
		}
		selectingState = 0;
	}
}

void updateMain() {
	updateCamera();
	
	selectorScale = Settings::getSetting<bool>(Settings::Setting::SelectorScale) ? cameraScale / 16.0 : 1.0;

	/// Update Inputs

	if (window->modifierPressed(GLFW_MOD_ALT)) {
		roomSnap = ROOM_SNAP_NONE;
	} else {
		roomSnap = ROOM_SNAP_TILE;
	}

	if (window->keyPressed(GLFW_KEY_F11)) {
		if (previousKeys.find(GLFW_KEY_F11) == previousKeys.end()) {
			window->toggleFullscreen();
		}

		previousKeys.insert(GLFW_KEY_F11);
	} else {
		previousKeys.erase(GLFW_KEY_F11);
	}

	if (window->keyPressed(GLFW_KEY_ESCAPE)) {
		if (previousKeys.find(GLFW_KEY_ESCAPE) == previousKeys.end()) {
			if (Popups::popups.size() > 0)
				Popups::popups[0]->reject();
			else
				Popups::addPopup(new QuitConfirmationPopup(window));
		}

		previousKeys.insert(GLFW_KEY_ESCAPE);
	} else {
		previousKeys.erase(GLFW_KEY_ESCAPE);
	}

	if (window->keyPressed(GLFW_KEY_ENTER)) {
		if (previousKeys.find(GLFW_KEY_ENTER) == previousKeys.end()) {
			if (Popups::popups.size() > 0)
				Popups::popups[0]->accept();
		}

		previousKeys.insert(GLFW_KEY_ENTER);
	} else {
		previousKeys.erase(GLFW_KEY_ENTER);
	}

	//// Connections
	connectionError = "";
	if (mouse->Right()) {
		Room *hoveringRoom = nullptr;
		for (auto it = rooms.rbegin(); it != rooms.rend(); it++) {
			Room *room = (*it);
			if (!visibleLayers[room->layer]) continue;

			if (room->inside(worldMouse)) {
				hoveringRoom = room;
				break;
			}
		}

		Vector2i tilePosition;

		if (hoveringRoom != nullptr) {
			tilePosition = Vector2i(
				floor(worldMouse.x - hoveringRoom->Position().x),
				-1 - floor(worldMouse.y - hoveringRoom->Position().y)
			);
		} else {
			tilePosition = Vector2i(-1, -1);
		}

		if (connectionState == 0) {
			if (connectionStart != nullptr) { delete connectionStart; connectionStart = nullptr; }
			if (connectionEnd   != nullptr) { delete connectionEnd;   connectionEnd   = nullptr; }

			if (hoveringRoom != nullptr) {
				int connectionId = hoveringRoom->getShortcutConnection(tilePosition);

				if (connectionId != -1 && !hoveringRoom->ConnectionUsed(connectionId)) {
					connectionStart = new Vector2(floor(worldMouse.x - hoveringRoom->Position().x) + 0.5 + hoveringRoom->Position().x, floor(worldMouse.y - hoveringRoom->Position().y) + 0.5 + hoveringRoom->Position().y);
					connectionEnd   = new Vector2(connectionStart);
					currentConnection = new Connection(hoveringRoom, connectionId, nullptr, 0);
				}
			}

			connectionState = (connectionStart == nullptr) ? 2 : 1;
		} else if (connectionState == 1) {
			int connectionId = -1;

			if (hoveringRoom != nullptr) {
				connectionId = hoveringRoom->getShortcutConnection(tilePosition);
			}

			if (connectionId != -1) {
				connectionEnd->x = floor(worldMouse.x - hoveringRoom->Position().x) + 0.5 + hoveringRoom->Position().x;
				connectionEnd->y = floor(worldMouse.y - hoveringRoom->Position().y) + 0.5 + hoveringRoom->Position().y;
				currentConnection->RoomB(hoveringRoom);
				currentConnection->ConnectionB(connectionId);

				if (currentConnection->RoomA() == currentConnection->RoomB()) {
					connectionError = "Can't connect to same room";
				} else if (currentConnection->RoomB() != nullptr && currentConnection->RoomB()->ConnectionUsed(currentConnection->ConnectionB())) {
					connectionError = "Already connected";
				} else if (currentConnection->RoomA()->RoomUsed(currentConnection->RoomB()) || currentConnection->RoomB()->RoomUsed(currentConnection->RoomA())) {
					connectionError = "Can't connect to room already connected to";
				}
			} else {
				connectionEnd->x = worldMouse.x;
				connectionEnd->y = worldMouse.y;
				currentConnection->RoomB(nullptr);
				currentConnection->ConnectionB(0);
				connectionError = "Needs to connect";
			}
		}
	} else {
		if (currentConnection != nullptr) {
			bool valid = true;

			if (currentConnection->RoomA() == currentConnection->RoomB()) valid = false;
			if (currentConnection->RoomA() == nullptr) valid = false;
			if (currentConnection->RoomB() == nullptr) valid = false;

			if (currentConnection->RoomA() != nullptr && currentConnection->RoomB() != nullptr) {
				if (currentConnection->RoomA()->ConnectionUsed(currentConnection->ConnectionA())) valid = false;
				if (currentConnection->RoomB()->ConnectionUsed(currentConnection->ConnectionB())) valid = false;
				if (currentConnection->RoomA()->RoomUsed(currentConnection->RoomB())) valid = false;
				if (currentConnection->RoomB()->RoomUsed(currentConnection->RoomA())) valid = false;
			}

			if (valid) {
				connections.push_back(currentConnection);
				currentConnection->RoomA()->connect(currentConnection->RoomB(), currentConnection->ConnectionA());
				currentConnection->RoomB()->connect(currentConnection->RoomA(), currentConnection->ConnectionB());
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

	if (window->keyPressed(GLFW_KEY_I)) {
		if (previousKeys.find(GLFW_KEY_I) == previousKeys.end()) {
			for (auto it = rooms.rbegin(); it != rooms.rend(); it++) {
				Room *room = *it;
				if (!visibleLayers[room->layer]) continue;

				if (room->inside(worldMouse)) {
					rooms.erase(std::remove(rooms.begin(), rooms.end(), room), rooms.end());
					rooms.insert(rooms.begin(), room);
					break;
				}
			}
		}

		previousKeys.insert(GLFW_KEY_I);
	} else {
		previousKeys.erase(GLFW_KEY_I);
	}

	if (window->keyPressed(GLFW_KEY_X)) {
		if (previousKeys.find(GLFW_KEY_X) == previousKeys.end()) {
			bool deleted = false;
			
			for (auto it = connections.rbegin(); it != connections.rend(); it++) {
				Connection *connection = *it;
				if (!visibleLayers[connection->RoomA()->layer]) continue;
				if (!visibleLayers[connection->RoomB()->layer]) continue;

				if (connection->hovered(worldMouse, lineSize)) {
					connections.erase(std::remove(connections.begin(), connections.end(), connection), connections.end());

					connection->RoomA()->disconnect(connection->RoomB(), connection->ConnectionA());
					connection->RoomB()->disconnect(connection->RoomA(), connection->ConnectionB());

					delete connection;

					deleted = true;

					break;
				}
			}

			if (!deleted) {
				Room *hoveredRoom = nullptr;
				for (auto it = rooms.rbegin(); it != rooms.rend(); it++) {
					Room *room = *it;
					if (!visibleLayers[room->layer]) continue;

					if (room->inside(worldMouse)) {
						if (room != offscreenDen) hoveredRoom = room;
						break;
					}
				}

				if (hoveredRoom != nullptr) {
					if (selectedRooms.find(hoveredRoom) != selectedRooms.end()) {
						for (Room *room : selectedRooms) {
							rooms.erase(std::remove(rooms.begin(), rooms.end(), room), rooms.end());

							connections.erase(std::remove_if(connections.begin(), connections.end(),
								[room](Connection *connection) {
									if (connection->RoomA() == room || connection->RoomB() == room) {
										connection->RoomA()->disconnect(connection->RoomB(), connection->ConnectionA());
										connection->RoomB()->disconnect(connection->RoomA(), connection->ConnectionB());

										delete connection;
										return true;
									}

									return false;
								}
							), connections.end());

							delete room;
						}

						selectedRooms.clear();
					} else {
						for (auto it = rooms.rbegin(); it != rooms.rend(); it++) {
							Room *room = *it;
							if (!visibleLayers[room->layer]) continue;

							if (room->inside(worldMouse)) {
								rooms.erase(std::remove(rooms.begin(), rooms.end(), room), rooms.end());

								connections.erase(std::remove_if(connections.begin(), connections.end(),
									[room](Connection *connection) {
										if (connection->RoomA() == room || connection->RoomB() == room) {
											connection->RoomA()->disconnect(connection->RoomB(), connection->ConnectionA());
											connection->RoomB()->disconnect(connection->RoomA(), connection->ConnectionB());

											delete connection;
											return true;
										}

										return false;
									}
								), connections.end());

								delete room;

								break;
							}
						}
					}
				}
			}
		}

		previousKeys.insert(GLFW_KEY_X);
	} else {
		previousKeys.erase(GLFW_KEY_X);
	}

	if (window->keyPressed(GLFW_KEY_S)) {
		if (previousKeys.find(GLFW_KEY_S) == previousKeys.end()) {
			if (selectedRooms.size() >= 1) {
				Popups::addPopup(new SubregionPopup(window, selectedRooms));
			} else {
				for (auto it = rooms.rbegin(); it != rooms.rend(); it++) {
					Room *room = *it;
					if (!visibleLayers[room->layer]) continue;

					if (room->inside(worldMouse)) {
						std::set<Room*> roomGroup;
						roomGroup.insert(room);
						Popups::addPopup(new SubregionPopup(window, roomGroup));

						break;
					}
				}
			}
		}

		previousKeys.insert(GLFW_KEY_S);
	} else {
		previousKeys.erase(GLFW_KEY_S);
	}

	if (window->keyPressed(GLFW_KEY_T)) {
		if (previousKeys.find(GLFW_KEY_T) == previousKeys.end()) {
			if (selectedRooms.size() >= 1) {
				Popups::addPopup(new RoomTagPopup(window, selectedRooms));
			} else {
				for (auto it = rooms.rbegin(); it != rooms.rend(); it++) {
					Room *room = *it;
					if (!visibleLayers[room->layer]) continue;

					if (room->inside(worldMouse)) {
						if (room->isOffscreen()) break;

						std::set<Room*> roomGroup;
						roomGroup.insert(room);
						Popups::addPopup(new RoomTagPopup(window, roomGroup));

						break;
					}
				}
			}
		}

		previousKeys.insert(GLFW_KEY_T);
	} else {
		previousKeys.erase(GLFW_KEY_T);
	}

	if (window->keyPressed(GLFW_KEY_L)) {
		if (previousKeys.find(GLFW_KEY_L) == previousKeys.end()) {
			if (selectedRooms.size() > 0) {
				int minimumLayer = 3;

				for (Room *room : selectedRooms)
					minimumLayer = min(minimumLayer, room->layer);

				minimumLayer = transitionLayer(minimumLayer);

				for (Room *room : selectedRooms)
					room->layer = minimumLayer;

			} else {
				Room *hoveringRoom = nullptr;
				for (auto it = rooms.rbegin(); it != rooms.rend(); it++) {
					Room *room = (*it);
					if (!visibleLayers[room->layer]) continue;

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

		previousKeys.insert(GLFW_KEY_L);
	} else {
		previousKeys.erase(GLFW_KEY_L);
	}
	
	if (window->keyPressed(GLFW_KEY_G)) {
		if (previousKeys.find(GLFW_KEY_G) == previousKeys.end()) {
			if (selectedRooms.size() > 0) {
				bool setMerge = true;
				
				for (Room *room : selectedRooms)
					if (room->data.merge) { setMerge = false; break; }
				
				for (Room *room : selectedRooms)
					room->data.merge = setMerge;
			} else {
				Room *hoveringRoom = nullptr;
				for (auto it = rooms.rbegin(); it != rooms.rend(); it++) {
					Room *room = (*it);

					if (!visibleLayers[room->layer]) continue;

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
		
		previousKeys.insert(GLFW_KEY_G);
	} else {
		previousKeys.erase(GLFW_KEY_G);
	}

	if (window->keyPressed(GLFW_KEY_H)) {
		if (previousKeys.find(GLFW_KEY_H) == previousKeys.end()) {
			if (selectedRooms.size() > 0) {
				bool setHidden = true;

				for (Room *room : selectedRooms)
					if (room->data.hidden) { setHidden = false; break; }

				for (Room *room : selectedRooms)
					room->data.hidden = setHidden;

			} else {
				Room *hoveringRoom = nullptr;
				for (auto it = rooms.rbegin(); it != rooms.rend(); it++) {
					Room *room = (*it);

					if (!visibleLayers[room->layer]) continue;

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

		previousKeys.insert(GLFW_KEY_H);
	} else {
		previousKeys.erase(GLFW_KEY_H);
	}

	if (window->keyPressed(GLFW_KEY_C)) {
		bool found = false;

		if (previousKeys.find(GLFW_KEY_C) == previousKeys.end()) {
			for (auto it = rooms.rbegin(); it != rooms.rend(); it++) {
				Room *room = *it;
				
				Vector2 roomMouse = worldMouse - room->Position();
				Vector2 shortcutPosition;
				
				if (room->isOffscreen()) {
					for (int i = 0; i < room->DenCount(); i++) {
						shortcutPosition = Vector2(room->Width() * 0.5 - room->DenCount() * 2.0 + i * 4.0 + 2.5, -room->Height() * 0.25 - 0.5);
						
						if (roomMouse.distanceTo(shortcutPosition) < selectorScale) {
							Popups::addPopup(new DenPopup(window, room, i));
			
							found = true;
							break;
						}
					}
				} else {
					for (Vector2i shortcut : room->DenEntrances()) {
						shortcutPosition = Vector2(shortcut.x + 0.5, -1 - shortcut.y + 0.5);
						
						if (roomMouse.distanceTo(shortcutPosition) < selectorScale) {
							Popups::addPopup(new DenPopup(window, room, room->DenId(shortcut)));
			
							found = true;
							break;
						}
					}
				}
				
				if (found) break;
			}
			
			if (!found) {
				for (auto it = rooms.rbegin(); it != rooms.rend(); it++) {
					Room *room = *it;
					
					if (room->inside(worldMouse)) {
						if (!room->isOffscreen()) break;
						
						Popups::addPopup(new DenPopup(window, room, offscreenDen->AddDen()));
					}
				}
			}
		}

		previousKeys.insert(GLFW_KEY_C);
	} else {
		previousKeys.erase(GLFW_KEY_C);
	}

	bool found = false;
	for (auto it = rooms.rbegin(); it != rooms.rend(); it++) {
		Room *room = *it;
		room->hoveredDen = -1;
		
		Vector2 roomMouse = worldMouse - room->Position();
		Vector2 shortcutPosition;
		
		if (room->isOffscreen()) {
			for (int i = 0; i < room->DenCount(); i++) {
				shortcutPosition = Vector2(room->Width() * 0.5 - room->DenCount() * 2.0 + i * 4.0 + 2.5, -room->Height() * 0.25 - 0.5);
				
				if (roomMouse.distanceTo(shortcutPosition) < selectorScale) {
					room->hoveredDen = i;
	
					found = true;
					break;
				}
			}
		} else {
			for (Vector2i shortcut : room->DenEntrances()) {
				shortcutPosition = Vector2(shortcut.x + 0.5, -1 - shortcut.y + 0.5);
				
				if (roomMouse.distanceTo(shortcutPosition) < selectorScale) {
					room->hoveredDen = room->DenId(shortcut) - room->ConnectionCount();
	
					found = true;
					break;
				}
			}
		}
		
		if (found) break;
	}
	
	if (!Popups::hasPopup("DenPopup") && offscreenDen != nullptr) {
		offscreenDen->cleanup();
	}
}

int main() {
	window = new Window(1024, 1024);
	window->setIcon(TEXTURE_PATH + "MainIcon.png");
	window->setTitle("FloodForge World Editor");
	mouse = window->GetMouse();

	if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
		std::cerr << "Failed to initialize GLAD!" << std::endl;
		return -1;
	}

	Settings::init();
	Fonts::init();
	MenuItems::init(window);
	Popups::init();
	Shaders::init();
	Draw::init();
	CreatureTextures::init();

	Popups::addPopup(new SplashArtPopup(window));

	while (window->isOpen()) {
		mouse->updateLastPressed();
		glfwPollEvents();

		window->ensureFullscreen();

		int width;
		int height;
		glfwGetWindowSize(window->getGLFWWindow(), &width, &height);
		if (width == 0 || height == 0) continue;
		float size = min(width, height);
		float offsetX = (width * 0.5) - size * 0.5;
		float offsetY = (height * 0.5) - size * 0.5;

		mouseMoved = (mouse->X() != lastMousePosition.x || mouse->Y() != lastMousePosition.y);
		
		globalMouse = Vector2(
			(mouse->X() - offsetX) / size * 1024,
			(mouse->Y() - offsetY) / size * 1024
		);
		screenMouse = Vector2(
			(globalMouse.x / 1024.0) *  2.0 - 1.0,
			(globalMouse.y / 1024.0) * -2.0 + 1.0
		);

		Mouse globalMouseObj = Mouse(window->getGLFWWindow(), globalMouse.x, globalMouse.y);
		globalMouseObj.copyPressed(*mouse);

		lineSize = 64.0 / cameraScale;

		// Update

		updateMain();


		// Draw

		glViewport(0, 0, width, height);

		window->clear();
		glDisable(GL_DEPTH_TEST);

		// Draw::color(0.0f, 0.0f, 0.0f);
		// fillRect(-1.0, -1.0, 1.0, 1.0);

		// glViewport(offsetX, offsetY, size, size);

		setThemeColour(ThemeColour::Background);
		Vector2 screenBounds = Vector2(width, height) / size;
		fillRect(-screenBounds.x, -screenBounds.y, screenBounds.x, screenBounds.y);

		applyFrustumToOrthographic(cameraOffset, 0.0f, cameraScale * screenBounds);

		/// Draw Grid
		glLineWidth(1);
		setThemeColor(ThemeColour::Grid);
		double gridStep = max(cameraScale / 16.0, 1.0);
		gridStep = std::pow(2, std::ceil(std::log2(gridStep - 0.01)));
		Draw::begin(Draw::LINES);
		Vector2 offset = (cameraOffset / gridStep).rounded() * gridStep;
		Vector2 extraOffset = Vector2(fmod((screenBounds.x - 1.0) * gridStep * 16.0, gridStep), 0);
		Vector2 gridScale = gridStep * 16.0 * screenBounds;
		for (float x = -gridScale.x + offset.x; x < gridScale.x + offset.x; x += gridStep) {
			Draw::vertex(x + extraOffset.x, -cameraScale * screenBounds.y + offset.y + extraOffset.y - gridStep);
			Draw::vertex(x + extraOffset.x,  cameraScale * screenBounds.y + offset.y + extraOffset.y + gridStep);
		}
		for (float y = -gridScale.y + offset.y; y < gridScale.y + offset.y; y += gridStep) {
			Draw::vertex(-cameraScale * screenBounds.x + offset.x + extraOffset.x - gridStep, y + extraOffset.y);
			Draw::vertex( cameraScale * screenBounds.x + offset.x + extraOffset.x + gridStep, y + extraOffset.y);
		}
		Draw::end();
		
		glLineWidth(lineSize);

		/// Draw Rooms
		glEnable(GL_BLEND);
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
		for (Room *room : rooms) {
			if (!visibleLayers[room->layer]) continue;
			if (!room->data.merge) continue;

			room->drawBlack(worldMouse, lineSize, screenBounds);
		}
		for (Room *room : rooms) {
			if (!visibleLayers[room->layer]) continue;
			
			if (!room->data.merge) {
				room->drawBlack(worldMouse, lineSize, screenBounds);
			}

			room->draw(worldMouse, lineSize, screenBounds);
			if (selectedRooms.find(room) != selectedRooms.end()) {
				setThemeColour(ThemeColour::SelectionBorder);
				strokeRect(room->Position().x, room->Position().y, room->Position().x + room->Width(), room->Position().y - room->Height(), 16.0f / lineSize);
			}
		}
		glDisable(GL_BLEND);

		/// Draw Connections
		for (Connection *connection : connections) {
			if (!visibleLayers[connection->RoomA()->layer]) continue;
			if (!visibleLayers[connection->RoomB()->layer]) continue;

			connection->draw(worldMouse, lineSize);
		}

		if (connectionStart != nullptr && connectionEnd != nullptr) {
			bool valid = true;

			if (currentConnection->RoomA() == currentConnection->RoomB()) valid = false;
			if (currentConnection->RoomA() == nullptr) valid = false;
			if (currentConnection->RoomB() == nullptr) valid = false;
			if (currentConnection->RoomA() != nullptr && currentConnection->RoomA()->ConnectionUsed(currentConnection->ConnectionA())) valid = false;
			if (currentConnection->RoomB() != nullptr && currentConnection->RoomB()->ConnectionUsed(currentConnection->ConnectionB())) valid = false;

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
				Vector2 directionA = currentConnection->RoomA()->getShortcutDirectionVector(currentConnection->ConnectionA());
				Vector2 directionB = Vector2(0, 0);

				if (currentConnection->RoomB() != nullptr) directionB = currentConnection->RoomB()->getShortcutDirectionVector(currentConnection->ConnectionB());

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
			// drawLine(connectionStart->x, connectionStart->y, connectionEnd->x, connectionEnd->y, 16.0 / lineSize);
		}

		if (selectingState == 1) {
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
			Fonts::rainworld->write(connectionError, mouse->X() / 512.0f - screenBounds.x, -mouse->Y() / 512.0f + screenBounds.y, 0.05);
		}

		MenuItems::draw(&globalMouseObj, screenBounds);

		Popups::draw(screenMouse, screenBounds);

		DebugData::draw(window, worldMouse, lineSize, screenBounds);

		window->render();

		Popups::cleanup();
		
		lastMousePosition.x = mouse->X();
		lastMousePosition.y = mouse->Y();
	}

	for (Room *room : rooms)
		delete room;

	rooms.clear();

	for (Connection *connection : connections)
		delete connection;

	connections.clear();

	Fonts::cleanup();
	MenuItems::cleanup();
	Shaders::cleanup();
	Draw::cleanup();
	Settings::cleanup();

	return 0;
}