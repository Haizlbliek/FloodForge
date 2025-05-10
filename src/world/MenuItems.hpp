#pragma once

#include <vector>
#include <functional>
#include <string>
#include <fstream>
#include <iostream>
#include <filesystem>
#include <tuple>
#include <iomanip>
#include <regex>

#include "../font/Fonts.hpp"
#include "../math/Rect.hpp"
#include "../math/Quadruple.hpp"

#include "../Utils.hpp"
#include "../Window.hpp"
#include "../Theme.hpp"

#include "Globals.hpp"
#include "Room.hpp"
#include "OffscreenRoom.hpp"
#include "DenPopup.hpp"
#include "FailureController.hpp"

#include "ExtraRoomData.hpp"

//#define VISIBLE_OUTPUT_PADDING

class Button {
	public:
		Button(std::string text, double x, double y, double width, double height, Font *font)
		 : x(x),
		   y(y),
		   width(width),
		   height(height),
		   text(text),
		   font(font) {
			// Temp
		}

		Button *OnLeftPress(std::function<void(Button*)> listener) {
			listenersLeft.push_back(listener);
			return this;
		}

		Button *OnRightPress(std::function<void(Button*)> listener) {
			listenersRight.push_back(listener);
			return this;
		}

		bool isHovered(Mouse *mouse, Vector2 screenBounds) {
			double mouseX = mouse->X() / 512.0 + screenBounds.x - 1.0;
			double mouseY = -(mouse->Y() / 512.0 + screenBounds.y - 1.0);

			return mouseX >= (x - 0.005) && mouseX <= (x + 0.005) + width && mouseY <= (y + 0.005) && mouseY >= (y - 0.005) - height;
		}

		void update(Mouse *mouse, Vector2 screenBounds) {
			pressed = isHovered(mouse, screenBounds) && (mouse->Left() || mouse->Right());

			if (isHovered(mouse, screenBounds) && mouse->JustLeft()) {
				press();
			}

			if (isHovered(mouse, screenBounds) && mouse->JustRight()) {
				pressRight();
			}
		}

		void draw(Mouse *mouse, Vector2 screenBounds);

		void Text(const std::string text) {
			this->text = text;
			width = font->getTextWidth(text, height - 0.01) + 0.02;
		}

		std::string Text() const { return text; }

		void X(const double x) {
			this->x = x;
		}

		Button &Darken(bool newDarken) {
			darken = newDarken;

			return *this;
		}

		const double Width() const {
			return width;
		}

	private:
		void press() {
			for (const auto &listener : listenersLeft) {
				listener(this);
			}
		}

		void pressRight() {
			for (const auto &listener : listenersRight) {
				listener(this);
			}
		}

		std::vector<std::function<void(Button*)>> listenersLeft;
		std::vector<std::function<void(Button*)>> listenersRight;

		bool pressed = false;

		double x;
		double y;
		double width;
		double height;

		std::string text;
		Font *font;

		bool darken = false;
};

class MenuItems {
	public:
		static void loadTextures() {
			std::string texturePath = BASE_PATH + "assets/themes/" + currentThemeName + "/";

			textureButtonNormal = loadTexture(texturePath + "ButtonNormal.png");
			textureButtonNormalHover = loadTexture(texturePath + "ButtonNormalHover.png");
			textureButtonPress = loadTexture(texturePath + "ButtonPress.png");
			textureButtonPressHover = loadTexture(texturePath + "ButtonPressHover.png");
			textureBar = loadTexture(texturePath + "Bar.png");
		}

		static Button &addButton(std::string text) {
			double x = currentButtonX;
			double width = Fonts::rainworld->getTextWidth(text, 0.03) + 0.02;

			currentButtonX += width + 0.04;
			Button *button = new Button(text, x, -0.01, width, 0.04, Fonts::rainworld);
			buttons.push_back(button);

			return *button;
		}

		static void replaceLastInstance(std::string& str, const std::string& old_sub, const std::string& new_sub) {
			size_t pos = str.rfind(old_sub);
			
			if (pos != std::string::npos) {
				str.replace(pos, old_sub.length(), new_sub);
			}
		}

