#pragma once

#include "../gl.h"

#include <random>
#include <string>
#include <fstream>
#include <sstream>
#include <vector>
#include <iostream>
#include <stack>
#include <set>
#include <algorithm>

#include "../Texture.hpp"
#include "../Theme.hpp"
#include "../Settings.hpp"
#include "../math/Vector.hpp"
#include "../math/Matrix4.hpp"
#include "../font/Fonts.hpp"

#include "FailureController.hpp"
#include "ExtraRoomData.hpp"
#include "Shaders.hpp"
#include "Den.hpp"

#define CONNECTION_TYPE_NONE 0
#define CONNECTION_TYPE_EXIT 1
#define CONNECTION_TYPE_DEN  2
#define CONNECTION_TYPE_MOLE 3
#define CONNECTION_TYPE_SCAV 4

enum ShortcutType {
	ROOM,
	DEN
};

struct Vertex {
	float x, y;
	float r, g, b, a;
};

namespace RoomHelpers {
	void drawTexture(GLuint texture, double rectX, double rectY, double scale);
};

class Room {
	public:
		virtual bool isOffscreen() { return false; }

		Room(std::string path, std::string name) {
			this->path = path;
			this->roomName = toLower(name);

			position = new Vector2(
				0.0f,
				0.0f
			);

			width = 1;
			height = 1;

			valid = false;

			geometry = nullptr;

			layer = 0;
			water = -1;
			subregion = -1;

			images = 0;
			data = ExtraRoomData();

			loadGeometry();
			generateVBO();
			checkImages();
		}

		virtual ~Room() {
			if (geometry != nullptr) delete[] geometry;

			geometry = nullptr;

			glDeleteBuffers(2, vbo);
			glDeleteVertexArrays(1, &vao);
		}
		

		virtual void drawBlack(Vector2 mousePosition, double lineSize, Vector2 screenBounds);
		virtual void draw(Vector2 mousePosition, double lineSize, Vector2 screenBounds);
		

		bool inside(Vector2 otherPosition) {
			return (
				otherPosition.x >= position.x &&
				otherPosition.y >= position.y - height &&
				otherPosition.x <= position.x + width &&
				otherPosition.y <= position.y
			);
		}

		bool intersects(Vector2 corner0, Vector2 corner1) {
			Vector2 cornerMin = Vector2::min(corner0, corner1);
			Vector2 cornerMax = Vector2::max(corner0, corner1);

			return (
				cornerMin.x <= position.x + width &&
				cornerMin.y <= position.y &&
				cornerMax.x >= position.x &&
				cornerMax.y >= position.y - height
			);
		}

		int getTile(int x, int y) const {
			if (!valid) return 1;

			if (x < 0 || y < 0) return 1;
			if (x >= width || y >= height) return 1;

			return geometry[x * height + y];
		}

		bool tileIsShortcut(int x, int y) const {
			int tile = getTile(x, y);
			
			return (tile & (256 | 128 | 64)) > 0;
		}
		
		void Position(Vector2 position) {
			this->position.x = position.x;
			this->position.y = position.y;
		}

		Vector2 &Position() {
			return position;
		}

		const std::vector<Vector2> ShortcutEntranceOffsetPositions() const {
			std::vector<Vector2> transformedEntrances;

			for (const std::pair<Vector2i, int> &connection : shortcutEntrances) {
				transformedEntrances.push_back(Vector2(
					position.x + connection.first.x + 0.5,
					position.y - connection.first.y - 0.5
				));
			}

			return transformedEntrances;
		}

		int getShortcutEntranceId(const Vector2i &searchPosition) const {
			unsigned int connectionId = 0;

			for (const std::pair<Vector2i, int> &connection : shortcutEntrances) {
				if (connection.first == searchPosition) {
					return connectionId;
				}

				connectionId++;
			}

			return -1;
		}

		const Vector2 getRoomEntranceOffsetPosition(unsigned int connectionId) const {
			Vector2i connection = getRoomEntrancePosition(connectionId);

			return Vector2(
				position.x + connection.x + 0.5,
				position.y - connection.y - 0.5
			);
		}

