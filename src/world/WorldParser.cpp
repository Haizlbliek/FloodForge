#include "WorldParser.hpp"

#include "RecentFiles.hpp"
#include "../popup/InfoPopup.hpp"
#include "CreatureTextures.hpp"

void WorldParser::importWorldFile(std::filesystem::path path) {
	RecentFiles::addPath(path);
	
	EditorState::region.reset();

	EditorState::region.exportDirectory = path.parent_path();
	EditorState::region.acronym = path.stem().generic_u8string();
	EditorState::region.acronym = EditorState::region.acronym.substr(EditorState::region.acronym.find_last_of('_') + 1);
	
	Logger::info("Opening world ", EditorState::region.acronym);
	
	EditorState::region.roomsDirectory = findDirectoryCaseInsensitive(EditorState::region.exportDirectory.parent_path(), EditorState::region.acronym + "-rooms");
	if (EditorState::region.roomsDirectory.empty()) {
		EditorState::fails.push_back("Cannot find rooms directory!");
	}
	Logger::info("Rooms directory: ", EditorState::region.roomsDirectory.generic_u8string());
	
	std::filesystem::path mapFilePath = findFileCaseInsensitive(EditorState::region.exportDirectory, "map_" + EditorState::region.acronym + ".txt");
	
	std::filesystem::path propertiesFilePath = findFileCaseInsensitive(EditorState::region.exportDirectory, "properties.txt");
	
	if (std::filesystem::exists(propertiesFilePath)) {
		Logger::info("Found properties file, loading subregions");
	
		parseProperties(propertiesFilePath);
	}
	
	if (std::filesystem::exists(mapFilePath)) {
		Logger::info("Loading map");
	
		parseMap(mapFilePath, EditorState::region.exportDirectory);
	} else {
		Logger::info("Map file not found, loading world file");
	}
	
	Logger::info("Loading world");
	parseWorld(path, EditorState::region.exportDirectory);
	
	Logger::info("Loading extra room data");
	
	for (Room *room : EditorState::rooms) {
		if (room->isOffscreen()) continue;
	
		for (auto x : EditorState::region.roomAttractiveness) {
			if (x.first != room->roomName) continue;
			
			room->data.attractiveness = x.second;
			break;
		}
		loadExtraRoomData(findFileCaseInsensitive(EditorState::region.roomsDirectory, room->roomName + "_settings.txt"), room->data);
	}
	
	Logger::info("Extra room data - loaded");
	
	if (EditorState::fails.size() > 0) {
		std::string fails = "";
		for (std::string fail : EditorState::fails) {
			fails += fail + "\n";
		}
		Popups::addPopup(new InfoPopup(fails));
		EditorState::fails.clear();
	}
}