		static void parseMap(std::filesystem::path mapFilePath, std::filesystem::path directory) {
			std::fstream mapFile(mapFilePath);
			
			std::map<std::string, ExtraRoomData> extraRoomData;

			std::string line;
			while (std::getline(mapFile, line)) {
				if (startsWith(line, "//FloodForge;")) {
					line = line.substr(line.find(';') + 1);
					std::vector<std::string> data = split(line, '|');
					
					if (data[0] == "ROOM") {
						extraRoomData[data[1]] = ExtraRoomData();
						
						for (int i = 2; i < data.size(); i++) {
							std::string key = data[i];
							
							if (key == "hidden") {
								extraRoomData[data[1]].hidden = true;
							} else if (key == "merge") {
								extraRoomData[data[1]].merge = true;
							}
						}
					}
				} else if (startsWith(line, "Connection: ")) {
					// line = line.substr(line.find(' ') + 1);

					// std::string roomAName = line.substr(0, line.find(','));
					// line = line.substr(line.find(',') + 1);
					// std::string roomBName = line.substr(0, line.find(','));
					// line = line.substr(line.find(',') + 1);

					// int connectionAX = std::stoi(line.substr(0, line.find(',')));
					// line = line.substr(line.find(',') + 1);
					// int connectionAY = std::stoi(line.substr(0, line.find(',')));
					// line = line.substr(line.find(',') + 1);
					// int connectionBX = std::stoi(line.substr(0, line.find(',')));
					// line = line.substr(line.find(',') + 1);
					// int connectionBY = std::stoi(line.substr(0, line.find(',')));
					// line = line.substr(line.find(',') + 1);

					// Room *roomA = nullptr;
					// Room *roomB = nullptr;

					// for (Room *room : rooms) {
					// 	if (room->RoomName() == roomAName) {
					// 		roomA = room;
					// 	}
					// 	if (room->RoomName() == roomBName) {
					// 		roomB = room;
					// 	}
					// }

					// if (roomA == nullptr || roomB == nullptr) continue;

					// connectionAY = roomA->Height() - connectionAY - 1;
					// connectionBY = roomB->Height() - connectionBY - 1;

					// int connectionA = roomA->getRoomEntranceId(Vector2i(connectionAX, connectionAY));
					// int connectionB = roomB->getRoomEntranceId(Vector2i(connectionBX, connectionBY));

					// if (connectionA == -1 || connectionB == -1) {
					// 	std::cout << "Failed to load connection from " << roomAName << " to " << roomBName << std::endl;
					// 	std::cout << "\t" << connectionAX << ", " << connectionAY << " - " << connectionA << std::endl;
					// 	std::cout << "\t" << connectionBX << ", " << connectionBY << " - " << connectionB << std::endl;
					// 	continue;
					// }

					// roomA->connect(roomB, connectionA);
					// roomB->connect(roomA, connectionB);

					// Connection *connection = new Connection(roomA, connectionA, roomB, connectionB);
					// connections.push_back(connection);
				} else {
					std::string roomName = toLower(line.substr(0, line.find(':')));

					std::string roomPath = directory.string();
					replaceLastInstance(roomPath, toLower(worldAcronym), roomsDirectory);
					roomPath = (std::filesystem::path(roomPath) / roomName).string();

					if (startsWith(roomName, "gate")) {
						replaceLastInstance(roomPath, roomsDirectory, "gates");
						// std::cout << "Found gate " << roomName << std::endl;
					}

					Room *room = nullptr;

					if (startsWith(roomName, "offscreenden")) {
						if (offscreenDen == nullptr) {
							offscreenDen = new OffscreenRoom(roomName, roomName);
							rooms.push_back(offscreenDen);
							room = offscreenDen;
						} else {
							room = offscreenDen;
						}
					} else {
						room = new Room(roomPath + ".txt", roomName);
						rooms.push_back(room);
					}

					Vector2 &position = room->Position();

					std::string temp;
					std::stringstream data(line.substr(line.find(':') + 1));

					double scale = 1.0 / 3.0;

					std::getline(data, temp, '>'); // Canon X
					double x = std::stod(temp) * scale;

					std::getline(data, temp, '<');
					std::getline(data, temp, '>'); // Canon Y
					double y = std::stod(temp) * scale;

					std::getline(data, temp, '<'); // Dev X
					std::getline(data, temp, '<'); // Dev Y

					std::getline(data, temp, '<');
					std::getline(data, temp, '>'); // Layer
					int layer = temp.empty() ? 0 : std::stoi(temp);
					
					std::getline(data, temp, '<');
					std::getline(data, temp, '>'); // Subregion
					std::string subregion = temp;

					position.x = x - room->Width() * 0.5;
					position.y = y + room->Height() * 0.5;
					room->layer = layer;
					
					// Backwards-Compatibility
					if (layer >= LAYER_HIDDEN && layer <= LAYER_HIDDEN + 2) {
						room->data.hidden = true;
						room->layer = layer - LAYER_HIDDEN;
					}

					if (subregion.empty()) {
						room->subregion = -1;
					} else {
						auto it = std::find(subregions.begin(), subregions.end(), subregion);
						if (it == subregions.end()) {
							subregions.push_back(subregion);
							it = std::find(subregions.begin(), subregions.end(), subregion);
						}

						room->subregion = std::distance(subregions.begin(), it);
					}
				}
			}
			mapFile.close();
			
			for (const auto &[oRoomName, extraRoomData] : extraRoomData) {
				std::string roomName = toLower(oRoomName);

				for (Room *room : rooms) {
					if (room->roomName == roomName) {
						room->data = extraRoomData;
						break;
					}
				}
			}
		}

