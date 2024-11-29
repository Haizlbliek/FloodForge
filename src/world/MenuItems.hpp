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

#include "../font/Fonts.hpp"
#include "../math/Quadruple.hpp"

#include "../Utils.hpp"
#include "../Window.hpp"
#include "../Theme.hpp"

#include "Globals.hpp"
#include "Room.hpp"
#include "OffscreenRoom.hpp"
#include "Popups.hpp"
#include "AcronymWindow.hpp"
#include "WarningPopup.hpp"

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

		bool isHovered(Mouse *mouse) {
			double mouseX = mouse->X() / 512.0 - 1.0;
			double mouseY = -(mouse->Y() / 512.0 - 1.0);

			return mouseX >= x && mouseX <= x + width && mouseY <= y && mouseY >= y - height;
		}

		void update(Mouse *mouse) {
			if (isHovered(mouse) && mouse->Left()) {
				if (!lastPressed) {
					lastPressed = true;
					press();
				}
			} else {
				lastPressed = false;
			}
		}

		void draw(Mouse *mouse) {
			setThemeColour(THEME_TEXT_COLOUR);
			font->write(text, x, y, height);

			if (isHovered(mouse)) {
				setThemeColour(THEME_BORDER_HIGHLIGHT_COLOUR);
				strokeRect(x - 0.005, y + 0.005, x + 0.005 + width, y - 0.005 - height);
			}
		}

		void Text(std::string text) {
			this->text = text;
			width = font->getTextWidth(text, height);
		}

		std::string Text() { return text; }

	private:
		void press() {
			for (const auto &listener : listeners) {
				listener(this);
			}
		}

		std::vector<std::function<void(Button*)>> listeners;

		double x;
		double y;
		double width;
		double height;

		std::string text;
		Font *font;


		bool lastPressed = false;
};

