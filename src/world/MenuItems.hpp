#ifndef MENU_ITEMS_HPP
#define MENU_ITEMS_HPP

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

		Button *OnPress(std::function<void(Button*)> listener) {
			listeners.push_back(listener);

			return this;
		}

		bool isHovered(Mouse *mouse, Vector2 screenBounds) {
			double mouseX = mouse->X() / 512.0 + screenBounds.x - 1.0;
			double mouseY = -(mouse->Y() / 512.0 + screenBounds.y - 1.0);

			return mouseX >= (x - 0.005) && mouseX <= (x + 0.005) + width && mouseY <= (y + 0.005) && mouseY >= (y - 0.005) - height;
		}

		void update(Mouse *mouse, Vector2 screenBounds) {
			pressed = isHovered(mouse, screenBounds) && mouse->Left();

			if (isHovered(mouse, screenBounds) && mouse->JustLeft()) {
				press();
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

		const double Width() const {
			return width;
		}

	private:
		void press() {
			for (const auto &listener : listeners) {
				listener(this);
			}
		}

		std::vector<std::function<void(Button*)>> listeners;

		bool pressed = false;

		double x;
		double y;
		double width;
		double height;

		std::string text;
		Font *font;
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

		static void addButton(std::string text, std::function<void(Button*)> listener) {
			double x = currentButtonX;
			double width = Fonts::rainworld->getTextWidth(text, 0.03) + 0.02;

			currentButtonX += width + 0.04;
			buttons.push_back((new Button(text, x, -0.01, width, 0.04, Fonts::rainworld))->OnPress(listener));
		}

		static void replaceLastInstance(std::string& str, const std::string& old_sub, const std::string& new_sub) {
		    size_t pos = str.rfind(old_sub);
		    
		    if (pos != std::string::npos) {
		        str.replace(pos, old_sub.length(), new_sub);
		    }
		}

		static void parseMap(std::filesystem::path mapFilePath, std::filesystem::path directory) {
			std::fstream mapFile(mapFilePath);

			std::string line;
			while (std::getline(mapFile, line)) {
				if (startsWith(line, "Connection: ")) {
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

					// int connectionA = roomA->getShortcutConnection(Vector2i(connectionAX, connectionAY));
					// int connectionB = roomB->getShortcutConnection(Vector2i(connectionBX, connectionBY));

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
					replaceLastInstance(roomPath, toLower(worldAcronym), toLower(worldAcronym) + "-rooms");
					roomPath = (std::filesystem::path(roomPath) / roomName).string();

					if (startsWith(roomName, "gate")) {
						replaceLastInstance(roomPath, worldAcronym + "-rooms", "gates");
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
						room = new Room(roomPath, roomName);
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
					int layer = std::stoi(temp);
					
					std::getline(data, temp, '<');
					std::getline(data, temp, '>'); // Subregion
					std::string subregion = temp;

					position.x = x - room->Width() * 0.5;
					position.y = y + room->Height() * 0.5;
					room->Layer(layer);
					if (layer >= LAYER_HIDDEN && layer <= LAYER_HIDDEN + 2) {
						room->Hidden(true);
						room->Layer(layer - LAYER_HIDDEN);
					}

					if (subregion.empty()) {
						room->Subregion(-1);
					} else {
						auto it = std::find(subregions.begin(), subregions.end(), subregion);
						if (it == subregions.end()) {
							subregions.push_back(subregion);
							it = std::find(subregions.begin(), subregions.end(), subregion);
						}

						room->Subregion(std::distance(subregions.begin(), it));
					}
				}
			}
			mapFile.close();
		}

		static std::vector<std::string> split(const std::string &text, char delimiter) {
			std::vector<std::string> tokens;
			std::string token;
			std::istringstream tokenStream(text);

			while (std::getline(tokenStream, token, delimiter)) {
				token.erase(0, token.find_first_not_of(" \t\n"));
				token.erase(token.find_last_not_of(" \t\n") + 1);
				tokens.push_back(token);
			}

			return tokens;
		}

		static std::tuple<std::string, std::vector<std::string>, std::string> parseRoomString(const std::string &input) {
			std::vector<std::string> connections;
			std::string flag = "";
			std::string roomName = "";

			auto colonSplit = split(input, ':');
			roomName = colonSplit[0];

			auto commaSplit = split(colonSplit[1], ',');
			for (const auto &item : commaSplit) {
				connections.push_back(item);
			}

			if (colonSplit.size() > 2) flag = colonSplit[2];

			return std::tuple<std::string, std::vector<std::string>, std::string> {
				roomName,
				connections,
				flag
			};
		}

		static void parseWorldRoom(std::string line, std::filesystem::path directory, std::vector<Quadruple<Room*, int, std::string, int>> &connectionsToAdd) {
			std::tuple<std::string, std::vector<std::string>, std::string> parts = parseRoomString(line);

			std::string roomName = toLower(std::get<0>(parts));

			std::string roomPath = directory.string();
			replaceLastInstance(roomPath, worldAcronym, worldAcronym + "-rooms");
			roomPath = (std::filesystem::path(roomPath) / roomName).string();

			if (startsWith(roomName, "gate")) {
				replaceLastInstance(roomPath, worldAcronym + "-rooms", "gates");
			}

			Room *room = nullptr;
			for (Room *otherRoom : rooms) {
				if (toLower(otherRoom->RoomName()) == roomName) {
					room = otherRoom;
					break;
				}
			}

			if (room == nullptr) {
				if (startsWith(roomName, "offscreenden")) {
					room = new OffscreenRoom(roomName, roomName);
				} else {
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
					if (toLower(connectionData.first->RoomName()) == connection && connectionData.third == toLower(roomName)) {
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

			room->Tag(std::get<2>(parts));
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
				if (toLower(otherRoom->RoomName()) == roomName) {
					room = otherRoom;
					break;
				}
			}

			if (roomName == "offscreen") {
				room = offscreenDen;
				// std::cout << offscreenDen << std::endl;
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
					den.count = std::stoi(sections[3]);
				} else {
					den.count = 1;
				}
			}
		}

		static void parseWorld(std::filesystem::path worldFilePath, std::filesystem::path directory) {
			std::fstream worldFile(worldFilePath);

			std::vector<Quadruple<Room*, int, std::string, int>> connectionsToAdd;

			int parseState = 0;
			// bool inRooms = false;
			// bool outOfRooms = false;
			std::string line;
			while (std::getline(worldFile, line)) {
				if (line == "ROOMS") {
					parseState = 1;
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

			for (Quadruple<Room*, int, std::string, int> connectionData : connectionsToAdd) {
				if (connectionData.second == -1 || connectionData.fourth == -1) {
					std::cout << "Failed to load connection from " << connectionData.first->RoomName() << " to " << connectionData.third << std::endl;
					continue;
				}

				Room *roomA = connectionData.first;
				Room *roomB = nullptr;

				for (Room *room : rooms) {
					if (toLower(room->RoomName()) == connectionData.third) {
						roomB = room;
						break;
					}
				}

				if (roomB == nullptr) {
					std::cout << "Failed to load connection from " << roomA->RoomName() << " to " << connectionData.third << std::endl;
					continue;
				}

				int connectionA = connectionData.second;
				int connectionB = connectionData.fourth;

				roomA->connect(roomB, connectionA);
				roomB->connect(roomA, connectionB);

				Connection *connection = new Connection(roomA, connectionA, roomB, connectionB);
				connections.push_back(connection);
			}
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
				file << toUpper(room->RoomName()) << ": ";
				file << position.x << "><" << position.y << "><"; // Canon Position
				file << position.x << "><" << position.y << "><"; // Dev Position
				if (room->Hidden()) {
					file << (LAYER_HIDDEN + room->Layer()) << "><";
				} else {
					file << room->Layer() << "><";
				}
				if (room->Subregion() > -1) file << subregions[room->Subregion()];
				file << "\n";
			}

			std::cout << "Exporting connections" << std::endl;
			for (Connection *connection : connections) {
				if (connection->RoomA()->Hidden() || connection->RoomB()->Hidden()) continue;

				Vector2i connectionA = connection->RoomA()->getShortcutConnection(connection->ConnectionA());
				Vector2i connectionB = connection->RoomB()->getShortcutConnection(connection->ConnectionB());

				connectionA = Vector2i(
					connectionA.x,
					connection->RoomA()->Height() - connectionA.y - 1
				);
				connectionB = Vector2i(
					connectionB.x,
					connection->RoomB()->Height() - connectionB.y - 1
				);

				file << "Connection: ";
				file << toUpper(connection->RoomA()->RoomName()) << ",";
				file << toUpper(connection->RoomB()->RoomName()) << ",";
				file << connectionA.x << "," << connectionA.y << ",";
				file << connectionB.x << "," << connectionB.y << ",";
				file << connection->RoomA()->getShortcutDirection(connection->ConnectionA()) << ",";
				file << connection->RoomB()->getShortcutDirection(connection->ConnectionB());
				file << "\n";
			}

			file.close();
		}

		static void exportWorldFile() {
			std::fstream file(exportDirectory / ("world_" + worldAcronym + ".txt"), std::ios::out | std::ios::trunc);

			if (!file.is_open()) { std::cout << "Error opening world_" << worldAcronym << ".txt\n"; return; }

			file << "ROOMS\n";
			for (Room *room : rooms) {
				if (room->Tag() == "OffscreenRoom") continue;

				file << toUpper(room->RoomName()) << " : ";

				std::vector<std::string> connections(room->ConnectionCount(), "DISCONNECTED");

				for (std::pair<Room*, unsigned int> connection : room->RoomConnections()) {
					connections[connection.second] = toUpper(connection.first->RoomName());
				}

				for (int i = 0; i < room->ConnectionCount(); i++) {
					if (i > 0) file << ", ";

					file << connections[i];
				}

				if (room->Tag() != "") file << " : " << room->Tag();

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
					line << toUpper(room->RoomName()) << " : ";
				}

				for (int i = 0; i < room->DenCount(); i++) {
					const Den &den = room->CreatureDen01(i);
					if (den.type.empty() || den.count == 0)
						continue;

					if (i > 0) line << ", ";
					if (room == offscreenDen) {
						line << "0-" << den.type;
					} else {
						line << (i + room->ConnectionCount()) << "-" << den.type;
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
				if (room->Hidden()) continue;

				double left   = room->Position().x;
				double right  = room->Position().x + room->Width();
				double top    = -room->Position().y + room->Height();
				double bottom = -room->Position().y;
				bounds.X0(std::min(bounds.X0(), left));
				bounds.X1(std::max(bounds.X1(), right));
				bounds.Y0(std::min(bounds.Y0(), bottom));
				bounds.Y1(std::max(bounds.Y1(), top));
			}

			// std::cout << bounds.X0() << std::endl;
			// std::cout << bounds.X1() << std::endl;
			// std::cout << bounds.Y0() << std::endl;
			// std::cout << bounds.Y1() << std::endl;

			const int padding = 10;

			const int width = std::floor(bounds.X1() - bounds.X0() + padding * 2);
			const int height = std::floor(bounds.Y1() - bounds.Y0() + padding * 2) * 3;

			// std::cout << width << std::endl;
			// std::cout << height << std::endl;

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
				if (room->Tag() == "OffscreenRoom") continue;
				if (room->Hidden()) continue;

				// Top left corner
				int x = std::floor(room->Position().x - room->Width() * 0.0 - bounds.X0()) + padding;
				int y = std::floor(-room->Position().y - room->Height() * 0.0 - bounds.Y0()) + padding;
				y += (2 - room->Layer()) * height / 3;

				for (int ox = 0; ox < room->Width(); ox++) {
					for (int oy = 0; oy < room->Height(); oy++) {
						unsigned int tileType = room->getTile(ox, oy) % 16;
						unsigned int tileData = room->getTile(ox, oy) / 16;

						int r = 0;
						int g = 0;
						int b = 0;

						if (tileType == 0 || (tileType == 4 && room->getShortcutConnection(Vector2i(ox, oy)) != -1)) {
							r = 255; g = 0; b = 0; // #FF0000
						}
						if (tileType == 1 || tileType == 2 || (tileType == 4 && r == 0)) {
							r = 0; g = 0; b = 0; // #000000
						}
						if (tileType == 3 || tileData & 1 || tileData & 2) {
							r = 153; g = 0; b = 0; // #990000
						}

						// Water
						if (r > 0) {
							if (oy >= room->Height() - room->Water()) b = 255; // #FF00FF or #9900FF
						}

						image[((y + oy) * width + x + ox) * 3 + 0] = r;
						image[((y + oy) * width + x + ox) * 3 + 1] = g;
						image[((y + oy) * width + x + ox) * 3 + 2] = b;
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
};

#endif