		static std::tuple<std::string, std::vector<std::string>, std::vector<std::string>> parseRoomString(const std::string &input) {
			std::vector<std::string> connections;
			std::vector<std::string> flags;
			std::string roomName = "";

			auto colonSplit = split(input, ':');
			roomName = colonSplit[0];

			auto commaSplit = split(colonSplit[1], ',');
			for (const auto &item : commaSplit) {
				connections.push_back(item);
			}

			for (int i = 2; i < colonSplit.size(); i++) {
				flags.push_back(colonSplit[i]);
			}

			return {
				roomName,
				connections,
				flags
			};
		}

		static void parseWorldRoom(std::string line, std::filesystem::path directory, std::vector<Quadruple<Room*, int, std::string, int>> &connectionsToAdd) {
			std::tuple<std::string, std::vector<std::string>, std::vector<std::string>> parts = parseRoomString(line);

			std::string roomName = toLower(std::get<0>(parts));

			Room *room = nullptr;
			for (Room *otherRoom : rooms) {
				if (toLower(otherRoom->roomName) == roomName) {
					room = otherRoom;
					break;
				}
			}

			if (room == nullptr) {
				if (startsWith(roomName, "offscreenden")) {
					room = new OffscreenRoom(roomName, roomName);
				} else {
					std::string roomPath = directory.string();
					replaceLastInstance(roomPath, worldAcronym, roomsDirectory);
		
					if (startsWith(roomName, "gate")) {
						replaceLastInstance(roomPath, roomsDirectory, "gates");
					}

					roomPath = findFileCaseInsensitive(roomPath, roomName + ".txt");

					room = new Room(roomPath, roomName);
				}

				rooms.push_back(room);
			}

			int connectionId = 0;
			for (std::string connection : std::get<1>(parts)) {
				connection = toLower(connection);
				if (connection == "disconnected") {
					connectionId++;
					continue;
				}

				bool alreadyExists = false;
				for (Quadruple<Room*, int, std::string, int> &connectionData : connectionsToAdd) {
					if (toLower(connectionData.first->roomName) == connection && connectionData.third == toLower(roomName)) {
						connectionData.fourth = connectionId;
						alreadyExists = true;
						break;
					}
				}
				if (alreadyExists) {
					connectionId++;
					continue;
				}

				connectionsToAdd.push_back(Quadruple<Room*, int, std::string, int> {
					room,
					connectionId,
					connection,
					-1
				});

				connectionId++;
			}

			room->Tag("");
			for (std::string tag : std::get<2>(parts)) {
				room->ToggleTag(tag);
			}
		}

