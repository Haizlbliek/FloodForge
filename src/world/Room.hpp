#include "../gl.h"

#include <random>
#include <string>
#include <fstream>
#include <sstream>
#include <vector>
#include <iostream>
#include <stack>
#include <set>

#include "../Texture.hpp"
#include "../math/Vector.hpp"
#include "../math/Matrix4.hpp"
#include "../font/Fonts.hpp"

#include "Shaders.hpp"
#include "Globals.hpp"

#ifndef ROOM_HPP
#define ROOM_HPP

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
		Room(std::string path, std::string name) {
			this->path = path;
			this->roomName = toLower(name);

			position = new Vector2(
				0.0f,
				0.0f
			);

			coord = new Vector2();

			width = 1;
			height = 1;

			valid = false;

			geometry = nullptr;

			layer = 0;
			water = -1;
			subregion = -1;

			tag = "";
			hidden = false;

			loadGeometry();
			generateVBO();
		}

		virtual ~Room() {
			delete position;
			delete coord;
			delete[] geometry;

			position = nullptr;
			coord = nullptr;
			geometry = nullptr;

			glDeleteBuffers(2, vbo);
			glDeleteVertexArrays(1, &vao);
		}

		bool inside(Vector2 otherPosition) {
			return (
				otherPosition.x >= position->x &&
				otherPosition.y >= position->y - height &&
				otherPosition.x <= position->x + width &&
				otherPosition.y <= position->y
			);
		}

		bool intersects(Vector2 corner0, Vector2 corner1) {
			Vector2 cornerMin = Vector2::min(corner0, corner1);
			Vector2 cornerMax = Vector2::max(corner0, corner1);

			return (
				cornerMin.x <= position->x + width &&
				cornerMin.y <= position->y &&
				cornerMax.x >= position->x &&
				cornerMax.y >= position->y - height
			);
		}

		int getTile(int x, int y) const {
			if (!valid) return 1;

			if (x < 0 || y < 0) return 1;
			if (x >= width || y >= height) return 1;

			return geometry[x * height + y];
		}

		virtual void draw(Vector2 mousePosition, double lineSize);
		
		void Position(Vector2 newPosition) {
			position->x = newPosition.x;
			position->y = newPosition.y;
		}

		Vector2 *Position() const {
			return position;
		}

		void Coord(const Vector2 &coord) {
			this->coord->x = coord.x;
			this->coord->y = coord.y;
		}

		const std::vector<Vector2> Connections() const {
			std::vector<Vector2> transformedConnections;

			for (Vector2i connection : connections) {
				transformedConnections.push_back(Vector2(
					position->x + connection.x + 0.5,
					position->y - connection.y - 0.5
				));
			}

			return transformedConnections;
		}

		int getConnection(const Vector2i &searchPosition) const {
			unsigned int connectionId = 0;

			for (Vector2i connectionPosition : connections) {
				if (connectionPosition == searchPosition) {
					return connectionId;
				}

				connectionId++;
			}

			return -1;
		}

		const Vector2 getConnectionPosition(unsigned int connectionId) const {
			if (connectionId >= connections.size()) return Vector2(0, 0);
			Vector2i connection = connections[connectionId];
			return Vector2(
				position->x + connection.x + 0.5,
				position->y - connection.y - 0.5
			);
		}

		const Vector2 getShortcutConnectionPosition(unsigned int connectionId) const {
			if (connectionId >= connections.size()) return Vector2(0, 0);
			Vector2i connection = getShortcutConnection(connectionId);
			return Vector2(
				position->x + connection.x + 0.5,
				position->y - connection.y - 0.5
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

			if (getTile(connection.x - 1, connection.y) & 128)
				forwardDirection = 2;
			else if (getTile(connection.x, connection.y + 1) & 128)
				forwardDirection = 3;
			else if (getTile(connection.x + 1, connection.y) & 128)
				forwardDirection = 0;
			else if (getTile(connection.x, connection.y - 1) & 128)
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
			return connections.size();
		}

		const std::vector<Vector2i> TileConnections() const {
			return connections;
		}

		const std::vector<Vector2i> ShortcutEntrances() const {
			return shortcutEntrances;
		}

		const int Width() const { return width; }
		const int Height() const { return height; }

		const std::string RoomName() const { return roomName; }

		void Layer(const int newLayer) { layer = newLayer; }
		const int Layer() const { return layer; }

		void Water(const int newWater) { water = newWater; }
		const int Water() const { return water; }

		void Tag(const std::string newTag) { tag = newTag; }
		const std::string Tag() const { return tag; }

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
				// Convert token to uint8_t and add to the vector
				result.push_back(static_cast<uint8_t>(std::stoi(token)));
			}

			return result;
		}

		void ensureConnections() {
			std::stack<Vector2i> verifiedConnections;

			for (int i = connections.size() - 1; i >= 0; i--) {
				Vector2i connection = Vector2i(connections[i]);

				Vector2i forwardDirection = Vector2i(0, 0);

				if (getTile(connection.x - 1, connection.y) & 128)
					forwardDirection.x = -1;
				else if (getTile(connection.x, connection.y + 1) & 128)
					forwardDirection.y = 1;
				else if (getTile(connection.x + 1, connection.y) & 128)
					forwardDirection.x = 1;
				else if (getTile(connection.x, connection.y - 1) & 128)
					forwardDirection.y = -1;

				if (forwardDirection.x == 0 && forwardDirection.y == 0) continue;

				int runs = 0;
				while (runs++ < 10000) {
					connection += forwardDirection;

					if ((getTile(connection.x + forwardDirection.x, connection.y + forwardDirection.y) & 128) == 0) {
						Vector2i lastDirection = Vector2i(forwardDirection);

						forwardDirection.x = 0;
						forwardDirection.y = 0;
						if (lastDirection.x !=  1 && getTile(connection.x - 1, connection.y) & 128)
							forwardDirection.x = -1;
						else if (lastDirection.y != -1 && getTile(connection.x, connection.y + 1) & 128)
							forwardDirection.y = 1;
						else if (lastDirection.x != -1 && getTile(connection.x + 1, connection.y) & 128)
							forwardDirection.x = 1;
						else if (lastDirection.y !=  1 && getTile(connection.x, connection.y - 1) & 128)
							forwardDirection.y = -1;
					}

					if (getTile(connection.x, connection.y) % 16 == 4) {
						break;
					}
				}

				verifiedConnections.push(connection);
			}

			int i = 0;
			while (verifiedConnections.size() > 0) {
				shortcutEntrances.push_back(verifiedConnections.top());
				verifiedConnections.pop();
				i++;
			}


			for (size_t i = 0; i < connections.size(); ++i) {
				for (size_t j = 0; j < connections.size() - i - 1; ++j) {
					const Vector2i &a = connections[j];
					const Vector2i &b = connections[j + 1];

					if (a.y > b.y || (a.y == b.y && a.x > b.x)) {
						std::swap(connections[j], connections[j + 1]);
						std::swap(shortcutEntrances[j], shortcutEntrances[j + 1]);
					}
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
				std::cout << "Yep, vater" << std::endl;
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

			// std::cout << "Data for " << roomName << "\n"
			// << "Width: " << width << "\n"
			// << "Height: " << height << "\n";

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
							connections.push_back(Vector2i(tileId / height, tileId % height));
							break;
					}
				}

				tileId++;

			}

			geometryFile.close();

			valid = true;
			ensureConnections();
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

		Vector2 *position;
		Vector2 *coord;

		int width;
		int height;

		int *geometry;
		int layer;
		int subregion;

		std::string tag;
		bool hidden;

		bool valid;

		std::set<std::pair<Room*, unsigned int>> roomConnections;
		std::vector<Vector2i> shortcutEntrances;
		std::vector<Vector2i> connections;

		int water;
};

#endif