		int getRoomEntranceId(const Vector2i &searchPosition) const {
			int index = 0;
			for (const Vector2i enterance : roomEntrances) {
				if (enterance == searchPosition) {
					return index;
				}

				index++;
			}

			return -1;
		}

		const Vector2i getRoomEntrancePosition(unsigned int connectionId) const {
			if (connectionId >= roomEntrances.size()) return Vector2i(-1, -1);

			return roomEntrances[connectionId];
		}

		Vector2 getRoomEntranceDirectionVector(unsigned int connectionId) const {
			return MathUtils::directionToVector(getRoomEntranceDirection(connectionId));
		}

		Direction getRoomEntranceDirection(unsigned int connectionId) const {
			Vector2i connection = roomEntrances[connectionId];

			if (tileIsShortcut(connection.x - 1, connection.y))
				return Direction::LEFT;
			else if (tileIsShortcut(connection.x, connection.y + 1))
				return Direction::DOWN;
			else if (tileIsShortcut(connection.x + 1, connection.y))
				return Direction::RIGHT;
			else if (tileIsShortcut(connection.x, connection.y - 1))
				return Direction::UP;

			return UNKNOWN;
		}
		
		bool canConnect(unsigned int connectionId) {
			if (shortcutEntrances.size() <= connectionId) return false;
			
			return true;
		}

		void connect(Room *room, unsigned int connectionId) {
			roomConnections.insert(std::pair<Room*, unsigned int> { room, connectionId });
		}

		void disconnect(Room *room, unsigned int connectionId) {
			roomConnections.erase(std::pair<Room*, unsigned int> { room, connectionId });
		}

		bool Connected(Room *room, unsigned int connectionId) const {
			return roomConnections.find(std::pair<Room*, unsigned int> { room, connectionId }) != roomConnections.end();
		}

		bool RoomUsed(Room *room) const {
			for (std::pair<Room*, unsigned int> connection : roomConnections) {
				if (connection.first == room) return true;
			}

			return false;
		}

		bool ConnectionUsed(unsigned int connectionId) const {
			for (std::pair<Room*, unsigned int> connection : roomConnections) {
				if (connection.second == connectionId) return true;
			}

			return false;
		}

		const std::vector<Room*> ConnectedRooms() const {
			std::vector<Room*> connectedRooms;

			for (std::pair<Room*, unsigned int> connection : roomConnections) {
				connectedRooms.push_back(connection.first);
			}

			return connectedRooms;
		}

		const std::set<std::pair<Room*, unsigned int>> RoomConnections() const {
			return roomConnections;
		}

		int RoomEntranceCount() const {
			return roomEntrances.size();
		}

		const std::vector<std::pair<Vector2i, ShortcutType>> ShortcutConnections() const {
			return shortcutEntrances;
		}

		const std::vector<Vector2i> RoomEntrances() const {
			return roomEntrances;
		}

		const int DenId(Vector2i coord) const {
			auto it = find(denEntrances.begin(), denEntrances.end(), coord);

			if (it == denEntrances.end()) return -1;

			return (it - denEntrances.begin()) + roomEntrances.size();
		}

		bool CreatureDenExists(int id) {
			return CreatureDen01Exists(id - roomEntrances.size());
		}

		bool CreatureDen01Exists(int id) {
			return (id >= 0 && id < dens.size());
		}

		Den &CreatureDen(int id) {
			return CreatureDen01(id - roomEntrances.size());
		}

		Den &CreatureDen01(int id) {
			if (id < 0 || id >= dens.size()) {
				std::cout << "INVALID DEN " << id << std::endl;
				throw "INVALID DEN";
			}

			return dens[id];
		}

		const int DenCount() const {
			return denEntrances.size();
		}

		const std::vector<Vector2i> DenEntrances() const {
			return denEntrances;
		}

		const std::vector<Den> Dens() const {
			return dens;
		}

		const int Width() const { return width; }
		const int Height() const { return height; }

		void Tag(const std::string newTag) { tags.clear(); if (newTag != "") tags.push_back(newTag); }
		void ToggleTag(const std::string newTag) {
			if (std::find(tags.begin(), tags.end(), newTag) != tags.end()) {
				std::remove(tags.begin(), tags.end(), newTag);
			} else {
				tags.push_back(newTag);
			}
		}
		const std::vector<std::string> Tags() const { return tags; }