void WorldParser::parseMap(std::filesystem::path mapFilePath, std::filesystem::path directory) {
	std::fstream mapFile(mapFilePath);
	
	std::map<std::string, ExtraRoomData> extraRoomData;

	std::string line;
	while (std::getline(mapFile, line)) {
		if (!line.empty() && line.back() == '\r') line.pop_back();

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
		} else if (startsWith(line, "//")) {
			EditorState::region.extraMap += line + "\n";
			continue;
		} else if (startsWith(line, "Connection: ")) {
			// Skip
		} else {
			std::string roomName = line.substr(0, line.find(':'));

			std::filesystem::path roomPath = EditorState::region.roomsDirectory;

			if (startsWith(toLower(roomName), "gate")) {
				roomPath = findDirectoryCaseInsensitive(roomPath.parent_path(), "gates");
				Logger::info("Found gate ", roomName);
			}
			
			std::filesystem::path filePath = findFileCaseInsensitive(roomPath, roomName + ".txt");
			if (filePath.empty()) {
				Logger::error("File '", (roomPath / roomName).generic_u8string(), ".txt' could not be found.");
			}

			Room *room = nullptr;

			if (startsWith(toLower(roomName), "offscreenden")) {
				if (EditorState::offscreenDen == nullptr) {
					size_t colonPos = line.find(':');
					std::string offscreenName = (colonPos != std::string::npos) ? line.substr(0, colonPos) : line;
					EditorState::offscreenDen = new OffscreenRoom(roomName, offscreenName);
					EditorState::rooms.push_back(EditorState::offscreenDen);
					room = EditorState::offscreenDen;
				} else {
					room = EditorState::offscreenDen;
				}
			} else {
				room = new Room(filePath, filePath.stem().string());
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
			} catch (...) {
				Logger::error("Failed to load map line '", line, "' due to stoi on '", temp, "' (int layer)");
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
	
	for (const auto &[roomName, extraRoomData] : extraRoomData) {
		for (Room *room : EditorState::rooms) {
			if (compareInsensitive(room->roomName, roomName)) {
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

	std::string roomName = std::get<0>(parts);

	Room *room = nullptr;
	for (Room *otherRoom : EditorState::rooms) {
		if (compareInsensitive(otherRoom->roomName, roomName)) {
			room = otherRoom;
			break;
		}
	}

	if (room == nullptr) {
		if (startsWith(roomName, "offscreenden")) {
			room = new OffscreenRoom(roomName, roomName);
		} else {
			std::filesystem::path roomPath = EditorState::region.roomsDirectory;

			if (startsWith(toLower(roomName), "gate")) {
				roomPath = findDirectoryCaseInsensitive(roomPath.parent_path(), "gates");
			}

			std::filesystem::path filePath = findFileCaseInsensitive(roomPath, roomName + ".txt");
			if (filePath.empty()) {
				Logger::error("File '", (roomPath / roomName).generic_u8string(), ".txt' could not be found.");
			}

			room = new Room(filePath, roomName);
		}

		EditorState::rooms.push_back(room);
	}

	int connectionId = 0;
	for (std::string connection : std::get<1>(parts)) {
		if (toLower(connection) == "disconnected") {
			connectionId++;
			continue;
		}

		bool alreadyExists = false;
		for (Quadruple<Room*, int, std::string, int> &connectionData : connectionsToAdd) {
			if (compareInsensitive(connectionData.first->roomName,  connection) && compareInsensitive(connectionData.third, roomName)) {
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
	std::vector<std::string> splits = split(line, " : ");
	TimelineType timelineType = TimelineType::ALL;
	std::set<std::string> timelines;

	if (splits[0][0] == '(') {
		std::string v = splits[0].substr(1, splits[0].find(')') - 1);
		splits[0] = splits[0].substr(splits[0].find(')') + 1);
		if (startsWith(v, "X-") || startsWith(v, "x-")) {
			timelineType = TimelineType::EXCEPT;
			v = v.substr(2);
		} else {
			timelineType = TimelineType::ONLY;
		}

		for (std::string timeline : split(v, ',')) {
			timelines.insert(timeline);
		}
	}

	std::string roomName = splits[0];
	if (splits[0] == "LINEAGE") {
		roomName = splits[1];
	}
	Room *room = nullptr;

	for (Room *otherRoom : EditorState::rooms) {
		if (compareInsensitive(otherRoom->roomName, roomName)) {
			room = otherRoom;
			break;
		}
	}

	if (toLower(roomName) == "offscreen") {
		room = EditorState::offscreenDen;
	}

	if (room == nullptr) {
		Logger::info("Skipped line due to missing room:");
		Logger::info("> ", line);
		EditorState::region.complicatedCreatures += line + "\n";
		return;
	}

	if (splits[0] == "LINEAGE") {
		int denId = -1;
		try {
			denId = std::stoi(splits[2]);
		} catch (...) {
			Logger::error("Failed to load creature line '", line, "' due to stoi for '", splits[2], "' (int denId LINEAGE)");
			return;
		}

		if (room == EditorState::offscreenDen) {
			denId = 0;
			EditorState::offscreenDen->getDen();
		}

		if (!room->CreatureDenExists(denId)) {
			Logger::info(roomName, " failed den ", denId);
			EditorState::fails.push_back(roomName + " failed den " + std::to_string(denId));
			return;
		}

		Den &den = room->CreatureDen(denId);
		den.creatures.push_back(DenLineage("", 0, "", 0.0));
		DenLineage &lineage = den.creatures[den.creatures.size() - 1];
		lineage.timelineType = timelineType;
		for (std::string timeline : timelines) {
			lineage.timelines.insert(timeline);
		}
		DenCreature *creature = &lineage;
		bool first = true;
		for (std::string creatureInDen : split(splits[3], ',')) {
			if (!first) {
				creature->lineageTo = new DenCreature("", 0, "", 0.0);
				creature = creature->lineageTo;
			}
			first = false;

			std::vector<std::string> sections = split(creatureInDen, '-');
			creature->type = CreatureTextures::parse(sections[0]);
			creature->count = 1;
			try {
				creature->lineageChance = std::stod(sections[sections.size() - 1]);
			} catch (...) {
				Logger::error("Failed to load creature line '", line, "' due to stod for '", sections[sections.size() - 1], "' (creature->lineageChance)");
			}

			if (sections.size() == 3) {
				if (sections[1][0] == '{') {
					std::string tag = sections[1].substr(1, sections[1].size() - 2);
					if (startsWith(tag, "Mean")) {
						creature->tag = "MEAN";
						creature->data = std::stod(tag.substr(tag.find_first_of(':') + 1));
					} else if (startsWith(tag, "Seed")) {
						creature->tag = "SEED";
						creature->data = std::stod(tag.substr(tag.find_first_of(':') + 1));
					} else if (startsWith(tag, "RotType")) {
						creature->tag = "RotType";
						creature->data = std::stod(tag.substr(tag.find_first_of(':') + 1));
					} else if (tag.find(':') != -1) {
						creature->tag = "LENGTH";
						creature->data = std::stod(tag.substr(tag.find_first_of(':') + 1));
					} else {
						creature->tag = tag;
					}
				}
			}
		}
	} else {
		for (std::string creatureInDen : split(splits[1], ',')) {
			std::vector<std::string> sections = split(creatureInDen, '-');
			int denId;
			try {
				denId = std::stoi(sections[0]);
			} catch (...) {
				Logger::error("Failed to load creature line '", line, "' due to stoi for '", sections[0], "' (int denId)");
				return;
			}
			std::string creature = sections[1];
	
			if (room == EditorState::offscreenDen) {
				denId = 0;
				EditorState::offscreenDen->getDen();
			}
	
			if (!room->CreatureDenExists(denId)) {
				Logger::info(roomName, " failed den ", denId);
				EditorState::fails.push_back(roomName + " failed den " + std::to_string(denId));
				continue;
			}
	
			Den &den = room->CreatureDen(denId);
			DenLineage denCreature = DenLineage("", 0, "", 0.0);
			denCreature.timelineType = timelineType;
			for (std::string timeline : timelines) {
				denCreature.timelines.insert(timeline);
			}
			denCreature.type = CreatureTextures::parse(creature);
	
			if (sections.size() == 3) {
				if (sections[2][0] == '{') {
					std::string tag = sections[2].substr(1, sections[2].size() - 2);
					if (startsWith(tag, "Mean")) {
						denCreature.tag = "MEAN";
						denCreature.data = std::stod(tag.substr(tag.find_first_of(':') + 1));
					} else if (startsWith(tag, "Seed")) {
						denCreature.tag = "SEED";
						denCreature.data = std::stod(tag.substr(tag.find_first_of(':') + 1));
					} else if (startsWith(tag, "RotType")) {
						denCreature.tag = "RotType";
						denCreature.data = std::stod(tag.substr(tag.find_first_of(':') + 1));
					} else if (tag.find(':') != -1) {
						denCreature.tag = "LENGTH";
						denCreature.data = std::stod(tag.substr(tag.find_first_of(':') + 1));
					} else {
						denCreature.tag = tag;
					}
					denCreature.count = 1;
				} else {
					try {
						denCreature.count = std::stoi(sections[2]);
					} catch (...) {
						Logger::error("Failed to load creature line '", line, "' due to stoi for '", sections[2], "' (den.count)");
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
					denCreature.tag = "MEAN";
					denCreature.data = std::stod(tag.substr(tag.find_first_of(':') + 1));
				} else if (startsWith(tag, "Seed")) {
					denCreature.tag = "SEED";
					denCreature.data = std::stod(tag.substr(tag.find_first_of(':') + 1));
				} else if (tag.find(':') != -1) {
					denCreature.tag = "LENGTH";
					denCreature.data = std::stod(tag.substr(tag.find_first_of(':') + 1));
				} else {
					denCreature.tag = tag;
				}
				try {
					denCreature.count = std::stoi(countString);
				} catch (...) {
					Logger::error("Failed to load creature line '", line, "' due to stoi on '", countString, "' (den.count)");
				}
			} else {
				denCreature.count = 1;
			}

			den.creatures.push_back(denCreature);
		}
	}
}

void WorldParser::parseWorldConditionalLink(std::string link, std::vector<ConditionalConnection> &connectionsToAdd) {
	std::vector<std::string> parts = split(link, ':');
	if (parts.size() != 3 && parts.size() != 4) {
		Logger::warn("Skipping line due to improper length");
		Logger::warn("> ", link);
		return;
	}

	std::vector<std::string> timelines = split(parts[0], ',');

	if (parts.size() == 3) {
		std::string roomName = parts[2];
		Room *room = nullptr;
		for (Room *otherRoom : EditorState::rooms) {
			if (compareInsensitive(otherRoom->roomName, roomName)) {
				room = otherRoom;
				break;
			}
		}

		if (room == nullptr) {
			Logger::warn("Skipping line due to missing room: ", roomName);
			Logger::warn("> ", link);
			return;
		}

		std::string mod = toLower(parts[1]);

		if (mod == "exclusiveroom") {
			if (room->timelineType == TimelineType::EXCEPT) {
				Logger::warn("Skipping line due to invalid EXCLUSIVEROOM: ", roomName);
				Logger::warn("> ", link);
				return;
			}

			room->timelineType = TimelineType::ONLY;
			for (std::string timeline : timelines) {
				room->timelines.insert(timeline);
			}
		} else if (mod == "hideroom") {
			if (room->timelineType == TimelineType::ONLY) {
				Logger::warn("Skipping line due to invalid HIDEROOM: ", roomName);
				Logger::warn("> ", link);
				return;
			}

			room->timelineType = TimelineType::EXCEPT;
			for (std::string timeline : timelines) {
				room->timelines.insert(timeline);
			}
		}

		return;
	}

	std::string roomName = parts[1];
	Room *room = nullptr;
	for (Room *otherRoom : EditorState::rooms) {
		if (compareInsensitive(otherRoom->roomName, roomName)) {
			room = otherRoom;
			break;
		}
	}

	if (room == nullptr) {
		Logger::warn("Skipping line due to missing room: ", roomName);
		Logger::warn("> ", link);
		return;
	}

	std::string currentConnection = parts[2];

	bool isCurrentDisconnected = false;
	int disconnectedId = -1;
	try {
		disconnectedId = std::stoi(currentConnection);
		isCurrentDisconnected = true;
	} catch (...) {
		isCurrentDisconnected = false;
	}

	std::string toConnection = parts[3];

	if (compareInsensitive(currentConnection, toConnection)) {
		Logger::warn("Skipping line due to no change");
		Logger::warn("> ", link);
		return;
	}

	if (toConnection == "disconnected") {
		Connection *connection = nullptr;
		for (Connection *otherConnection : room->connections) {
			Room *otherRoom = (otherConnection->roomA == room) ? otherConnection->roomB : otherConnection->roomA;

			if (compareInsensitive(otherRoom->roomName, currentConnection)) {
				connection = otherConnection;
				break;
			}
		}

		if (connection == nullptr) {
			Logger::warn("Skipping line due to missing connection");
			Logger::warn("> ", link);
			return;
		}

		if (connection->timelineType == TimelineType::ONLY) {
			for (std::string timeline : timelines) {
				connection->timelines.erase(timeline);
			}
		} else {
			connection->timelineType = TimelineType::EXCEPT;
			for (std::string timeline : timelines) {
				connection->timelines.insert(timeline);
			}
		}
	} else {
		int connectionId = -1;

		if (isCurrentDisconnected) {
			std::string timeline = timelines[0];
			std::vector<bool> connected(room->RoomEntranceCount(), false);
			for (Connection *connection : room->connections) {
				if (!connection->allowsTimeline(timeline)) continue;

				connected[connection->roomA == room ? connection->connectionA : connection->connectionB] = true;
			}

			for (int i = 0; i < connected.size(); i++) {
				if (!connected[i]) {
					disconnectedId--;
					if (disconnectedId == 0) {
						connectionId = i;
						break;
					}
				}
			}
		} else {
			Connection *connection = nullptr;
			for (Connection *otherConnection : room->connections) {
				Room *otherRoom = (otherConnection->roomA == room) ? otherConnection->roomB : otherConnection->roomA;
	
				if (compareInsensitive(otherRoom->roomName,  currentConnection)) {
					connection = otherConnection;
					break;
				}
			}
	
			if (connection == nullptr) {
				Logger::warn("Line missing connection, adding new connection anyways");
				Logger::warn("> ", link);
			} else {
				connectionId = (connection->roomA == room) ? connection->connectionA : connection->connectionB;

				if (connection->timelineType == TimelineType::ONLY) {
					for (std::string timeline : timelines) {
						connection->timelines.erase(timeline);
					}
				} else {
					connection->timelineType = TimelineType::EXCEPT;
	
					for (std::string timeline : timelines) {
						connection->timelines.insert(timeline);
					}
				}
			}
		}

		Connection *connection = nullptr;
		for (Connection *otherConnection : room->connections) {
			Room *otherRoom = (otherConnection->roomA == room) ? otherConnection->roomB : otherConnection->roomA;

			if (compareInsensitive(otherRoom->roomName, toConnection)) {
				connection = otherConnection;
				break;
			}
		}

		if (connection != nullptr) {
			if (connection->timelineType == TimelineType::EXCEPT) {
				for (std::string timeline : timelines) {
					connection->timelines.erase(timeline);
				}
			} else {
				connection->timelineType = TimelineType::ONLY;

				for (std::string timeline : timelines) {
					connection->timelines.insert(timeline);
				}
			}
		} else {
			if (connectionId == -1) {
				Logger::warn("Connection id cannot be inferred");
				Logger::warn("> ", link);
				return;
			}

			for (ConditionalConnection &connectionData : connectionsToAdd) {
				if (connectionData.roomB == nullptr && compareInsensitive(connectionData.roomA->roomName, toConnection) && compareInsensitive(connectionData.roomBName, room->roomName)) {
					connectionData.roomB = room;
					connectionData.connectionB = connectionId;
					return;
				}
			}

			std::set<std::string> connectionTimelines;
			for (std::string timeline : timelines) {
				connectionTimelines.insert(timeline);
			}
			connectionsToAdd.push_back({
				room,
				connectionId,
				toConnection,
				nullptr,
				-1,
				connectionTimelines,
				TimelineType::ONLY
			});
		}
	}
}

void WorldParser::parseWorld(std::filesystem::path worldFilePath, std::filesystem::path directory) {
	std::fstream worldFile(worldFilePath);

	std::vector<Quadruple<Room*, int, std::string, int>> connectionsToAdd;
	std::vector<std::string> conditionalLinks;

	enum class ParseState {
		None,
		DuringConditionalLinks,
		DuringRooms,
		DuringCreatures
	};

	ParseState parseState = ParseState::None;
	std::string line;
	while (std::getline(worldFile, line)) {
		if (!line.empty() && line.back() == '\r') line.pop_back();

		if (line == "ROOMS") {
			parseState = ParseState::DuringRooms;
			Logger::info("World - Rooms");
			continue;
		}

		if (line == "END ROOMS") {
			parseState = ParseState::None;
	
			if (EditorState::offscreenDen == nullptr) {
				EditorState::offscreenDen = new OffscreenRoom("offscreenden" + EditorState::region.acronym, "OffscreenDen" + EditorState::region.acronym);
				EditorState::rooms.push_back(EditorState::offscreenDen);
			}

			continue;
		}

		if (line == "CREATURES") {
			parseState = ParseState::DuringCreatures;
			Logger::info("World - Creatures");
			continue;
		}

		if (line == "END CREATURES") {
			parseState = ParseState::None;
			continue;
		}

		if (line == "CONDITIONAL LINKS") {
			parseState = ParseState::DuringConditionalLinks;
			Logger::info("World - Conditional Links");
			continue;
		}

		if (line == "END CONDITIONAL LINKS") {
			parseState = ParseState::None;
			continue;
		}

		if (parseState == ParseState::None) {
			EditorState::region.extraWorld += line + "\n";
			continue;
		}

		if (line == "") continue;
		if (startsWith(line, "//")) continue;

		if (parseState == ParseState::DuringRooms) {
			parseWorldRoom(line, directory, connectionsToAdd);
		}

		if (parseState == ParseState::DuringCreatures) {
			parseWorldCreature(line);
		}

		if (parseState == ParseState::DuringConditionalLinks) {
			conditionalLinks.push_back(line);
		}
	}
	worldFile.close();

	Logger::info("Loading connections");

	for (Quadruple<Room*, int, std::string, int> &connectionData : connectionsToAdd) {
		if (connectionData.second == -1 || connectionData.fourth == -1) {
			Logger::info("Failed to load connection from ", connectionData.first->roomName, " to ", connectionData.third);
			continue;
		}

		Room *roomA = connectionData.first;
		Room *roomB = nullptr;

		for (Room *room : EditorState::rooms) {
			if (compareInsensitive(room->roomName, connectionData.third)) {
				roomB = room;
				break;
			}
		}

		if (roomB == nullptr) {
			Logger::info("Failed to load connection from ", roomA->roomName, " to ", connectionData.third);
			EditorState::fails.push_back("Failed to load connection from " + roomA->roomName + " to " + connectionData.third);
			continue;
		}

		int connectionA = connectionData.second;
		int connectionB = connectionData.fourth;
		
		if (!roomA->canConnect(connectionA) || !roomB->canConnect(connectionB)) {
			Logger::info("Failed to load connection from ", roomA->roomName, " to ", connectionData.third, " - invalid room");
			EditorState::fails.push_back("Failed to load connection from " + roomA->roomName + " to " + connectionData.third + " - invalid room");
			continue;
		}

		Connection *connection = new Connection(roomA, connectionA, roomB, connectionB);
		EditorState::connections.push_back(connection);

		roomA->connect(connection);
		roomB->connect(connection);
	}
	
	Logger::info("Connections loaded");

	Logger::info("Loading Conditional Links");

	std::vector<ConditionalConnection> conditionalConnectionsToAdd;
	for (std::string link : conditionalLinks) {
		parseWorldConditionalLink(link, conditionalConnectionsToAdd);
	}
	for (ConditionalConnection &connectionData : conditionalConnectionsToAdd) {
		if (connectionData.roomB == nullptr) {
			Logger::info("Connection missing roomB");
			continue;
		}

		if (connectionData.connectionA == -1 || connectionData.connectionB == -1) {
			Logger::info("Failed to load connection from ", connectionData.roomA->roomName, " to ", connectionData.roomB->roomName);
			continue;
		}
		
		if (!connectionData.roomA->canConnect(connectionData.connectionA) || !connectionData.roomB->canConnect(connectionData.connectionB)) {
			Logger::info("Failed to load connection from ", connectionData.roomA->roomName, " to ", connectionData.roomB->roomName, " - invalid room");
			EditorState::fails.push_back("Failed to load connection from " + connectionData.roomA->roomName + " to " + connectionData.roomB->roomName + " - invalid room");
			continue;
		}

		Connection *connection = new Connection(connectionData.roomA, connectionData.connectionA, connectionData.roomB, connectionData.connectionB);
		for (std::string timeline : connectionData.timelines) {
			connection->timelines.insert(timeline);
		}
		connection->timelineType= connectionData.timelineType;
		EditorState::connections.push_back(connection);

		connectionData.roomA->connect(connection);
		connectionData.roomB->connect(connection);
	}

	Logger::info("Conditional Links loaded");
}

RoomAttractiveness WorldParser::parseRoomAttractiveness(std::string value) {
	if (value == "neutral") return RoomAttractiveness::NEUTRAL;
	if (value == "forbidden") return RoomAttractiveness::FORBIDDEN;
	if (value == "avoid") return RoomAttractiveness::AVOID;
	if (value == "like") return RoomAttractiveness::LIKE;
	if (value == "stay") return RoomAttractiveness::STAY;
	
	return RoomAttractiveness::DEFAULT;
}

void WorldParser::parseProperties(std::filesystem::path propertiesFilePath) {
	std::fstream propertiesFile(propertiesFilePath);
	
	std::string line;
	while (std::getline(propertiesFile, line)) {
		if (!line.empty() && line.back() == '\r') line.pop_back();

		if (startsWith(line, "Subregion: ")) {
			std::string subregionName = line.substr(line.find(':') + 2);
			Logger::info("Subregion: ", subregionName);
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
			EditorState::region.roomAttractiveness.push_back({ room, attractiveness });
		} else if (startsWith(line, "//FloodForge|")) {
			std::vector<std::string> splits = split(line, '|');
			try {
				if (splits[1] == "SubregionColorOverride") {
					EditorState::region.overrideSubregionColors[std::stoi(splits[2])] = stringToColour(splits[3]);
				}
			} catch (std::exception err) {
				Logger::info("Error while loading property comment: ", line);
			}
		} else {
			EditorState::region.extraProperties += line + "\n";
		}
	}

	propertiesFile.close();
}

void WorldParser::loadExtraRoomData(std::filesystem::path roomPath, ExtraRoomData &data) {
	if (roomPath == "") return;

	std::fstream file(roomPath, std::ios::in | std::ios::binary);

	if (!file.is_open()) return;

	std::string line;
	while (std::getline(file, line)) {
		if (!line.empty() && line.back() == '\r') line.pop_back();
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