		static void parseWorldCreature(std::string line) {
			std::vector<std::string> splits = split(line, ':');

			if (splits[0] == "LINEAGE" || splits[0][0] == '(') {
				// std::cout << "Skipping parsing '" << line << "'\n";
				return;
			}

			std::string roomName = toLower(splits[0]);
			Room *room = nullptr;

			for (Room *otherRoom : rooms) {
				if (toLower(otherRoom->roomName) == roomName) {
					room = otherRoom;
					break;
				}
			}

			if (roomName == "offscreen") {
				room = offscreenDen;
			}

			if (room == nullptr) return;

			for (std::string creatureInDen : split(splits[1], ',')) {
				std::vector<std::string> sections = split(creatureInDen, '-');
				int denId = std::stoi(sections[0]);
				std::string creature = sections[1];

				if (room == offscreenDen) {
					denId = offscreenDen->DenCount();
					offscreenDen->AddDen();
				}

				if (!room->CreatureDenExists(denId)) {
					std::cout << (roomName + " failed den " + std::to_string(denId)) << std::endl;
					FailureController::fails.push_back(roomName + " failed den " + std::to_string(denId));
					continue;
				}

				Den &den = room->CreatureDen(denId);
				den.type = CreatureTextures::parse(creature);

				if (sections.size() == 3) {
					if (sections[2][0] == '{') {
						std::string tag = sections[2].substr(1, sections[2].size() - 2);
						if (startsWith(tag, "Mean")) {
							den.tag = "MEAN";
							den.data = std::stod(tag.substr(tag.find_first_of(':') + 1));
						} else if (startsWith(tag, "Seed")) {
							den.tag = "SEED";
							den.data = std::stod(tag.substr(tag.find_first_of(':') + 1));
						} else if (tag.find(':') != -1) {
							den.tag = "LENGTH";
							den.data = std::stod(tag.substr(tag.find_first_of(':') + 1));
						} else {
							den.tag = tag;
						}
						den.count = 1;
					} else {
						den.count = std::stoi(sections[2]);
					}
				} else if (sections.size() == 4) {
					std::string tagString = "";
					std::string countString = "";
					if (sections[2][0] == '{') {
						tagString = sections[2];
						countString = sections[3];
					} else {
						countString = sections[2];
						tagString = sections[3];
					}

					std::string tag = tagString.substr(1, tagString.size() - 2);
					if (startsWith(tag, "Mean")) {
						den.tag = "MEAN";
						den.data = std::stod(tag.substr(tag.find_first_of(':') + 1));
					} else if (startsWith(tag, "Seed")) {
						den.tag = "SEED";
						den.data = std::stod(tag.substr(tag.find_first_of(':') + 1));
					} else if (tag.find(':') != -1) {
						den.tag = "LENGTH";
						den.data = std::stod(tag.substr(tag.find_first_of(':') + 1));
					} else {
						den.tag = tag;
					}
					den.count = std::stoi(countString);
				} else {
					den.count = 1;
				}
			}
		}

		static void parseWorld(std::filesystem::path worldFilePath, std::filesystem::path directory) {
			std::fstream worldFile(worldFilePath);

			std::vector<Quadruple<Room*, int, std::string, int>> connectionsToAdd;

			int parseState = 0;
			std::string line;
			while (std::getline(worldFile, line)) {
				if (line == "ROOMS") {
					parseState = 1;
					std::cout << "World - Rooms" << std::endl;
					continue;
				}

				if (line == "END ROOMS") {
					parseState = 2;
			
					if (offscreenDen == nullptr) {
						offscreenDen = new OffscreenRoom("offscreenden" + worldAcronym, "OffscreenDen" + toUpper(worldAcronym));
						rooms.push_back(offscreenDen);
					}

					continue;
				}

				if (line == "CREATURES") {
					parseState = 3;
					std::cout << "World - Creatures" << std::endl;
					continue;
				}

				if (line == "END CREATURES") {
					parseState = 4;
					continue;
				}

				if (parseState == 4) {
					extraWorld += line + "\n";
					continue;
				}

				if (line == "") continue;

				if (parseState == 1)
					parseWorldRoom(line, directory, connectionsToAdd);
				
				if (parseState == 3)
					parseWorldCreature(line);
			}
			worldFile.close();
			
			std::cout << "Loading connections" << std::endl;

			for (Quadruple<Room*, int, std::string, int> connectionData : connectionsToAdd) {
				if (connectionData.second == -1 || connectionData.fourth == -1) {
					std::cout << "Failed to load connection from " << connectionData.first->roomName << " to " << connectionData.third << std::endl;
					continue;
				}

				Room *roomA = connectionData.first;
				Room *roomB = nullptr;

				for (Room *room : rooms) {
					if (toLower(room->roomName) == connectionData.third) {
						roomB = room;
						break;
					}
				}

				if (roomB == nullptr) {
					std::cout << "Failed to load connection from " << roomA->roomName << " to " << connectionData.third << std::endl;
					FailureController::fails.push_back("Failed to load connection from " + roomA->roomName + " to " + connectionData.third);
					continue;
				}

				int connectionA = connectionData.second;
				int connectionB = connectionData.fourth;
				
				if (!roomA->canConnect(connectionA) || !roomB->canConnect(connectionB)) {
					std::cout << "Failed to load connection from " << roomA->roomName << " to " << connectionData.third << " - invalid room" << std::endl;
					FailureController::fails.push_back("Failed to load connection from " + roomA->roomName + " to " + connectionData.third + " - invalid room");
					continue;
				}

				roomA->connect(roomB, connectionA);
				roomB->connect(roomA, connectionB);

				Connection *connection = new Connection(roomA, connectionA, roomB, connectionB);
				connections.push_back(connection);
			}
			
			std::cout << "Connections loaded" << std::endl;
		}

