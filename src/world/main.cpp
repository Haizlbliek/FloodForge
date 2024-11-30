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

#include "Shaders.hpp"
#include "Globals.hpp"
#include "Room.hpp"
#include "OffscreenRoom.hpp"
#include "Connection.hpp"
#include "MenuItems.hpp"
#include "Popups.hpp"
#include "SplashArtPopup.hpp"
#include "QuitConfirmationPopup.hpp"
#include "SubregionPopup.hpp"

#define TEXTURE_PATH (BASE_PATH + "assets/")

#define clamp(x, a, b) x >= b ? b : (x <= a ? a : x)
#define min(a, b) (a < b) ? a : b
#define max(a, b) (a > b) ? a : b

std::vector<Room*> rooms;
std::vector<Connection*> connections;
std::vector<std::string> subregions;

bool debugRoomConnections = false;

int roomColours = 0;

void applyFrustumToOrthographic(Vector2 position, float rotation, Vector2 scale, float left, float right, float bottom, float top, float nearVal, float farVal) {
	left *= scale.x;
	right *= scale.x;
	bottom *= scale.y;
	top *= scale.y;

	left += position.x;
	right += position.x;
	bottom += position.y;
	top += position.y;

	float cosRot = std::cos(rotation);
	float sinRot = std::sin(rotation);

	GLfloat rotationMatrix[16] = {
		cosRot,  sinRot, 0, 0,
		-sinRot, cosRot, 0, 0,
		0,       0,      1, 0,
		0,       0,      0, 1
	};

	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	glOrtho(left, right, bottom, top, nearVal, farVal);

	glMultMatrixf(rotationMatrix);
}

void applyFrustumToOrthographic(Vector2 position, float rotation, Vector2 scale) {
	applyFrustumToOrthographic(position, rotation, scale, -1.0f, 1.0f, -1.0f, 1.0f, 0.000f, 100.0f);
}

int transitionLayer(int layer) {
	return (layer + 1) % 3;
}