		const int Images() const { return images; }

		std::string roomName = "";
		int layer = 0;
		int water = 0;
		int subregion = 0;
		
		ExtraRoomData data;
		
		int hoveredDen = -1;
		
		bool valid;

	protected:
		Room() {}
		
		std::vector<uint8_t> parseStringToUint8Vector(const std::string& input) {
			std::vector<uint8_t> result;
			std::stringstream ss(input);
			std::string token;

			while (std::getline(ss, token, ',')) {
				result.push_back(static_cast<uint8_t>(std::stoi(token)));
			}

			return result;
		}

		void ensureConnections() {
			std::vector<std::pair<Vector2i, ShortcutType>> verifiedConnections;

			for (int i = shortcutEntrances.size() - 1; i >= 0; i--) {
				Vector2i connection = shortcutEntrances[i].first;

				Vector2i forwardDirection = Vector2i(0, 0);
				bool hasDirection = true;

				if (tileIsShortcut(connection.x - 1, connection.y))
					forwardDirection.x = -1;
				else if (tileIsShortcut(connection.x, connection.y + 1))
					forwardDirection.y = 1;
				else if (tileIsShortcut(connection.x + 1, connection.y))
					forwardDirection.x = 1;
				else if (tileIsShortcut(connection.x, connection.y - 1))
					forwardDirection.y = -1;

				if (forwardDirection.x == 0 && forwardDirection.y == 0) continue;

				int runs = 0;
				while (runs++ < 10000) {
					connection += forwardDirection;

					if (!tileIsShortcut(connection.x + forwardDirection.x, connection.y + forwardDirection.y)) {
						Vector2i lastDirection = Vector2i(forwardDirection);

						forwardDirection.x = 0;
						forwardDirection.y = 0;
						hasDirection = false;
						if (     lastDirection.x !=  1 && tileIsShortcut(connection.x - 1, connection.y    )) { forwardDirection.x = -1; hasDirection = true; }
						else if (lastDirection.y != -1 && tileIsShortcut(connection.x,     connection.y + 1)) { forwardDirection.y =  1; hasDirection = true; }
						else if (lastDirection.x != -1 && tileIsShortcut(connection.x + 1, connection.y    )) { forwardDirection.x =  1; hasDirection = true; }
						else if (lastDirection.y !=  1 && tileIsShortcut(connection.x,     connection.y - 1)) { forwardDirection.y = -1; hasDirection = true; }
					}

					if (getTile(connection.x, connection.y) % 16 == 4) {
						hasDirection = true;
						break;
					}
					
					if (!hasDirection) break;
				}

				if (hasDirection) verifiedConnections.push_back(std::make_pair(connection, shortcutEntrances[i].second));
			}

			std::reverse(verifiedConnections.begin(), verifiedConnections.end());

			for (size_t i = 0; i < shortcutEntrances.size(); ++i) {
				for (size_t j = 0; j < shortcutEntrances.size() - i - 1; ++j) {
					const Vector2i &a = shortcutEntrances[j].first;
					const Vector2i &b = shortcutEntrances[j + 1].first;

					if (a.y > b.y || (a.y == b.y && a.x > b.x)) {
						std::swap(shortcutEntrances[j], shortcutEntrances[j + 1]);
						std::swap(verifiedConnections[j], verifiedConnections[j + 1]);
					}
				}
			}
			
			for (std::pair<Vector2i, ShortcutType> verifiedConnection : verifiedConnections) {
				ShortcutType type = verifiedConnection.second;

				if (type == ShortcutType::ROOM) {
					roomEntrances.push_back(verifiedConnection.first);
				} else if (type == ShortcutType::DEN) {
					denEntrances.push_back(verifiedConnection.first);
				}
			}
		}

