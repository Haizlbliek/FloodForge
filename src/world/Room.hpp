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
#include "../math/Vector.hpp"
#include "../math/Matrix4.hpp"
#include "../font/Fonts.hpp"
#include "../Theme.hpp"
#include "../popup/WarningPopup.hpp"

#include "Shaders.hpp"
#include "Den.hpp"

#define CONNECTION_TYPE_NONE 0
#define CONNECTION_TYPE_EXIT 1
#define CONNECTION_TYPE_DEN  2
#define CONNECTION_TYPE_MOLE 3
#define CONNECTION_TYPE_SCAV 4

struct Vertex {
    float x, y;
    float r, g, b, a;
};

class Room {
	public:
		const bool isOffscreen = false;

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

			hidden = false;

			loadGeometry();
			generateVBO();
		}

		virtual ~Room() {
			if (geometry != nullptr) delete[] geometry;

			geometry = nullptr;

			glDeleteBuffers(2, vbo);
			glDeleteVertexArrays(1, &vao);
		}

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

		virtual void draw(Vector2 mousePosition, double lineSize, Vector2 screenBounds);
		
		void Position(Vector2 position) {
			this->position.x = position.x;
			this->position.y = position.y;
		}

		Vector2 &Position() {
			return position;
		}

		const std::vector<Vector2> Connections() const {
			std::vector<Vector2> transformedConnections;

			for (const std::pair<Vector2i, int> &connection : connections) {
				transformedConnections.push_back(Vector2(
					position.x + connection.first.x + 0.5,
					position.y - connection.first.y - 0.5
				));
			}

			return transformedConnections;
		}

		int getConnection(const Vector2i &searchPosition) const {
			unsigned int connectionId = 0;

			for (const std::pair<Vector2i, int> &connection : connections) {
				if (connection.first == searchPosition) {
					return connectionId;
				}

				connectionId++;
			}

			return -1;
		}

		const Vector2 getConnectionPosition(unsigned int connectionId) const {
			if (connectionId >= connections.size()) return Vector2(0, 0);
			const Vector2i &connection = connections[connectionId].first;
			return Vector2(
				position.x + connection.x + 0.5,
				position.y - connection.y - 0.5
			);
		}

		const Vector2 getShortcutConnectionPosition(unsigned int connectionId) const {
			if (connectionId >= connections.size()) return Vector2(0, 0);
			Vector2i connection = getShortcutConnection(connectionId);
			return Vector2(
				position.x + connection.x + 0.5,
				position.y - connection.y - 0.5
			);
		}

		int getShortcutConnection(const Vector2i &searchPosition) const {
			int index = 0;
			for (const Vector2i enterance : shortcutEntrances) {
				if (enterance == searchPosition) {
					return index;
				}

				index++;
			}

			return -1;
		}

		const Vector2i getShortcutConnection(unsigned int connectionId) const {
			Vector2i connection = shortcutEntrances[connectionId];
			return connection;
		}

		int getShortcutDirection(unsigned int connectionId) const {
			Vector2i connection = shortcutEntrances[connectionId];

			int forwardDirection = -1;

			if (tileIsShortcut(connection.x - 1, connection.y))
				forwardDirection = 2;
			else if (tileIsShortcut(connection.x, connection.y + 1))
				forwardDirection = 3;
			else if (tileIsShortcut(connection.x + 1, connection.y))
				forwardDirection = 0;
			else if (tileIsShortcut(connection.x, connection.y - 1))
				forwardDirection = 1;

			return forwardDirection;
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

		int ConnectionCount() const {
			return shortcutEntrances.size();
		}

		const std::vector<std::pair<Vector2i, int>> TileConnections() const {
			return connections;
		}

		const std::vector<Vector2i> ShortcutEntrances() const {
			return shortcutEntrances;
		}

		const int DenId(Vector2i coord) const {
			auto it = find(denEntrances.begin(), denEntrances.end(), coord);

			if (it == denEntrances.end()) return -1;

			return (it - denEntrances.begin()) + shortcutEntrances.size();
		}

		bool CreatureDenExists(int id) {
			return CreatureDen01Exists(id - shortcutEntrances.size());
		}

		bool CreatureDen01Exists(int id) {
			return (id >= 0 && id < dens.size());
		}

		Den &CreatureDen(int id) {
			return CreatureDen01(id - shortcutEntrances.size());
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

		const std::string RoomName() const { return roomName; }

		void Layer(const int newLayer) { layer = newLayer; }
		const int Layer() const { return layer; }

		void Water(const int newWater) { water = newWater; }
		const int Water() const { return water; }

		void Tag(const std::string newTag) { tags.clear(); tags.push_back(newTag); }
		void ToggleTag(const std::string newTag) {
			if (std::find(tags.begin(), tags.end(), newTag) != tags.end()) {
				std::remove(tags.begin(), tags.end(), newTag);
			} else {
				tags.push_back(newTag);
			}
		}
		const std::vector<std::string> Tags() const { return tags; }

		void Subregion(const int newSubregion) { subregion = newSubregion; }
		const int Subregion() { return subregion; }

		void Hidden(const bool newHidden) { hidden = newHidden; }
		const bool Hidden() const { return hidden; }

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
			std::vector<std::pair<Vector2i, int>> verifiedConnections;

			for (int i = connections.size() - 1; i >= 0; i--) {
				Vector2i connection = connections[i].first;

				Vector2i forwardDirection = Vector2i(0, 0);

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
						if (lastDirection.x !=  1 && tileIsShortcut(connection.x - 1, connection.y))
							forwardDirection.x = -1;
						else if (lastDirection.y != -1 && tileIsShortcut(connection.x, connection.y + 1))
							forwardDirection.y = 1;
						else if (lastDirection.x != -1 && tileIsShortcut(connection.x + 1, connection.y))
							forwardDirection.x = 1;
						else if (lastDirection.y !=  1 && tileIsShortcut(connection.x, connection.y - 1))
							forwardDirection.y = -1;
					}

					if (getTile(connection.x, connection.y) % 16 == 4) {
						break;
					}
				}

				verifiedConnections.push_back(std::pair<Vector2i, int>{connection, connections[i].second});
			}

			std::reverse(verifiedConnections.begin(), verifiedConnections.end());

			for (size_t i = 0; i < connections.size(); ++i) {
				for (size_t j = 0; j < connections.size() - i - 1; ++j) {
					const Vector2i &a = connections[j].first;
					const Vector2i &b = connections[j + 1].first;

					if (a.y > b.y || (a.y == b.y && a.x > b.x)) {
						std::swap(connections[j], connections[j + 1]);
						std::swap(verifiedConnections[j], verifiedConnections[j + 1]);
					}
				}
			}
			
			for (std::pair<Vector2i, int> verifiedConnection : verifiedConnections) {
				if (verifiedConnection.second == 0) {
					shortcutEntrances.push_back(verifiedConnection.first);
				} else {
					denEntrances.push_back(verifiedConnection.first);
				}
			}
		}

		void loadGeometry() {
			std::fstream geometryFile(path + ".txt");
			if (!geometryFile.is_open()) {
				std::cout << "Failed to load '" << path << "' - Doesn't exist." << std::endl;
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
			std::getline(geometryFile, tempLine); // Junk
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
							connections.push_back(std::pair<Vector2i, int>{ Vector2i(tileId / height, tileId % height), 0 });
							break;
						case 5: // Den
							geometry[tileId] += 256;
							connections.push_back(std::pair<Vector2i, int>{ Vector2i(tileId / height, tileId % height), 1 });
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

		void generateVBO();
		void addQuad(const Vertex &a, const Vertex &b, const Vertex &c, const Vertex &d);
		void addTri(const Vertex &a, const Vertex &b, const Vertex &c);

		std::vector<Vertex> vertices;
		std::vector<uint32_t> indices;
		size_t cur_index;
		GLuint vbo[2]; // first: vertices, second: indices
		GLuint vao;

		std::string path;
		std::string roomName;

		Vector2 position;

		int width;
		int height;

		int *geometry;
		int layer;
		int subregion;
		std::vector<Den> dens;

		std::vector<std::string> tags;
		bool hidden;

		bool valid;

		std::set<std::pair<Room*, unsigned int>> roomConnections;
		std::vector<Vector2i> shortcutEntrances;
		std::vector<Vector2i> denEntrances;
		std::vector<std::pair<Vector2i, int>> connections;

		int water;
};