int main() {
	loadTheme();
	
	Window *window = new Window(1024, 1024);
	window->setIcon(TEXTURE_PATH + "MainIcon.png");
	window->setTitle("FloodForge World Editor");

	if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
		std::cerr << "Failed to initialize GLAD!" << std::endl;
		return -1;
	}

	Fonts::init();
	MenuItems::init(window);
	Popups::init();
	Shaders::init();

	addPopup(new SplashArtPopup(window));

	bool leftMouseDown;

	std::set<int> previousKeys;

	Vector2 cameraOffset = Vector2(0.0f, 0.0f);
	Vector2 cameraScale = Vector2(32.0f, 32.0f);
	bool cameraPanning = false;
	bool cameraPanningBlocked = false;
	Vector2 cameraPanStartMouse = Vector2(0.0f, 0.0f);
	Vector2 cameraPanStart = Vector2(0.0f, 0.0f);

	Room *holdingRoom = nullptr;
	Vector2 holdingStart = Vector2(0.0f, 0.0f);


	std::string line;

	bool shownLayers[3] = { true, true, true };

	// rooms.push_back(new OffscreenRoom("exampleroom", "Example Room"));

	Vector2 *connectionStart = nullptr;
	Vector2 *connectionEnd = nullptr;
	Connection *currentConnection = nullptr;
	std::string connectionError = "";

	int connectionState = 0;

	while (window->isOpen()) {
		glfwPollEvents();

		window->ensureFullscreen();

		int width;
		int height;
		glfwGetWindowSize(window->getGLFWWindow(), &width, &height);
		float size = min(width, height);
		float offsetX = (width * 0.5) - size * 0.5;
		float offsetY = (height * 0.5) - size * 0.5;

		Mouse *mouse = window->GetMouse();
		
		double globalMouseX = (mouse->X() - offsetX) / size * 1024;
		double globalMouseY = (mouse->Y() - offsetY) / size * 1024;
		double screenMouseX = (globalMouseX / 1024.0) *  2.0 - 1.0;
		double screenMouseY = (globalMouseY / 1024.0) * -2.0 + 1.0;

		Mouse customMouse = Mouse(window->getGLFWWindow(), globalMouseX, globalMouseY);


		// Update

		bool isHoveringPopup = false;
		for (Popup *popup : popups) {
			Rect bounds = popup->Bounds();

			if (bounds.inside(Vector2(screenMouseX, screenMouseY))) {
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

		double previousScreenMouseX = (globalMouseX / 1024.0) *  2.0 - 1.0;
		double previousScreenMouseY = (globalMouseY / 1024.0) * -2.0 + 1.0;

		Vector2 previousWorldMouse = Vector2(
			previousScreenMouseX * cameraScale.x + cameraOffset.x,
			previousScreenMouseY * cameraScale.y + cameraOffset.y
		);

		cameraScale.x *= zoom;
		cameraScale.y *= zoom;

		double globalMouseX2 = (mouse->X() - offsetX) / size * 1024;
		double globalMouseY2 = (mouse->Y() - offsetY) / size * 1024;

		Vector2 worldMouse = Vector2(
			screenMouseX * cameraScale.x + cameraOffset.x,
			screenMouseY * cameraScale.y + cameraOffset.y
		);

		Vector2 oldCameraOffset = cameraOffset;
		cameraOffset.x += previousWorldMouse.x - worldMouse.x;
		cameraOffset.y += previousWorldMouse.y - worldMouse.y;

		if (std::isnan(cameraOffset.x) || std::isnan(cameraOffset.y)) {
			cameraOffset.x = oldCameraOffset.x;
			cameraOffset.y = oldCameraOffset.y;
		}

		//// Panning
		if (mouse->Middle()) {
			if (!cameraPanningBlocked && !cameraPanning) {
				if (isHoveringPopup) cameraPanningBlocked = true;

				if (!cameraPanningBlocked) {
					cameraPanStart.x = cameraOffset.x;
					cameraPanStart.y = cameraOffset.y;
					cameraPanStartMouse.x = globalMouseX;
					cameraPanStartMouse.y = globalMouseY;
					cameraPanning = true;
				}
			}

			if (cameraPanning && !cameraPanningBlocked) {
				cameraOffset.x = cameraPanStart.x + cameraScale.x * (cameraPanStartMouse.x - globalMouseX) / 512.0;
				cameraOffset.y = cameraPanStart.y + cameraScale.y * (cameraPanStartMouse.y - globalMouseY) / -512.0;
			}
		} else {
			cameraPanning = false;
			cameraPanningBlocked = false;
		}


		/// Update Inputs

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
				if (popups.size() > 0)
					popups[0]->reject();
				else
					addPopup(new QuitConfirmationPopup(window));
			}

			previousKeys.insert(GLFW_KEY_ESCAPE);
		} else {
			previousKeys.erase(GLFW_KEY_ESCAPE);
		}

		if (window->keyPressed(GLFW_KEY_ENTER)) {
			if (previousKeys.find(GLFW_KEY_ENTER) == previousKeys.end()) {
				if (popups.size() > 0)
					popups[0]->accept();
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

				if (room->inside(worldMouse)) {
					hoveringRoom = room;
					break;
				}
			}

			Vector2i tilePosition;

			if (hoveringRoom != nullptr) {
				tilePosition = Vector2i(
					floor(worldMouse.x - hoveringRoom->Position()->x),
					-1 - floor(worldMouse.y - hoveringRoom->Position()->y)
				);
			} else {
				tilePosition = Vector2i(-1, -1);
			}

			if (connectionState == 0) {
				if (connectionStart != nullptr) { delete connectionStart; connectionStart = nullptr; }
				if (connectionEnd   != nullptr) { delete connectionEnd;   connectionEnd   = nullptr; }

				if (hoveringRoom != nullptr) {
					bool valid = false;
					int connectionId = 0;

					for (Vector2i connectionPosition : hoveringRoom->TileConnections()) {
						if (hoveringRoom->ConnectionUsed(connectionId)) { connectionId++; continue; }

						if (connectionPosition == tilePosition) {
							valid = true;
							break;
						}

						connectionId++;
					}

					if (valid) {
						connectionStart = new Vector2(floor(worldMouse.x - hoveringRoom->Position()->x) + 0.5 + hoveringRoom->Position()->x, floor(worldMouse.y - hoveringRoom->Position()->y) + 0.5 + hoveringRoom->Position()->y);
						connectionEnd   = new Vector2(connectionStart);
						currentConnection = new Connection(hoveringRoom, connectionId, nullptr, 0);
					}
				}

				connectionState = (connectionStart == nullptr) ? 2 : 1;
			} else if (connectionState == 1) {
				bool snap = false;
				int connectionId = 0;

				if (hoveringRoom != nullptr) {
					for (Vector2i connectionPosition : hoveringRoom->TileConnections()) {
						if (connectionPosition == tilePosition) {
							snap = true;
							break;
						}

						connectionId++;
					}
				}

				if (snap) {
					connectionEnd->x = floor(worldMouse.x - hoveringRoom->Position()->x) + 0.5 + hoveringRoom->Position()->x;
					connectionEnd->y = floor(worldMouse.y - hoveringRoom->Position()->y) + 0.5 + hoveringRoom->Position()->y;
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

		//// Popups
		if (mouse->Left()) {
			if (!leftMouseDown) {
				bool blockLeftMouse = false;

				for (Popup *popup : popups) {
					Rect bounds = popup->Bounds();

					if (bounds.inside(Vector2(screenMouseX, screenMouseY))) {
						popup->mouseClick(screenMouseX, screenMouseY);
						blockLeftMouse = true;
					}
				}

				if (!blockLeftMouse) {
					for (auto it = rooms.rbegin(); it != rooms.rend(); it++) {
						Room *room = *it;

						if (room->inside(worldMouse)) {
							rooms.erase(std::remove(rooms.begin(), rooms.end(), room), rooms.end());
							rooms.push_back(room);
							holdingRoom = room;
							holdingStart = worldMouse;
							break;
						}
					}
				}
			} else {
				if (holdingRoom != nullptr) {
					holdingRoom->Position()->add(worldMouse - holdingStart);
					holdingStart = worldMouse;
				}
			}

			leftMouseDown = true;
		} else {
			leftMouseDown = false;
			holdingRoom = nullptr;
		}

		if (window->keyPressed(GLFW_KEY_X)) {
			if (previousKeys.find(GLFW_KEY_X) == previousKeys.end()) {
				bool deleted = false;
				for (auto it = connections.rbegin(); it != connections.rend(); it++) {
					Connection *connection = *it;

					if (connection->distance(worldMouse) < 0.5) {
						connections.erase(std::remove(connections.begin(), connections.end(), connection), connections.end());

						connection->RoomA()->disconnect(connection->RoomB(), connection->ConnectionA());
						connection->RoomB()->disconnect(connection->RoomA(), connection->ConnectionB());

						delete connection;

						deleted = true;

						break;
					}
				}

				if (!deleted) {
					for (auto it = rooms.rbegin(); it != rooms.rend(); it++) {
						Room *room = *it;

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

							deleted = true;

							break;
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
				for (auto it = rooms.rbegin(); it != rooms.rend(); it++) {
					Room *room = *it;

					if (room->inside(worldMouse)) {
						addPopup(new SubregionPopup(window, room));

						break;
					}
				}
			}

			previousKeys.insert(GLFW_KEY_S);
		} else {
			previousKeys.erase(GLFW_KEY_S);
		}

		if (window->keyPressed(GLFW_KEY_L)) {
			if (previousKeys.find(GLFW_KEY_L) == previousKeys.end()) {
				Room *hoveringRoom = nullptr;
				for (auto it = rooms.rbegin(); it != rooms.rend(); it++) {
					Room *room = (*it);

					if (room->inside(worldMouse)) {
						hoveringRoom = room;
						break;
					}
				}

				if (hoveringRoom != nullptr) {
					hoveringRoom->Layer(transitionLayer(hoveringRoom->Layer()));
				}
			}

			previousKeys.insert(GLFW_KEY_L);
		} else {
			previousKeys.erase(GLFW_KEY_L);
		}


		if (window->keyPressed(GLFW_KEY_D)) {
			if (previousKeys.find(GLFW_KEY_D) == previousKeys.end()) {
				Connection *hoveringConnection = nullptr;
				for (auto it = connections.rbegin(); it != connections.rend(); it++) {
					Connection *connection = *it;

					if (connection->distance(worldMouse) < 0.5) {
						hoveringConnection = connection;

						break;
					}
				}

				if (hoveringConnection != nullptr) {
					std::cout << "Debugging connection:" << std::endl;
					std::cout << "\tRoom A: " << hoveringConnection->RoomA()->RoomName() << std::endl;
					std::cout << "\tRoom B: " << hoveringConnection->RoomB()->RoomName() << std::endl;
					std::cout << "\tConnection A: " << hoveringConnection->ConnectionA() << std::endl;
					std::cout << "\tConnection B: " << hoveringConnection->ConnectionB() << std::endl;
				} else {
					Room *hoveringRoom = nullptr;
					for (auto it = rooms.rbegin(); it != rooms.rend(); it++) {
						Room *room = (*it);

						if (room->inside(worldMouse)) {
							hoveringRoom = room;
							break;
						}
					}

					if (hoveringRoom != nullptr) {
						std::cout << "Debugging room:" << std::endl;
						std::cout << "\tName: " << hoveringRoom->RoomName() << std::endl;
						std::cout << "\tWidth: " << hoveringRoom->Width() << std::endl;
						std::cout << "\tHeight: " << hoveringRoom->Height() << std::endl;
						std::cout << "\tLayer: " << hoveringRoom->Layer() << std::endl;
						std::cout << "\tConnections: " << std::endl;
						int connectionId = 0;
						for (Vector2i enterance : hoveringRoom->ShortcutEntrances()) {
							std::cout << "\t\t" << connectionId << ": " << enterance << " - " << (hoveringRoom->ConnectionUsed(connectionId) ? "Used" : "Not Used") << std::endl;
							connectionId++;
						}
					} else {
						std::cout << "Debug:" << std::endl;
						std::cout << "\tCam Scale: " << cameraScale.x << std::endl;
						std::cout << "\tCam Position X: " << cameraOffset.x << std::endl;
						std::cout << "\tCam Position Y: " << cameraOffset.y << std::endl;
					}
				}
			}

			previousKeys.insert(GLFW_KEY_D);
		} else {
			previousKeys.erase(GLFW_KEY_D);
		}


		// Draw

		glViewport(0, 0, width, height);

		window->clear();
		glDisable(GL_DEPTH_TEST);

		glColor3f(0.0f, 0.0f, 0.0f);
		fillRect(-1.0, -1.0, 1.0, 1.0);

		glViewport(offsetX, offsetY, size, size);

		setThemeColour(THEME_BACKGROUND_COLOUR);
		// glColor3f(0.3f, 0.3f, 0.3f);
		fillRect(-1.0, -1.0, 1.0, 1.0);

		applyFrustumToOrthographic(cameraOffset, 0.0f, cameraScale);

		double lineSize = 64.0 / cameraScale.x;
		glLineWidth(lineSize);

		/// Draw Rooms

		glEnable(GL_BLEND);
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
		for (int layerId = 2; layerId >= 0; layerId--) {
			if (!shownLayers[layerId]) continue;

			for (Room *room : rooms) {
				if (room->Layer() == layerId) room->draw(worldMouse, lineSize);
			}
		}
		glDisable(GL_BLEND);

		/// Draw Connections
		for (Connection *connection : connections)
			connection->draw(worldMouse, lineSize);

		if (connectionStart != nullptr && connectionEnd != nullptr) {
			bool valid = true;

			if (currentConnection->RoomA() == currentConnection->RoomB()) valid = false;
			if (currentConnection->RoomA() == nullptr) valid = false;
			if (currentConnection->RoomB() == nullptr) valid = false;
			if (currentConnection->RoomA() != nullptr && currentConnection->RoomA()->ConnectionUsed(currentConnection->ConnectionA())) valid = false;
			if (currentConnection->RoomB() != nullptr && currentConnection->RoomB()->ConnectionUsed(currentConnection->ConnectionB())) valid = false;

			if (valid) {
				glColor3f(1.0f, 1.0f, 0.0f);
			} else {
				glColor3f(1.0f, 0.0f, 0.0f);
			}

			glColor3f(1.0f, 1.0f, 0.0f);
			drawLine(connectionStart->x, connectionStart->y, connectionEnd->x, connectionEnd->y, 8.0);
		}

		/// Draw UI
		applyFrustumToOrthographic(Vector2(0.0f, 0.0f), 0.0f, Vector2(1.0f, 1.0f));

		if (connectionError != "") {
			glColor3f(1.0, 0.0, 0.0);
			Fonts::rainworld->write(connectionError, mouse->X() / 512.0f - 1.0f, -mouse->Y() / 512.0f + 1.0f, 0.05);
		}

		MenuItems::draw(&customMouse);

		for (Popup *popup : popups) {
			Rect bounds = popup->Bounds();
			bool mouseInside = bounds.inside(Vector2(screenMouseX, screenMouseY));

			popup->draw(screenMouseX, screenMouseY, mouseInside);
		}

		window->render();

		Popups::cleanup();
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

	return 0;
}