		static void parseProperties(std::string propertiesFilePath) {
			std::fstream propertiesFile(propertiesFilePath);
			
			std::string line;
			while (std::getline(propertiesFile, line)) {
				if (startsWith(line, "Subregion: ")) {
					std::string subregionName = line.substr(line.find(':') + 2);
					std::cout << "Subregion: " << subregionName << std::endl;
					subregions.push_back(subregionName);
				} else {
					extraProperties += line + "\n";
				}
			}

			propertiesFile.close();
		}

		static void loadExtraRoomData(std::string roomPath, ExtraRoomData &data) {
			if (roomPath == "") return;
		
			std::fstream file(roomPath, std::ios::in | std::ios::binary);

			if (!file.is_open()) return;

			std::string line;
			while (std::getline(file, line)) {
				if (!startsWith(line, "PlacedObjects:")) continue;
				
				std::vector<std::string> splits = split(line.substr(line.find(':') + 1), ',');
				
				for (std::string item : splits) {
					if (item[0] == ' ') item = item.substr(1); // Remove space
					if (item.size() <= 1) continue;
					
					std::vector<std::string> splits2 = split(item, '>');
					std::string key = splits2[0];
					
					GLuint texture = CreatureTextures::getTexture("room-" + key);
					if (texture == CreatureTextures::UNKNOWN) continue;

					DevItem devItem = DevItem();
					devItem.name = key;
					devItem.position = Vector2(std::stod(splits2[1].substr(1)) / 20.0, std::stod(splits2[2].substr(1)) / 20.0);
					devItem.texture = texture;
					data.devItems.push_back(devItem);
				}
			}

			file.close();
		}

		static void exportMapFile() {
			std::fstream file(exportDirectory / ("map_" + worldAcronym + ".txt"), std::ios::out | std::ios::trunc);

			if (!file.is_open()) { std::cout << "Error opening map_" << worldAcronym << ".txt\n"; return; }

			std::cout << "Exporting rooms" << std::endl;
			for (Room *room : rooms) {
				const Vector2 &roomPosition = room->Position();
				Vector2 position = Vector2(
					(roomPosition.x + room->Width() * 0.5) * 3.0,
					(roomPosition.y - room->Height() * 0.5) * 3.0
				);

				file << std::setprecision(12);
				file << toUpper(room->roomName) << ": ";
				file << position.x << "><" << position.y << "><"; // Canon Position
				file << position.x << "><" << position.y << "><"; // Dev Position
				file << room->layer << "><";
				if (room->subregion > -1) file << subregions[room->subregion];
				file << "\n";
			}
			
			std::cout << "Exporting extra data" << std::endl;
			for (Room *room : rooms) {
				file << "//FloodForge;ROOM|" << room->roomName;
				if (room->data.hidden) file << "|hidden";
				if (room->data.merge) file << "|merge";
				file << "\n";
			}

			std::cout << "Exporting connections" << std::endl;
			for (Connection *connection : connections) {
				if (connection->RoomA()->data.hidden || connection->RoomB()->data.hidden) continue;

				Vector2i connectionA = connection->RoomA()->getRoomEntrancePosition(connection->ConnectionA());
				Vector2i connectionB = connection->RoomB()->getRoomEntrancePosition(connection->ConnectionB());

				connectionA = Vector2i(
					connectionA.x,
					connection->RoomA()->Height() - connectionA.y - 1
				);
				connectionB = Vector2i(
					connectionB.x,
					connection->RoomB()->Height() - connectionB.y - 1
				);

				file << "Connection: ";
				file << toUpper(connection->RoomA()->roomName) << ",";
				file << toUpper(connection->RoomB()->roomName) << ",";
				file << connectionA.x << "," << connectionA.y << ",";
				file << connectionB.x << "," << connectionB.y << ",";
				file << connection->RoomA()->getRoomEntranceDirection(connection->ConnectionA()) << ",";
				file << connection->RoomB()->getRoomEntranceDirection(connection->ConnectionB());
				file << "\n";
			}

			file.close();
		}