		void loadGeometry() {
			std::fstream geometryFile(path);
			if (!geometryFile.is_open() || !std::filesystem::exists(path)) {
				FailureController::fails.push_back("Failed to load '" + roomName + "' - Doesn't exist");
				std::cout << "Failed to load '" << path << "' - Doesn't exist." << std::endl;
				width = 72;
				height = 43;
				water = -1;
				geometry = new int[width * height];
				for (int i = 0; i < width * height; i++) {
					geometry[i] = 0;
				}
				valid = false;
				return;
			}

			std::string tempLine;

			std::getline(geometryFile, tempLine);
			std::getline(geometryFile, tempLine, '*');
			width = std::stoi(tempLine);
			std::getline(geometryFile, tempLine, '|');
			height = std::stoi(tempLine);
			if (tempLine.find('\n') != std::string::npos) {
				water = -1;
			} else {
				std::getline(geometryFile, tempLine, '|');
				water = std::stoi(tempLine);
				if (water == 0) water = -1;
				std::getline(geometryFile, tempLine); // Junk
			}

			std::getline(geometryFile, tempLine); // Junk
			
			std::getline(geometryFile, tempLine); // Cameras
			std::vector<std::string> items = split(tempLine, '|');
			images = items.size();
			
			std::getline(geometryFile, tempLine); // Junk
			std::getline(geometryFile, tempLine); // Junk
			std::getline(geometryFile, tempLine); // Junk
			std::getline(geometryFile, tempLine); // Junk
			std::getline(geometryFile, tempLine); // Junk
			std::getline(geometryFile, tempLine); // Junk
			std::getline(geometryFile, tempLine); // Junk

			// Collision Data
			geometry = new int[width * height];

			int tileId = 0;
			while (std::getline(geometryFile, tempLine, '|')) {
				if (tempLine.empty() || tempLine == "\n") break;
				if (!std::isdigit(tempLine[0])) break;

				std::vector<uint8_t> data = parseStringToUint8Vector(tempLine);
				geometry[tileId] = data[0];

				for (int i = 1; i < data.size(); i++) {
					switch (data[i]) {
						case 1: // Vertical Pole
							geometry[tileId] += 16;
							break;
						case 2: // Horizontal Pole
							geometry[tileId] += 32;
							break;
						case 3: // Shortcut
							geometry[tileId] += 128;
							break;
						case 4: // Exit
							geometry[tileId] += 64;
							shortcutEntrances.push_back(std::make_pair( Vector2i(tileId / height, tileId % height), ShortcutType::ROOM ));
							break;
						case 5: // Den
							geometry[tileId] += 256;
							shortcutEntrances.push_back(std::make_pair( Vector2i(tileId / height, tileId % height), ShortcutType::DEN ));
							break;
					}
				}

				tileId++;

			}

			geometryFile.close();

			valid = true;
			ensureConnections();

			for (Vector2i denLocation : denEntrances) {
				dens.push_back(Den("", 0, "", 0.0));
			}
		}
		
		void checkImages() {
			if (!Settings::getSetting<bool>(Settings::Setting::WarnMissingImages)) return;
			
			std::string imageDir = path.substr(0, path.find_last_of(std::filesystem::path::preferred_separator));
			for (int i = 0; i < images; i++) {
				std::string imagePath = roomName + "_" + std::to_string(i + 1) + ".png";
				
				std::string foundPath = findFileCaseInsensitive(imageDir, imagePath);
				
				if (foundPath.empty()) {
					FailureController::fails.push_back("Can't find '" + imagePath + "'");
				}
			}
		}

		void generateVBO();
		void addQuad(const Vertex &a, const Vertex &b, const Vertex &c, const Vertex &d);
		void addTri(const Vertex &a, const Vertex &b, const Vertex &c);

		std::vector<Vertex> vertices;
		std::vector<uint32_t> indices;
		size_t cur_index;
		GLuint vbo[2]; // first: vertices, second: indices
		GLuint vao;

		std::string path;

		Vector2 position;

		int width;
		int height;

		int *geometry;
		std::vector<Den> dens;
		int images;

		std::vector<std::string> tags;

		std::set<std::pair<Room*, unsigned int>> roomConnections;
		std::vector<Vector2i> roomEntrances;
		std::vector<Vector2i> denEntrances;
		std::vector<std::pair<Vector2i, ShortcutType>> shortcutEntrances;
};