class MenuItems {
	public:
		static void addButton(std::string text, std::function<void(Button*)> listener) {
			double x = currentButtonX;
			double width = Fonts::rainworld->getTextWidth(text, 0.04);

			currentButtonX += width + 0.04;
			buttons.push_back((new Button(text, x, 0.99, width, 0.04, Fonts::rainworld))->OnPress(listener));
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

					Room *room;

					if (startsWith(roomName, "offscreenden")) {
						room = new OffscreenRoom(roomName, roomName);
					} else {
						room = new Room(roomPath, roomName);
					}

					rooms.push_back(room);

					Vector2 *position = room->Position();

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

					position->x = x - room->Width() * 0.5;
					position->y = y + room->Height() * 0.5;
					room->Layer(layer);

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

		static void parseWorld(std::filesystem::path worldFilePath, std::filesystem::path directory) {
			std::fstream worldFile(worldFilePath);

			std::vector<Quadruple<Room*, int, std::string, int>> connectionsToAdd;

			bool inRooms = false;
			bool outOfRooms = false;
			std::string line;
			while (std::getline(worldFile, line)) {
				if (line == "ROOMS") {
					inRooms = true;
					continue;
				}
				if (line == "END ROOMS") {
					outOfRooms = true;
					continue;
				}

				if (outOfRooms) {
					extraWorld += line + "\n";
					continue;
				}

				if (line == "") continue;
				if (!inRooms) continue;

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

				// std::cout << "Connecting " << roomA->RoomName() << " - " << connectionA << " to " << roomB->RoomName() << " - " << connectionB << std::endl;

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

			for (Room *room : rooms) {
				Vector2 *roomPosition = room->Position();
				Vector2 position = Vector2(
					(roomPosition->x + room->Width() * 0.5) * 3.0,
					(roomPosition->y - room->Height() * 0.5) * 3.0
				);

				file << std::setprecision(12);
				file << toUpper(room->RoomName()) << ": ";
				file << position.x << "><" << position.y << "><"; // Canon Position
				file << position.x << "><" << position.y << "><"; // Dev Position
				file << room->Layer() << "><";
				file << subregions[room->Subregion()];
				file << "\n";
			}

			for (Connection *connection : connections) {
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
			file << "END ROOMS\n";

			file.close();
		}

		static void exportImageFile(std::filesystem::path outputPath, std::filesystem::path otherPath) {
			Rect bounds;

			for (Room *room : rooms) {
				double left   = room->Position()->x;
				double right  = room->Position()->x + room->Width();
				double top    = -room->Position()->y + room->Height();
				double bottom = -room->Position()->y;
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

				// Top left corner
				int x = std::floor(room->Position()->x - room->Width() * 0.0 - bounds.X0()) + padding;
				int y = std::floor(-room->Position()->y - room->Height() * 0.0 - bounds.Y0()) + padding;
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

/*
		static void compareImages(std::filesystem::path imageA, std::filesystem::path imageB, std::filesystem::path outputPath) {
			std::cout << "Image A Path: " << imageA << std::endl;
			std::cout << "Image B Path: " << imageB << std::endl;
			std::cout << "Output Path: " << outputPath << std::endl;

		    // Load the first image (Image A)
		    int widthA, heightA, channelsA;
		    unsigned char* imgA = stbi_load(imageA.string().c_str(), &widthA, &heightA, &channelsA, 0); // Let stb choose the number of channels
		    if (!imgA) {
		        std::cerr << "Failed to load image: " << imageA << std::endl;
		        return;
		    }

		    // Load the second image (Image B)
		    int widthB, heightB, channelsB;
		    unsigned char* imgB = stbi_load(imageB.string().c_str(), &widthB, &heightB, &channelsB, 0); // Let stb choose the number of channels
		    if (!imgB) {
		        std::cerr << "Failed to load image: " << imageB << std::endl;
		        stbi_image_free(imgA);
		        return;
		    }

		    // Ensure the images have the same dimensions
		    if (widthA != widthB || heightA != heightB) {
		        std::cerr << "Images have different sizes!" << std::endl;
		        stbi_image_free(imgA);
		        stbi_image_free(imgB);
		        return;
		    }



		    // Adjust the channel count if necessary
		    // if (channelsA == 4 && channelsB == 3) {
		    // 	std::cout << "Adjusting 1" << std::endl;
		    //     // Image A has 4 channels (RGBA), Image B has 3 channels (RGB)
		    //     // Convert Image A (RGBA) to RGB by discarding the alpha channel
		    //     std::vector<unsigned char> imgA_RGB(widthA * heightA * 3);
		    //     for (int i = 0; i < widthA * heightA; ++i) {
		    //         imgA_RGB[i * 3 + 0] = imgA[i * 4 + 0];  // Red
		    //         imgA_RGB[i * 3 + 1] = imgA[i * 4 + 1];  // Green
		    //         imgA_RGB[i * 3 + 2] = imgA[i * 4 + 2];  // Blue
		    //     }
		    //     imgA = imgA_RGB.data();  // Use the converted data
		    //     channelsA = 3;  // Update the channel count to 3 (RGB)
		    // } else if (channelsA == 3 && channelsB == 4) {
		    // 	std::cout << "Adjusting 2" << std::endl;
		    //     // Image A has 3 channels (RGB), Image B has 4 channels (RGBA)
		    //     // Convert Image A (RGB) to RGBA by adding an alpha channel (255)
		    //     std::vector<unsigned char> imgA_RGBA(widthA * heightA * 4);
		    //     for (int i = 0; i < widthA * heightA; ++i) {
		    //         imgA_RGBA[i * 4 + 0] = imgA[i * 3 + 0];  // Red
		    //         imgA_RGBA[i * 4 + 1] = imgA[i * 3 + 1];  // Green
		    //         imgA_RGBA[i * 4 + 2] = imgA[i * 3 + 2];  // Blue
		    //         imgA_RGBA[i * 4 + 3] = 255;               // Alpha (fully opaque)
		    //     }
		    //     imgA = imgA_RGBA.data();  // Use the converted data
		    //     channelsA = 4;  // Update the channel count to 4 (RGBA)
		    // }

		    // if (channelsA != channelsB) {
		    //     std::cerr << "Images have incompatible channel counts!" << std::endl;
		    //     stbi_image_free(imgA);
		    //     stbi_image_free(imgB);
		    //     return;
		    // }

		    std::cout << "Creating diff file" << std::endl;

		    // Create a new image for the difference (black-and-white)
		    std::vector<unsigned char> outputImage(widthA * heightA * 3, 0);  // B/W, so we have R=G=B for each pixel

		    bool same = true;
		    for (int y = 0; y < heightA; ++y) {
		        for (int x = 0; x < widthA; ++x) {
		            int idx = (y * widthA + x) * 3;
		            bool isDifferent = false;

		            // Compare the pixels (ignore alpha channel for comparison)
		            for (int c = 0; c < 3; ++c) {
		            	// std::cout << "Calculating Value A\n";
		            	int valueA = imgA[(y * widthA + x) * channelsA + c];
		            	// std::cout << "Calculating Value B\n";
		            	int valueB = imgB[(y * widthB + x) * channelsB + c];
		            	// std::cout << "Calculated\n";
		                if (valueB != valueB) {
		                    isDifferent = true;
		                    break;
		                }
		            }

		            // Set the corresponding output pixel to white (255) if different, black (0) otherwise
		            if (isDifferent) {
		            	same = false;
		                outputImage[idx] = 255;   // Red channel (white)
		                outputImage[idx + 1] = 255; // Green channel (white)
		                outputImage[idx + 2] = 255; // Blue channel (white)
		            }
		            else {
		                outputImage[idx] = 0;   // Red channel (black)
		                outputImage[idx + 1] = 0; // Green channel (black)
		                outputImage[idx + 2] = 0; // Blue channel (black)
		            }
		        }
		    }

		    std::cout << "Writing diff file" << std::endl;

		    // Save the output image
		    if (stbi_write_png(outputPath.string().c_str(), widthA, heightA, 3, outputImage.data(), widthA * 3)) {
		        std::cout << "Comparison image saved to " << outputPath << std::endl;
		    } else {
		        std::cerr << "Failed to save the output image." << std::endl;
		    }

		    std::cout << (same ? "Accurate" : "Not Accurate") << std::endl;

		    // Free memory
		    stbi_image_free(imgA);
		    stbi_image_free(imgB);
		}
*/

		static void init(Window *window) {
			MenuItems::window = window;
			worldAcronym = "";

			addButton("New",
				[window](Button *button) {
					addPopup(new AcronymWindow(window));
				}
			);

			addButton("Add Room",
				[window](Button *button) {
					if (worldAcronym == "") {
						addPopup(new WarningPopup(window, "You must create or import a region\nbefore adding rooms."));
						return;
					}

					std::string pathString = OpenFileDialog("RW Room File (XX_ROOM.txt)\0*.txt\0");

					if (pathString == "") return;

					std::filesystem::path path = pathString;

					std::string roomName = path.filename().string();
					roomName = roomName.substr(0, roomName.find_last_of('.'));

					Room *room = new Room(path.string().substr(0, path.string().find_last_of('.')), roomName);
					rooms.push_back(room);
				}
			);

			addButton("Import",
				[window](Button *button) {
					std::string pathString = OpenFileDialog("RW World File (world_xx.txt)\0*.txt\0");

					if (pathString == "") return;

					std::filesystem::path path = pathString;

					exportDirectory = path.parent_path();
					worldAcronym = toLower(path.filename().string());
					worldAcronym = worldAcronym.substr(worldAcronym.find_last_of('_') + 1, worldAcronym.find_last_of('.') - worldAcronym.find_last_of('_') - 1);

					std::cout << "Opening world " << worldAcronym << std::endl;

					std::filesystem::path mapFilePath = findFileCaseInsensitive(exportDirectory.string(), "map_" + worldAcronym + ".txt");

					std::string propertiesFilePath = findFileCaseInsensitive(exportDirectory.string(), "properties.txt");

					for (Room *room : rooms) delete room;
					rooms.clear();
					for (Connection *connection : connections) delete connection;
					connections.clear();
					subregions.clear();
					extraProperties = "";
					extraWorld = "";

					if (std::filesystem::exists(propertiesFilePath)) {
						std::cout << "Found properties file, loading subregions" << std::endl;

						parseProperties(propertiesFilePath);
					}

					if (std::filesystem::exists(mapFilePath)) {
						parseMap(mapFilePath, exportDirectory);
					} else {
						std::cout << "Map file not found, loading world file" << std::endl;
					}

					parseWorld(path, exportDirectory);
				}
			);

			addButton("Export",
				[window](Button *button) {
					if (exportDirectory.string().length() == 0) {
						if (worldAcronym == "") {
							addPopup(new WarningPopup(window, "You must create or import a region\nbefore exporting."));
							return;
						}

						std::filesystem::path fileDirectory = OpenNewFileDialog("RW World File (world_xx.txt)\0*.txt");

						if (fileDirectory.empty()) return;

						exportDirectory = fileDirectory.parent_path();
					}

					exportMapFile();
					exportWorldFile();
					exportImageFile(exportDirectory / ("map_" + worldAcronym + ".png"), exportDirectory / ("map_" + worldAcronym + "_2.png"));
				}
			);

			addButton("No Colours",
				[window](Button *button) {
					::roomColours = (::roomColours + 1) % 3;

					if (::roomColours == 0) {
						button->Text("No Colours");
					} else if (::roomColours == 1) {
						button->Text("Layer Colours");
					} else {
						button->Text("Subregion Colours");
					}
				}
			);
		}

		static void cleanup() {
			for (Button *button : buttons) {
				delete button;
			}

			buttons.clear();
		}

		static void draw(Mouse *mouse) {
			setThemeColour(THEME_HEADER_COLOUR);
			fillRect(-1.0f, 1.0f, 1.0f, 1.0f - 0.06f);
			glLineWidth(1);

			for (Button *button : buttons) {
				button->update(mouse);
				button->draw(mouse);
			}
		}

		static void WorldAcronym(std::string worldAcronym) {
			MenuItems::worldAcronym = worldAcronym;
		}
		
		static std::string extraProperties;
		static std::string extraWorld;

	private:
		static std::vector<Button*> buttons;

		static Window *window;

		static double currentButtonX;

		static std::filesystem::path exportDirectory;
		static std::string worldAcronym;
};

#endif