		static void exportWorldFile() {
			std::fstream file(exportDirectory / ("world_" + worldAcronym + ".txt"), std::ios::out | std::ios::trunc);

			if (!file.is_open()) { std::cout << "Error opening world_" << worldAcronym << ".txt\n"; return; }

			file << "ROOMS\n";
			for (Room *room : rooms) {
				if (room->isOffscreen()) continue;

				file << toUpper(room->roomName) << " : ";

				std::vector<std::string> connections(room->RoomEntranceCount(), "DISCONNECTED");

				for (std::pair<Room*, unsigned int> connection : room->RoomConnections()) {
					connections[connection.second] = toUpper(connection.first->roomName);
				}

				for (int i = 0; i < room->RoomEntranceCount(); i++) {
					if (i > 0) file << ", ";

					file << connections[i];
				}

				for (std::string tag : room->Tags()) {
					file << " : " << tag;
				}
				// if (room->Tag() != "") file << " : " << room->Tag();

				file << "\n";
			}
			file << "END ROOMS\n\n";

			file << "CREATURES\n";

			for (Room *room : rooms) {
				std::stringstream line;
				bool add = false;
				
				if (room == offscreenDen) {
					line << "OFFSCREEN : ";
				} else {
					line << toUpper(room->roomName) << " : ";
				}

				for (int i = 0; i < room->DenCount(); i++) {
					const Den &den = room->CreatureDen01(i);
					if (den.type.empty() || den.count == 0)
						continue;

					if (i > 0) line << ", ";
					if (room == offscreenDen) {
						line << "0-" << den.type;
					} else {
						line << (i + room->RoomEntranceCount()) << "-" << den.type;
					}
					if (!den.tag.empty()) {
						if (den.tag == "MEAN") {
							line << "-{Mean:" << den.data << "}";
						} else if (den.tag == "LENGTH") {
							if (den.type == "PoleMimic") {
								line << "-{" << int(den.data) << "}";
							} else {
								line << "-{" << den.data << "}";
							}
						} else if (den.tag == "SEED") {
							line << "-{Seed:" << int(den.data) << "}";
						} else {
							line << "-{" << den.tag << "}";
						}
					}
					if (den.count > 1) line << "-" << den.count;
					add = true;
				}

				if (add) file << line.str() << "\n";
			}

			file << "END CREATURES\n";

			file << extraWorld;

			file.close();
		}

