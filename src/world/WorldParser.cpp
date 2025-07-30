#include "WorldParser.hpp"

#include "RecentFiles.hpp"
#include "../popup/InfoPopup.hpp"
#include "CreatureTextures.hpp"

void WorldParser::importWorldFile(std::filesystem::path path) {
	RecentFiles::addPath(path);
	
	EditorState::region.reset();

	EditorState::region.exportDirectory = path.parent_path();
	EditorState::region.acronym = toLower(path.filename().string());
	EditorState::region.acronym = EditorState::region.acronym.substr(EditorState::region.acronym.find_last_of('_') + 1, EditorState::region.acronym.find_last_of('.') - EditorState::region.acronym.find_last_of('_') - 1);
	
	Logger::log("Opening world ", EditorState::region.acronym);
	
	std::filesystem::path roomsPath = findDirectoryCaseInsensitive(EditorState::region.exportDirectory.parent_path().string(), EditorState::region.acronym + "-rooms");
	if (roomsPath.empty()) {
		EditorState::region.roomsDirectory = "";
		EditorState::fails.push_back("Cannot find rooms directory!");
	} else {
		EditorState::region.roomsDirectory = roomsPath.filename().string();
	}
	Logger::log("Rooms directory: ", EditorState::region.roomsDirectory);
	
	std::filesystem::path mapFilePath = findFileCaseInsensitive(EditorState::region.exportDirectory.string(), "map_" + EditorState::region.acronym + ".txt");
	
	std::string propertiesFilePath = findFileCaseInsensitive(EditorState::region.exportDirectory.string(), "properties.txt");
	
	if (std::filesystem::exists(propertiesFilePath)) {
		Logger::log("Found properties file, loading subregions");
	
		parseProperties(propertiesFilePath);
	}
	
	if (std::filesystem::exists(mapFilePath)) {
		Logger::log("Loading map");
	
		parseMap(mapFilePath, EditorState::region.exportDirectory);
	} else {
		Logger::log("Map file not found, loading world file");
	}
	
	Logger::log("Loading world");
	parseWorld(path, EditorState::region.exportDirectory);
	
	Logger::log("Loading extra room data");
	
	for (Room *room : EditorState::rooms) {
		if (room->isOffscreen()) continue;
	
		for (auto x : EditorState::region.roomAttractiveness) {
			if (x.first != room->roomName) continue;
			
			room->data.attractiveness = x.second;
			break;
		}
		loadExtraRoomData(findFileCaseInsensitive((EditorState::region.exportDirectory.parent_path() / EditorState::region.roomsDirectory).string(), room->roomName + "_settings.txt"), room->data);
	}
	
	Logger::log("Extra room data - loaded");
	
	if (EditorState::fails.size() > 0) {
		std::string fails = "";
		for (std::string fail : EditorState::fails) {
			fails += fail + "\n";
		}
		Popups::addPopup(new InfoPopup(EditorState::window, fails));
		EditorState::fails.clear();
	}
}

void WorldParser::parseMap(std::filesystem::path mapFilePath, std::filesystem::path directory) {
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
					} else if (key == "nomerge") {
						extraRoomData[data[1]].merge = false;
					}
				}
			}
		} else if (startsWith(line, "Connection: ")) {
			// Skip
		} else {
			std::string roomName = toLower(line.substr(0, line.find(':')));

			std::filesystem::path roomPath = directory.parent_path() / EditorState::region.roomsDirectory;

			if (startsWith(roomName, "gate")) {
				roomPath = findDirectoryCaseInsensitive(roomPath.parent_path().string(), "gates");
				Logger::log("Found gate ", roomName);
			}
			
			roomPath = roomPath / roomName;

			Room *room = nullptr;

			if (startsWith(roomName, "offscreenden")) {
				if (EditorState::offscreenDen == nullptr) {
					EditorState::offscreenDen = new OffscreenRoom(roomName, roomName);
					EditorState::rooms.push_back(EditorState::offscreenDen);
					room = EditorState::offscreenDen;
				} else {
					room = EditorState::offscreenDen;
				}
			} else {
				room = new Room(roomPath.string() + ".txt", roomName);
				EditorState::rooms.push_back(room);
			}

			std::string temp;
			std::stringstream data(line.substr(line.find(':') + 1));

			double scale = 1.0 / 3.0;

			std::getline(data, temp, '>'); // Canon X
			double x = std::stod(temp) * scale;

			std::getline(data, temp, '<');
			std::getline(data, temp, '>'); // Canon Y
			double y = std::stod(temp) * scale;

			std::getline(data, temp, '<');
			std::getline(data, temp, '>'); // Dev X
			double devX = std::stod(temp) * scale;

			std::getline(data, temp, '<');
			std::getline(data, temp, '>'); // Dev Y
			double devY = std::stod(temp) * scale;

			std::getline(data, temp, '<');
			std::getline(data, temp, '>'); // Layer
			int layer = 0;
			try {
				layer = temp.empty() ? 0 : std::stoi(temp);
			} catch (const std::invalid_argument &e) {
				Logger::logError("Failed to load map line '", line, "' due to stoi on '", temp, "' (int layer)");
			}
			
			std::getline(data, temp, '<');
			std::getline(data, temp, '>'); // Subregion
			std::string subregion = temp;

			room->canonPosition.x = x - room->Width() * 0.5;
			room->canonPosition.y = y + room->Height() * 0.5;
			room->devPosition.x = devX - room->Width() * 0.5;
			room->devPosition.y = devY + room->Height() * 0.5;
			room->layer = layer;
			
			// Backwards-Compatibility
			if (layer >= LAYER_HIDDEN && layer <= LAYER_HIDDEN + 2) {
				room->data.hidden = true;
				room->layer = layer - LAYER_HIDDEN;
			}

			if (subregion.empty()) {
				room->subregion = -1;
			} else {
				auto it = std::find(EditorState::subregions.begin(), EditorState::subregions.end(), subregion);
				if (it == EditorState::subregions.end()) {
					EditorState::subregions.push_back(subregion);
					it = std::find(EditorState::subregions.begin(), EditorState::subregions.end(), subregion);
				}

				room->subregion = std::distance(EditorState::subregions.begin(), it);
			}
		}
	}
	mapFile.close();
	
	for (const auto &[oRoomName, extraRoomData] : extraRoomData) {
		std::string roomName = toLower(oRoomName);

		for (Room *room : EditorState::rooms) {
			if (room->roomName == roomName) {
				room->data = extraRoomData;
				break;
			}
		}
	}
}

