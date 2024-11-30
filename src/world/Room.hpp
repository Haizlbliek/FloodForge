#include "../gl.h"

#include <random>
#include <string>
#include <fstream>
#include <sstream>
#include <vector>
#include <iostream>
#include <stack>

#include "../Texture.hpp"
#include "../Grid.hpp"
#include "../math/Vector.hpp"
#include "../math/Matrix4.hpp"
#include "../font/Fonts.hpp"

#include "Shaders.hpp"
#include "Globals.hpp"

//#define DEBUG_ROOMS

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

			loadGeometry();
			generateVBO();
		}

		virtual ~Room() {
			delete position;
			delete coord;
			delete geometry;

			position = nullptr;
			coord = nullptr;
			geometry = nullptr;
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
				cornerMin.x >= position->x &&
				cornerMin.y >= position->y - height &&
				cornerMax.x <= position->x + width &&
				cornerMax.y <= position->y
			);
		}

		int getTile(int x, int y) const {
			if (!valid) return 1;

			if (x < 0 || y < 0) return 1;
			if (x >= width || y >= height) return 1;

			return geometry[x * height + y];
		}

		virtual void draw(Vector2 mousePosition, double lineSize);
/*
		virtual void draw(Vector2 mousePosition, double lineSize) {
			if (!valid) return;

			Colour tint = Colour(1.0, 1.0, 1.0);
			double tintAmount = 0.5;

			if (::roomColours == 1) {
				if (layer == 0) tint = Colour(1.0, 0.0, 0.0);
				if (layer == 1) tint = Colour(1.0, 1.0, 1.0);
				if (layer == 2) tint = Colour(0.0, 1.0, 0.0);
			}
			
			if (::roomColours == 2) {
				if (subregion == -1) tint = Colour(1.0, 1.0, 1.0);
				if (subregion ==  0) tint = Colour(1.0, 0.0, 0.0);
				if (subregion ==  1) tint = Colour(0.0, 1.0, 0.0);
				if (subregion ==  2) tint = Colour(0.0, 0.0, 1.0);
				if (subregion ==  3) tint = Colour(1.0, 1.0, 0.0);
				if (subregion ==  4) tint = Colour(0.0, 1.0, 1.0);
				if (subregion ==  5) tint = Colour(1.0, 0.0, 1.0);
				if (subregion ==  6) tint = Colour(1.0, 0.5, 0.0);
				if (subregion ==  7) tint = Colour(1.0, 1.0, 0.5);
				if (subregion ==  8) tint = Colour(0.5, 1.0, 0.0);
				if (subregion ==  9) tint = Colour(1.0, 1.0, 0.5);
				if (subregion == 10) tint = Colour(0.5, 0.0, 1.0);
				if (subregion == 11) tint = Colour(1.0, 0.5, 1.0);
			}

			glColor(Colour(1.0, 1.0, 1.0).mix(tint, tintAmount));
			fillRect(position->x, position->y, position->x + width, position->y - height);

			glBegin(GL_QUADS);
			for (int x = 0; x < width; x++) {
				for (int y = 0; y < height; y++) {
					int tileType = getTile(x, y) % 16;
					int tileData = getTile(x, y) / 16;

					float x0 = position->x + x;
					float y0 = position->y - y;
					float x1 = position->x + x + 1;
					float y1 = position->y - y - 1;
					float x2 = (x0 + x1) * 0.5;
					float y2 = (y0 + y1) * 0.5;

					if (tileType == 1) {
						glColor(Colour(0.125, 0.125, 0.125).mix(tint, tintAmount));
						glVertex2f(x0, y0);
						glVertex2f(x1, y0);
						glVertex2f(x1, y1);
						glVertex2f(x0, y1);
					}
					if (tileType == 4) {
						glColor(Colour(0.0, 1.0, 1.0).mix(tint, tintAmount));
						glVertex2f(x0, y0);
						glVertex2f(x1, y0);
						glVertex2f(x1, y1);
						glVertex2f(x0, y1);
					}
					if (tileType == 2) {
						glColor(Colour(1.0, 0.0, 0.0).mix(tint, tintAmount));

						int bits = 0;
						bits += (getTile(x - 1, y) == 1) ? 1 : 0;
						bits += (getTile(x + 1, y) == 1) ? 2 : 0;
						bits += (getTile(x, y - 1) == 1) ? 4 : 0;
						bits += (getTile(x, y + 1) == 1) ? 8 : 0;

						if (bits == 1 + 4) {
							glVertex2f(x0, y0);
							glVertex2f(x1, y0);
							glVertex2f(x0, y1);
							glVertex2f(x0, y0);
						} else if (bits == 1 + 8) {
							glVertex2f(x0, y1);
							glVertex2f(x1, y1);
							glVertex2f(x0, y0);
							glVertex2f(x0, y1);
						} else if (bits == 2 + 4) {
							glVertex2f(x1, y0);
							glVertex2f(x0, y0);
							glVertex2f(x1, y1);
							glVertex2f(x1, y0);
						} else if (bits == 2 + 8) {
							glVertex2f(x1, y1);
							glVertex2f(x0, y1);
							glVertex2f(x1, y0);
							glVertex2f(x1, y1);
						}
					}
					if (tileType == 3) {
						glColor(Colour(0.0, 1.0, 0.0).mix(tint, tintAmount));
						glVertex2f(x0, y0);
						glVertex2f(x1, y0);
						glVertex2f(x1, (y0 + y1) * 0.5f);
						glVertex2f(x0, (y0 + y1) * 0.5f);
					}

					if (tileData & 1) { // 16 - Vertical Pole
						glColor(Colour(0.0, 0.0, 1.0).mix(tint, tintAmount));
						glVertex2f(x0 + 0.375, y0);
						glVertex2f(x1 - 0.375, y0);
						glVertex2f(x1 - 0.375, y1);
						glVertex2f(x0 + 0.375, y1);
					}

					if (tileData & 2) { // 32 - Horizontal Pole
						glColor(Colour(0.0, 0.0, 1.0).mix(tint, tintAmount));
						glVertex2f(x0, y0 - 0.375);
						glVertex2f(x1, y0 - 0.375);
						glVertex2f(x1, y1 + 0.375);
						glVertex2f(x0, y1 + 0.375);
					}

					if (tileData & 4) { // 64 - Room Exit
						glColor(Colour(1.0, 0.0, 1.0).mix(tint, tintAmount));
						glVertex2f(x0 + 0.25, y0 - 0.25);
						glVertex2f(x1 - 0.25, y0 - 0.25);
						glVertex2f(x1 - 0.25, y1 + 0.25);
						glVertex2f(x0 + 0.25, y1 + 0.25);
					}

					if (tileData & 8) { // 128 - Shortcut
						glColor(Colour(0.125, 0.125, 0.125).mix(tint, tintAmount));
						glVertex2f(x0 + 0.40625, y0 - 0.40625);
						glVertex2f(x1 - 0.40625, y0 - 0.40625);
						glVertex2f(x1 - 0.40625, y1 + 0.40625);
						glVertex2f(x0 + 0.40625, y1 + 0.40625);

						glColor(Colour(1.0, 1.0, 1.0).mix(tint, tintAmount));
						glVertex2f(x0 + 0.4375, y0 - 0.4375);
						glVertex2f(x1 - 0.4375, y0 - 0.4375);
						glVertex2f(x1 - 0.4375, y1 + 0.4375);
						glVertex2f(x0 + 0.4375, y1 + 0.4375);
					}
				}
			}
			glEnd();

			if (debugRoomConnections) {
				for (int x = 0; x < width; x++) {
					for (int y = 0; y < height; y++) {
						if (!((getTile(x, y) / 16) & 4)) continue;

						float x0 = position->x + x;
						float y0 = position->y - y;

						glColor(Colour(1.0, 1.0, 1.0).mix(tint, tintAmount));
						Fonts::rainworld->writeCentred(std::to_string(getConnection(Vector2i(x, y))), x0 + 0.5, y0 - 0.5, 3.0, CENTER_XY);
					}
				}
			}

			if (water != -1) {
				glColor(Colour(0.0, 0.0, 0.5, 0.5).mix(tint, tintAmount));
				fillRect(position->x, position->y - (height - std::min(water, height)), position->x + width, position->y - height);
			}

			if (inside(mousePosition)) {
				glColor(Colour(0.00, 0.75, 0.00).mix(tint, tintAmount));
			} else {
				glColor(Colour(0.75, 0.75, 0.75).mix(tint, tintAmount));
			}
			strokeRect(position->x, position->y, position->x + width, position->y - height);

#ifdef DEBUG_ROOMS
			glColor(Colour(1.0, 1.0, 0.0).mix(tint, tintAmount));
			strokerect(coord->x - 1, coord->y + 1, coord->x + 1, coord->y - 1);
#endif
		}
*/
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

		const Vector2 getConnection(unsigned int connectionId) const {
			if (connectionId >= connections.size()) return Vector2(0, 0);
			Vector2i connection = connections[connectionId];
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
					const Vector2i& a = connections[j];
					const Vector2i& b = connections[j + 1];

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

			std::getline(geometryFile, roomName);
			std::getline(geometryFile, tempLine, '*');
			width = std::stoi(tempLine);
			std::getline(geometryFile, tempLine, '|');
			height = std::stoi(tempLine);
			std::getline(geometryFile, tempLine, '|');
			water = std::stoi(tempLine);
			if (water == 0) water = -1;
			std::getline(geometryFile, tempLine); // Junk

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

		void generateVBO() {
			glGenBuffers(1, &vbo);
			glGenVertexArrays(1, &vao);
			fillRect(position->x, position->y, position->x + width, position->y - height);

			glBegin(GL_QUADS);
			for (int x = 0; x < width; x++) {
				for (int y = 0; y < height; y++) {
					int tileType = getTile(x, y) % 16;
					int tileData = getTile(x, y) / 16;

					float x0 = position->x + x;
					float y0 = position->y - y;
					float x1 = position->x + x + 1;
					float y1 = position->y - y - 1;
					float x2 = (x0 + x1) * 0.5;
					float y2 = (y0 + y1) * 0.5;

					if (tileType == 1) {
						vertices.push_back({ x0, y0, 0.125, 0.125, 0.125 });
						vertices.push_back({ x1, y0, 0.125, 0.125, 0.125 });
						vertices.push_back({ x1, y1, 0.125, 0.125, 0.125 });
						vertices.push_back({ x0, y1, 0.125, 0.125, 0.125 });
					}
					if (tileType == 4) {
						vertices.push_back({ x0, y0, 0.0, 1.0, 1.0 });
						vertices.push_back({ x1, y0, 0.0, 1.0, 1.0 });
						vertices.push_back({ x1, y1, 0.0, 1.0, 1.0 });
						vertices.push_back({ x0, y1, 0.0, 1.0, 1.0 });
					}
					if (tileType == 2) {
						int bits = 0;
						bits += (getTile(x - 1, y) == 1) ? 1 : 0;
						bits += (getTile(x + 1, y) == 1) ? 2 : 0;
						bits += (getTile(x, y - 1) == 1) ? 4 : 0;
						bits += (getTile(x, y + 1) == 1) ? 8 : 0;

						if (bits == 1 + 4) {
							vertices.push_back({ x0, y0, 1.0, 0.0, 0.0 });
							vertices.push_back({ x1, y0, 1.0, 0.0, 0.0 });
							vertices.push_back({ x0, y1, 1.0, 0.0, 0.0 });
							vertices.push_back({ x0, y0, 1.0, 0.0, 0.0 });
						} else if (bits == 1 + 8) {
							vertices.push_back({ x0, y1, 1.0, 0.0, 0.0 });
							vertices.push_back({ x1, y1, 1.0, 0.0, 0.0 });
							vertices.push_back({ x0, y0, 1.0, 0.0, 0.0 });
							vertices.push_back({ x0, y1, 1.0, 0.0, 0.0 });
						} else if (bits == 2 + 4) {
							vertices.push_back({ x1, y0, 1.0, 0.0, 0.0 });
							vertices.push_back({ x0, y0, 1.0, 0.0, 0.0 });
							vertices.push_back({ x1, y1, 1.0, 0.0, 0.0 });
							vertices.push_back({ x1, y0, 1.0, 0.0, 0.0 });
						} else if (bits == 2 + 8) {
							vertices.push_back({ x1, y1, 1.0, 0.0, 0.0 });
							vertices.push_back({ x0, y1, 1.0, 0.0, 0.0 });
							vertices.push_back({ x1, y0, 1.0, 0.0, 0.0 });
							vertices.push_back({ x1, y1, 1.0, 0.0, 0.0 });
						}
					}
					if (tileType == 3) {
						vertices.push_back({ x0, y0, 0.0, 1.0, 0.0 });
						vertices.push_back({ x1, y0, 0.0, 1.0, 0.0 });
						vertices.push_back({ x1, (y0 + y1) * 0.5f, 0.0, 1.0, 0.0 });
						vertices.push_back({ x0, (y0 + y1) * 0.5f, 0.0, 1.0, 0.0 });
					}

					if (tileData & 1) { // 16 - Vertical Pole
						vertices.push_back({ x0 + 0.375f, y0, 0.0, 0.0, 1.0 });
						vertices.push_back({ x1 - 0.375f, y0, 0.0, 0.0, 1.0 });
						vertices.push_back({ x1 - 0.375f, y1, 0.0, 0.0, 1.0 });
						vertices.push_back({ x0 + 0.375f, y1, 0.0, 0.0, 1.0 });
					}

					if (tileData & 2) { // 32 - Horizontal Pole
						vertices.push_back({ x0, y0 - 0.375f, 0.0, 0.0, 1.0 });
						vertices.push_back({ x1, y0 - 0.375f, 0.0, 0.0, 1.0 });
						vertices.push_back({ x1, y1 + 0.375f, 0.0, 0.0, 1.0 });
						vertices.push_back({ x0, y1 + 0.375f, 0.0, 0.0, 1.0 });
					}

					if (tileData & 4) { // 64 - Room Exit
						vertices.push_back({ x0 + 0.25f, y0 - 0.25f, 1.0, 0.0, 1.0 });
						vertices.push_back({ x1 - 0.25f, y0 - 0.25f, 1.0, 0.0, 1.0 });
						vertices.push_back({ x1 - 0.25f, y1 + 0.25f, 1.0, 0.0, 1.0 });
						vertices.push_back({ x0 + 0.25f, y1 + 0.25f, 1.0, 0.0, 1.0 });
					}

					if (tileData & 8) { // 128 - Shortcut
						vertices.push_back({ x0 + 0.40625f, y0 - 0.40625f, 0.125, 0.125, 0.125 });
						vertices.push_back({ x1 - 0.40625f, y0 - 0.40625f, 0.125, 0.125, 0.125 });
						vertices.push_back({ x1 - 0.40625f, y1 + 0.40625f, 0.125, 0.125, 0.125 });
						vertices.push_back({ x0 + 0.40625f, y1 + 0.40625f, 0.125, 0.125, 0.125 });

						vertices.push_back({ x0 + 0.4375f, y0 - 0.4375f, 1.0, 1.0, 1.0 });
						vertices.push_back({ x1 - 0.4375f, y0 - 0.4375f, 1.0, 1.0, 1.0 });
						vertices.push_back({ x1 - 0.4375f, y1 + 0.4375f, 1.0, 1.0, 1.0 });
						vertices.push_back({ x0 + 0.4375f, y1 + 0.4375f, 1.0, 1.0, 1.0 });
					}
				}
			}
			glEnd();

			// if (debugRoomConnections) {
			// 	for (int x = 0; x < width; x++) {
			// 		for (int y = 0; y < height; y++) {
			// 			if (!((getTile(x, y) / 16) & 4)) continue;

			// 			float x0 = position->x + x;
			// 			float y0 = position->y - y;

			// 			glColor(Colour(1.0, 1.0, 1.0).mix(tint, tintAmount));
			// 			Fonts::rainworld->writeCentred(std::to_string(getConnection(Vector2i(x, y))), x0 + 0.5, y0 - 0.5, 3.0, CENTER_XY);
			// 		}
			// 	}
			// }
		}

		std::vector<Vertex> vertices;
		GLuint vbo;
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

		bool valid;

		std::set<std::pair<Room*, unsigned int>> roomConnections;
		std::vector<Vector2i> shortcutEntrances;
		std::vector<Vector2i> connections;

		int water;
};

#endif