		static void exportImageFile(std::filesystem::path outputPath, std::filesystem::path otherPath) {
			Rect bounds;

			for (Room *room : rooms) {
				if (room->data.hidden) continue;

				double left   = room->Position().x;
				double right  = room->Position().x + room->Width();
				double top    = -room->Position().y + room->Height();
				double bottom = -room->Position().y;
				bounds.X0(std::min(bounds.X0(), left));
				bounds.X1(std::max(bounds.X1(), right));
				bounds.Y0(std::min(bounds.Y0(), bottom));
				bounds.Y1(std::max(bounds.Y1(), top));
			}

			const int padding = 10;

			const int width = std::floor(bounds.X1() - bounds.X0() + padding * 2);
			const int height = std::floor(bounds.Y1() - bounds.Y0() + padding * 2) * 3;

			std::vector<unsigned char> image(width * height * 3);

#ifdef VISIBLE_OUTPUT_PADDING
			for (int x = 0; x < width; x++) {
				for (int y = 0; y < height; y++) {
					int i = y * width + x;

					if (x < padding || (y % (height / 3)) < padding || x >= width - padding || (y % (height / 3)) >= height / 3 - padding) {
						image[i * 3 + 0] = 0;
						image[i * 3 + 1] = 255;
						image[i * 3 + 2] = 255;
					} else {
						image[i * 3 + 0] = 0;
						image[i * 3 + 1] = 255;
						image[i * 3 + 2] = 0;
					}
				}
			}
#else
			for (int i = 0; i < width * height; i++) {
				image[i * 3 + 0] = 0;
				image[i * 3 + 1] = 255;
				image[i * 3 + 2] = 0;
			}
#endif

			for (Room *room : rooms) {
				if (room->isOffscreen()) continue;
				if (room->data.hidden) continue;

				// Top left corner
				int x = std::floor(room->Position().x - room->Width() * 0.0 - bounds.X0()) + padding;
				int y = std::floor(-room->Position().y - room->Height() * 0.0 - bounds.Y0()) + padding;
				y += (2 - room->layer) * height / 3;

				for (int ox = 0; ox < room->Width(); ox++) {
					for (int oy = 0; oy < room->Height(); oy++) {
						int i = ((y + oy) * width + x + ox) * 3;

						unsigned int tileType = room->getTile(ox, oy) % 16;
						unsigned int tileData = room->getTile(ox, oy) / 16;

						int r = 0;
						int g = 0;
						int b = 0;

						if (tileType == 0) {
							r = 255; g = 0; // #FF0000
						}
						if (tileType == 1 || tileType == 4) {
							r = 0; g = 0; // #000000
						}
						if (tileType == 2 || tileType == 3 || tileData & 1 || tileData & 2) {
							r = 153; g = 0; // #990000
						}

						// Water
						if (r > 0) {
							if (oy >= room->Height() - room->water) b = 255; // #FF00FF or #9900FF
						}

						if (!room->data.merge || !(r == 0 && g == 0 && b == 0) || (image[i + 0] == 0 && image[i + 2] == 0)) {
							image[i + 0] = r;
							image[i + 1] = g;
							image[i + 2] = b;
						}
					}
				}
			}

			if (stbi_write_png(outputPath.string().c_str(), width, height, 3, image.data(), width * 3)) {
				std::cout << "Image saved successfully!" << std::endl;
			} else {
				std::cout << "Error saving image!" << std::endl;
			}
		}

		static void exportPropertiesFile(std::filesystem::path outputPath) {
			std::fstream propertiesFile(outputPath, std::ios::out | std::ios::trunc);
			
			propertiesFile << extraProperties;

			for (std::string subregion : subregions) {
				propertiesFile << "Subregion: " << subregion << "\n";
			}

			propertiesFile.close();
		}

		static void init(Window *window);

		static void cleanup() {
			for (Button *button : buttons) {
				delete button;
			}

			buttons.clear();
		}

		static void draw(Mouse *mouse, Vector2 screenBounds) {
			Draw::color(1.0, 1.0, 1.0);
			
			Draw::useTexture(MenuItems::textureBar);

			glEnable(GL_BLEND);
			glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
			Draw::begin(Draw::QUADS);
			Draw::texCoord(-screenBounds.x, 0.0f); Draw::vertex(-screenBounds.x, screenBounds.y);
			Draw::texCoord( screenBounds.x, 0.0f); Draw::vertex( screenBounds.x, screenBounds.y);
			Draw::texCoord( screenBounds.x, 1.0f); Draw::vertex( screenBounds.x, screenBounds.y - 0.09f);
			Draw::texCoord(-screenBounds.x, 1.0f); Draw::vertex(-screenBounds.x, screenBounds.y - 0.09f);
			Draw::end();
			glDisable(GL_BLEND);
			Draw::useTexture(0);

			glLineWidth(1);

			for (Button *button : buttons) {
				button->update(mouse, screenBounds);
				button->draw(mouse, screenBounds);
			}
		}

		static void WorldAcronym(std::string worldAcronym) {
			MenuItems::worldAcronym = worldAcronym;
		}

		static std::filesystem::path ExportDirectory() { return exportDirectory; }
		
		static std::string extraProperties;
		static std::string extraWorld;

		static GLuint textureButtonNormal;
		static GLuint textureButtonNormalHover;
		static GLuint textureButtonPress;
		static GLuint textureButtonPressHover;
		static GLuint textureBar;

	private:
		static void repositionButtons() {
			currentButtonX = 0.01;

			for (Button *button : buttons) {
				button->X(currentButtonX);

				currentButtonX += button->Width() + 0.04;
			}
		}
	
		static std::vector<Button*> buttons;

		static Window *window;

		static double currentButtonX;

		static std::filesystem::path exportDirectory;
		static std::string worldAcronym;
		static std::string roomsDirectory;
};