std::tuple<std::string, std::vector<std::string>, std::vector<std::string>> WorldParser::parseRoomString(const std::string &input) {
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

void WorldParser::parseWorldRoom(std::string line, std::filesystem::path directory, std::vector<Quadruple<Room*, int, std::string, int>> &connectionsToAdd) {
	std::tuple<std::string, std::vector<std::string>, std::vector<std::string>> parts = parseRoomString(line);

	std::string roomName = toLower(std::get<0>(parts));

	Room *room = nullptr;
	for (Room *otherRoom : EditorState::rooms) {
		if (toLower(otherRoom->roomName) == roomName) {
			room = otherRoom;
			break;
		}
	}

	if (room == nullptr) {
		if (startsWith(roomName, "offscreenden")) {
			room = new OffscreenRoom(roomName, roomName);
		} else {
			std::filesystem::path roomPath = directory.parent_path() / EditorState::region.roomsDirectory;

			if (startsWith(roomName, "gate")) {
				roomPath = findDirectoryCaseInsensitive(roomPath.parent_path().string(), "gates");
			}

			std::string filePath = findFileCaseInsensitive(roomPath.string(), roomName + ".txt");
			if (filePath.empty()) {
				Logger::logError("File `", roomPath, "/", roomName, ".txt` could not be found.");
			}

			room = new Room(filePath, roomName);
		}

		EditorState::rooms.push_back(room);
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

	room->SetTag("");
	for (std::string tag : std::get<2>(parts)) {
		room->ToggleTag(tag);
	}
}

void WorldParser::parseWorldCreature(std::string line) {
	std::vector<std::string> splits = split(line, ':');

	if (splits[0] == "LINEAGE" || splits[0][0] == '(') {
		Logger::log("Skipped parsing complicated creature: '", line, "'");
		EditorState::region.complicatedCreatures += line + "\n";
		return;
	}

	std::string roomName = toLower(splits[0]);
	Room *room = nullptr;

	for (Room *otherRoom : EditorState::rooms) {
		if (toLower(otherRoom->roomName) == roomName) {
			room = otherRoom;
			break;
		}
	}

	if (roomName == "offscreen") {
		room = EditorState::offscreenDen;
	}

	if (room == nullptr) return;

	for (std::string creatureInDen : split(splits[1], ',')) {
		std::vector<std::string> sections = split(creatureInDen, '-');
		int denId;
		try {
			denId = std::stoi(sections[0]);
		} catch (const std::invalid_argument &e) {
			Logger::logError("Failed to load creature line '", line, "' due to stoi for '", sections[0], "' (int denId)");
		}
		std::string creature = sections[1];

		if (room == EditorState::offscreenDen) {
			denId = EditorState::offscreenDen->DenCount();
			EditorState::offscreenDen->AddDen();
		}

		if (!room->CreatureDenExists(denId)) {
			Logger::log(roomName, " failed den ", denId);
			EditorState::fails.push_back(roomName + " failed den " + std::to_string(denId));
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
				try {
					den.count = std::stoi(sections[2]);
				} catch (const std::invalid_argument &e) {
					Logger::logError("Failed to load creature line '", line, "' due to stoi for '", sections[2], "' (den.count)");
				}
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
			try {
				den.count = std::stoi(countString);
			} catch (const std::invalid_argument &e) {
				Logger::logError("Failed to load creature line '", line, "' due to stoi on '", countString, "' (den.count)");
			}
		} else {
			den.count = 1;
		}
	}
}

void WorldParser::parseWorld(std::filesystem::path worldFilePath, std::filesystem::path directory) {
	std::fstream worldFile(worldFilePath);

	std::vector<Quadruple<Room*, int, std::string, int>> connectionsToAdd;

	int parseState = 0;
	std::string line;
	while (std::getline(worldFile, line)) {
		if (line == "ROOMS") {
			parseState = 1;
			Logger::log("World - Rooms");
			continue;
		}

		if (line == "END ROOMS") {
			parseState = 2;
	
			if (EditorState::offscreenDen == nullptr) {
				EditorState::offscreenDen = new OffscreenRoom("offscreenden" + EditorState::region.acronym, "OffscreenDen" + toUpper(EditorState::region.acronym));
				EditorState::rooms.push_back(EditorState::offscreenDen);
			}

			continue;
		}

		if (line == "CREATURES") {
			parseState = 3;
			Logger::log("World - Creatures");
			continue;
		}

		if (line == "END CREATURES") {
			parseState = 4;
			continue;
		}

		if (parseState == 4) {
			EditorState::region.extraWorld += line + "\n";
			continue;
		}

		if (line == "") continue;
		if (startsWith(line, "//")) continue;

		if (parseState == 1)
			parseWorldRoom(line, directory, connectionsToAdd);
		
		if (parseState == 3)
			parseWorldCreature(line);
	}
	worldFile.close();
	
	Logger::log("Loading connections");

	for (Quadruple<Room*, int, std::string, int> connectionData : connectionsToAdd) {
		if (connectionData.second == -1 || connectionData.fourth == -1) {
			Logger::log("Failed to load connection from ", connectionData.first->roomName, " to ", connectionData.third);
			continue;
		}

		Room *roomA = connectionData.first;
		Room *roomB = nullptr;

		for (Room *room : EditorState::rooms) {
			if (toLower(room->roomName) == connectionData.third) {
				roomB = room;
				break;
			}
		}

		if (roomB == nullptr) {
			Logger::log("Failed to load connection from ", roomA->roomName, " to ", connectionData.third);
			EditorState::fails.push_back("Failed to load connection from " + roomA->roomName + " to " + connectionData.third);
			continue;
		}

		int connectionA = connectionData.second;
		int connectionB = connectionData.fourth;
		
		if (!roomA->canConnect(connectionA) || !roomB->canConnect(connectionB)) {
			Logger::log("Failed to load connection from ", roomA->roomName, " to ", connectionData.third, " - invalid room");
			EditorState::fails.push_back("Failed to load connection from " + roomA->roomName + " to " + connectionData.third + " - invalid room");
			continue;
		}

		roomA->connect(roomB, connectionA);
		roomB->connect(roomA, connectionB);

		Connection *connection = new Connection(roomA, connectionA, roomB, connectionB);
		EditorState::connections.push_back(connection);
	}
	
	Logger::log("Connections loaded");
}

RoomAttractiveness WorldParser::parseRoomAttractiveness(std::string value) {
	if (value == "neutral") return RoomAttractiveness::NEUTRAL;
	if (value == "forbidden") return RoomAttractiveness::FORBIDDEN;
	if (value == "avoid") return RoomAttractiveness::AVOID;
	if (value == "like") return RoomAttractiveness::LIKE;
	if (value == "stay") return RoomAttractiveness::STAY;
	
	return RoomAttractiveness::DEFAULT;
}

void WorldParser::parseProperties(std::string propertiesFilePath) {
	std::fstream propertiesFile(propertiesFilePath);
	
	std::string line;
	while (std::getline(propertiesFile, line)) {
		if (startsWith(line, "Subregion: ")) {
			std::string subregionName = line.substr(line.find(':') + 2);
			Logger::log("Subregion: ", subregionName);
			EditorState::subregions.push_back(subregionName);
		} else if (startsWith(line, "Room_Attr: ")) {
			std::string attractivenesses = line.substr(line.find(':') + 2);
			std::string room = attractivenesses.substr(0, attractivenesses.find(':'));
			std::vector<std::string> states = split(attractivenesses.substr(attractivenesses.find(':') + 2), ',');
			
			std::unordered_map<std::string, RoomAttractiveness> attractiveness;
			for (std::string state : states) {
				std::string creature = state.substr(0, state.find_first_of('-'));
				std::string value = state.substr(state.find_first_of('-') + 1);
				
				creature = CreatureTextures::parse(creature);
				attractiveness[creature] = parseRoomAttractiveness(toLower(value));
			}
			EditorState::region.roomAttractiveness.push_back({ toLower(room), attractiveness });
		} else {
			EditorState::region.extraProperties += line + "\n";
		}
	}

	propertiesFile.close();
}

void WorldParser::loadExtraRoomData(std::string roomPath, ExtraRoomData &data) {
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