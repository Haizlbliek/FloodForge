#include "WorldExporter.hpp"
#include "Backup.hpp"

#include "../math/Rect.hpp"

void WorldExporter::exportMapFile() {
	Logger::info("Exporting map file");

	std::filesystem::path path = EditorState::region.exportDirectory / ("map_" + EditorState::region.acronym + ".txt");
	Backup::backup(path);
	std::fstream file(path, std::ios::out | std::ios::trunc);

	if (!file.is_open()) {
		Logger::info("Error opening map_", EditorState::region.acronym, ".txt");
		return;
	}

	Logger::info("- Rooms");
	for (Room *room : EditorState::rooms) {
		Vector2 canonPosition = Vector2(
			(room->canonPosition.x + room->Width() * 0.5) * 3.0,
			(room->canonPosition.y - room->Height() * 0.5) * 3.0
		);
		Vector2 devPosition = Vector2(
			(room->devPosition.x + room->Width() * 0.5) * 3.0,
			(room->devPosition.y - room->Height() * 0.5) * 3.0
		);

		file << std::setprecision(12);
		file << toUpper(room->roomName) << ": ";
		file << canonPosition.x << "><" << canonPosition.y << "><";
		file << devPosition.x << "><" << devPosition.y << "><";
		file << room->layer << "><";
		if (room->subregion > -1) file << EditorState::subregions[room->subregion];
		file << "\n";
	}
	
	Logger::info("- FloodForge Data");
	for (Room *room : EditorState::rooms) {
		if (room->isOffscreen() || room->data.empty())
			continue;

		file << "//FloodForge;ROOM|" << room->roomName;
		if (room->data.hidden) file << "|hidden";
		if (!room->data.merge) file << "|nomerge";
		file << "\n";
	}

	Logger::info("- Connections");
	for (Connection *connection : EditorState::connections) {
		if (connection->roomA->data.hidden || connection->roomB->data.hidden) continue;

		Vector2i connectionA = connection->roomA->getRoomEntrancePosition(connection->connectionA);
		Vector2i connectionB = connection->roomB->getRoomEntrancePosition(connection->connectionB);

		connectionA = Vector2i(
			connectionA.x,
			connection->roomA->Height() - connectionA.y - 1
		);
		connectionB = Vector2i(
			connectionB.x,
			connection->roomB->Height() - connectionB.y - 1
		);

		file << "Connection: ";
		file << toUpper(connection->roomA->roomName) << ",";
		file << toUpper(connection->roomB->roomName) << ",";
		file << connectionA.x << "," << connectionA.y << ",";
		file << connectionB.x << "," << connectionB.y << ",";
		file << (int) connection->roomA->getRoomEntranceDirection(connection->connectionA) << ",";
		file << (int) connection->roomB->getRoomEntranceDirection(connection->connectionB);
		file << "\n";
	}

	file << EditorState::region.extraMap;

	file.close();
}

void parseConditionalLinkConnection(std::fstream &file, Room *room, Connection *connection, std::vector<std::string> &timelines, std::map<std::string, std::vector<std::pair<std::string, bool>>> &state, std::vector<std::pair<std::string, bool>> &defaultState) {
	Room *otherRoom = nullptr;
	int connectionId = -1;
	if (connection->roomA == room) {
		otherRoom = connection->roomB;
		connectionId = connection->connectionA;
	} else {
		otherRoom = connection->roomA;
		connectionId = connection->connectionB;
	}
	if (otherRoom == nullptr || connectionId == -1) return;

	for (std::string timeline : connection->timelines) {
		if (state.count(timeline) == 0) {
			state[timeline] = defaultState;
			timelines.push_back(timeline);
		}

		if (connection->timelineType == TimelineType::ONLY) {
			file << timeline << " : " << toUpper(room->roomName) << " : ";

			if (state[timeline][connectionId].first == "DISCONNECTED") {
				int disconnectedBefore = 0;
				for (int i = 0; i < connectionId; i++) {
					if (state[timeline][i].first == "DISCONNECTED") {
						disconnectedBefore++;
					}
				}
				file << (disconnectedBefore + 1);
			} else {
				file << state[timeline][connectionId].first;
			}
			file << " : " << toUpper(otherRoom->roomName) << "\n";

			if (toUpper(otherRoom->roomName) != state[timeline][connectionId].first) {
				state[timeline][connectionId] = { toUpper(otherRoom->roomName), true };
			}
		}
		else if (connection->timelineType == TimelineType::EXCEPT) {
			for (std::string otherTimeline : timelines) {
				if (otherTimeline == timeline) continue;
				if (!state[otherTimeline][connectionId].second) continue;

				file << otherTimeline << " : " << toUpper(room->roomName) << " : ";
				if (state[otherTimeline][connectionId].first == "DISCONNECTED") {
					int disconnectedBefore = 0;
					for (int i = 0; i < connectionId; i++) {
						if (state[otherTimeline][i].first == "DISCONNECTED") {
							disconnectedBefore++;
						}
					}
					file << (disconnectedBefore + 1);
				} else {
					file << state[otherTimeline][connectionId].first;
				}
				file << " : " << toUpper(otherRoom->roomName) << "\n";
			}

			file << timeline << " : " << toUpper(room->roomName) << " : ";
			if (state[timeline][connectionId].second) {
				if (state[timeline][connectionId].first == "DISCONNECTED") {
					int disconnectedBefore = 0;
					for (int i = 0; i < connectionId; i++) {
						if (state[timeline][i].first == "DISCONNECTED") {
							disconnectedBefore++;
						}
					}
					file << (disconnectedBefore + 1);
				} else {
					file << state[timeline][connectionId].first;
				}
			} else {
				file << toUpper(otherRoom->roomName);
			}
			file << " : " << defaultState[connectionId].first << "\n";

			if (toUpper(otherRoom->roomName) != defaultState[connectionId].first) {
				defaultState[connectionId] = { toUpper(otherRoom->roomName), false };
			}
		}
	}
}

void WorldExporter::exportWorldFile() {
	Logger::info("Exporting world file");

	std::filesystem::path path = EditorState::region.exportDirectory / ("world_" + EditorState::region.acronym + ".txt");
	Backup::backup(path);
	std::fstream file(path, std::ios::out | std::ios::trunc);

	if (!file.is_open()) {
		Logger::info("Error opening world_", EditorState::region.acronym, ".txt");
		return;
	}
	
	std::map<std::string, std::vector<std::pair<std::string, bool>>> roomDefaultStates;

	Logger::info("- Conditional Links");
	file << "CONDITIONAL LINKS\n";
	for (Room *room : EditorState::rooms) {
		if (room->isOffscreen()) continue;

		std::vector<std::string> timelines;
		std::map<std::string, std::vector<std::pair<std::string, bool>>> state;
		std::vector<std::pair<std::string, bool>> defaultState(room->RoomEntranceCount(), { "DISCONNECTED", false });
		for (Connection *connection : room->connections) {
			if (connection->timelineType != TimelineType::ALL) continue;

			if (connection->roomA == room) {
				defaultState[connection->connectionA] = { toUpper(connection->roomB->roomName), false };
			} else {
				defaultState[connection->connectionB] = { toUpper(connection->roomA->roomName), false };
			}
		}

		for (Connection *connection : room->connections) {
			if (connection->timelineType != TimelineType::EXCEPT || connection->timelines.size() == 0) continue;

			parseConditionalLinkConnection(file, room, connection, timelines, state, defaultState);
		}

		for (Connection *connection : room->connections) {
			if (connection->timelineType != TimelineType::ONLY || connection->timelines.size() == 0) continue;

			parseConditionalLinkConnection(file, room, connection, timelines, state, defaultState);
		}

		roomDefaultStates[room->roomName] = defaultState;

		if (room->timelineType == TimelineType::ALL || room->timelines.size() == 0) {
			continue;
		}

		bool first = true;
		for (std::string timeline : room->timelines) {
			if (!first) file << ",";
			first = false;
			file << timeline;
		}

		file << " : " << ((room->timelineType == TimelineType::ONLY) ? "EXCLUSIVEROOM" : "HIDEROOM");
		file << " : " << toUpper(room->roomName) << "\n";
	}
	file << "END CONDITIONAL LINKS\n\n";

	Logger::info("- Rooms");
	file << "ROOMS\n";
	for (Room *room : EditorState::rooms) {
		if (room->isOffscreen()) continue;

		file << toUpper(room->roomName) << " : ";

		std::vector<std::pair<std::string, bool>> connections = roomDefaultStates[room->roomName];

		for (int i = 0; i < room->RoomEntranceCount(); i++) {
			if (i > 0) file << ", ";

			file << connections[i].first;
		}

		for (std::string tag : room->Tags()) {
			file << " : " << tag;
		}

		file << "\n";
	}
	file << "END ROOMS\n\n";

	Logger::info("- Creatures");
	file << "CREATURES\n";

	for (Room *room : EditorState::rooms) {
		for (int i = 0; i < room->DenCount(); i++) {
			std::vector<const DenLineage *> nonLineageCreatures;

			const Den &den = room->CreatureDen01(i);
			for (int j = 0; j < den.creatures.size(); j++) {
				const DenLineage &creature = den.creatures[j];

				if (creature.lineageTo != nullptr) continue;

				if (creature.type.empty() || creature.count == 0) continue;

				nonLineageCreatures.push_back(&creature);
			}

			for (int j = 0; j < nonLineageCreatures.size(); j++) {
				const DenLineage *mainCreature = nonLineageCreatures[j];
				if (mainCreature == nullptr) continue;

				std::vector<const DenLineage *> sameTimelineCreatures;
				sameTimelineCreatures.push_back(mainCreature);
				nonLineageCreatures[j] = nullptr;
				for (int k = j + 1; k < nonLineageCreatures.size(); k++) {
					const DenLineage *otherCreature = nonLineageCreatures[k];
					if (otherCreature == nullptr) continue;

					if (mainCreature->timelinesMatch(otherCreature)) {
						sameTimelineCreatures.push_back(otherCreature);
						nonLineageCreatures[k] = nullptr;
					}
				}

				if (mainCreature->timelineType != TimelineType::ALL) {
					file << "(";
					if (mainCreature->timelineType == TimelineType::EXCEPT) {
						file << "X-";
					}
					bool first = true;
					for (std::string timeline : mainCreature->timelines) {
						if (!first) file << ",";
						first = false;

						file << timeline;
					}
					file << ")";
				}
	
				if (room == EditorState::offscreenDen) {
					file << "OFFSCREEN : ";
				} else {
					file << toUpper(room->roomName) << " : ";
				}

				bool first = true;

				for (const DenLineage *creature : sameTimelineCreatures) {
					if (!first) file << ", ";
					first = false;

					if (room == EditorState::offscreenDen) {
						file << "0-" << creature->type;
					} else {
						file << (i + room->RoomEntranceCount()) << "-" << creature->type;
					}
					if (!creature->tag.empty()) {
						if (creature->tag == "MEAN") {
							file << "-{Mean:" << creature->data << "}";
						} else if (creature->tag == "LENGTH") {
							if (creature->type == "PoleMimic") {
								file << "-{" << int(creature->data) << "}";
							} else {
								file << "-{" << creature->data << "}";
							}
						} else if (creature->tag == "SEED") {
							file << "-{Seed:" << int(creature->data) << "}";
						} else if (creature->tag == "RotType") {
							file << "-{RotType:" << int(creature->data) << "}";
						} else {
							file << "-{" << creature->tag << "}";
						}
					}
					if (creature->count > 1) file << "-" << creature->count;
				}

				file << "\n";
			}
		}

		for (int i = 0; i < room->DenCount(); i++) {
			const Den &den = room->CreatureDen01(i);
			for (int j = 0; j < den.creatures.size(); j++) {
				const DenLineage &lineage = den.creatures[j];
				const DenCreature *creature = &den.creatures[j];

				if (creature->lineageTo == nullptr) continue;

				if (lineage.timelineType != TimelineType::ALL && lineage.timelines.size() > 0) {
					file << "(";
					if (lineage.timelineType == TimelineType::EXCEPT) {
						file << "X-";
					}
					bool first = true;
					for (std::string timeline : lineage.timelines) {
						if (!first) file << ",";
						first = false;

						file << timeline;
					}
					file << ")";
				}

				file << "LINEAGE : ";

				if (room == EditorState::offscreenDen) {
					file << "OFFSCREEN : ";
				} else {
					file << toUpper(room->roomName) << " : ";
				}

				if (room == EditorState::offscreenDen) {
					file << "0 : ";
				} else {
					file << (i + room->RoomEntranceCount()) << " : ";
				}

				while (creature != nullptr) {
					file << ((creature->type.empty() || creature->count == 0) ? "NONE" : creature->type);

					if (!creature->tag.empty()) {
						if (creature->tag == "MEAN") {
							file << "-{Mean:" << creature->data << "}";
						} else if (creature->tag == "LENGTH") {
							if (creature->type == "PoleMimic") {
								file << "-{" << int(creature->data) << "}";
							} else {
								file << "-{" << creature->data << "}";
							}
						} else if (creature->tag == "SEED") {
							file << "-{Seed:" << int(creature->data) << "}";
						} else if (creature->tag == "RotType") {
							file << "-{RotType:" << int(creature->data) << "}";
						} else {
							file << "-{" << creature->tag << "}";
						}
					}

					if (creature->lineageTo == nullptr) {
						file << "-0\n";
						break;
					}
					file << "-" << std::clamp(creature->lineageChance, 0.0, 1.0) << ", ";

					creature = creature->lineageTo;
				}
			}
		}
	}
	
	file << EditorState::region.complicatedCreatures;

	file << "END CREATURES\n";

	file << EditorState::region.extraWorld;

	file.close();
}

void WorldExporter::exportImageFile(std::filesystem::path outputPath, std::filesystem::path otherPath) {
	Logger::info("Exporting image file");

	std::filesystem::path mapPath = EditorState::region.exportDirectory / ("map_image_" + EditorState::region.acronym + ".txt");
	Backup::backup(mapPath);
	std::fstream mapFile(mapPath, std::ios::out | std::ios::trunc);

	bool hasMapFile = true;

	if (!mapFile.is_open()) {
		Logger::info("Error creating map_image_", EditorState::region.acronym, ".txt");
		hasMapFile = false;
	}

	Vector2 topLeft = Vector2(INFINITY, INFINITY);
	Vector2 bottomRight = Vector2(-INFINITY, -INFINITY);

	for (Room *room : EditorState::rooms) {
		if (room->isOffscreen()) continue;

		double left = room->canonPosition.x;
		double right = room->canonPosition.x + room->Width();
		double top = room->canonPosition.y - room->Height();
		double bottom = room->canonPosition.y;
		topLeft.x = std::min(topLeft.x, left);
		bottomRight.x = std::max(bottomRight.x, right);
		topLeft.y = std::min(topLeft.y, top);
		bottomRight.y = std::max(bottomRight.y, bottom);
	}

	int layerHeight = int(bottomRight.y - topLeft.y) + 20;

	const int textureWidth = int(bottomRight.x - topLeft.x) + 20;
	const int textureHeight = layerHeight * LAYER_COUNT;

	std::vector<unsigned char> image(textureWidth * textureHeight * 3);

	if (Settings::getSetting<bool>(Settings::Setting::DebugVisibleOutputPadding)) {
		for (int x = 0; x < textureWidth; x++) {
			for (int y = 0; y < textureHeight; y++) {
				int i = y * textureWidth + x;

				if (x < 10 || (y % layerHeight) < 10 || x >= textureWidth - 10 || (y % layerHeight) >= layerHeight - 10) {
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
	} else {
		for (int i = 0; i < textureWidth * textureHeight; i++) {
			image[i * 3 + 0] = 0;
			image[i * 3 + 1] = 255;
			image[i * 3 + 2] = 0;
		}
	}

	for (Room *room : EditorState::rooms) {
		if (room->isOffscreen()) continue;
		if (room->data.hidden) continue;

		Vector2i roomPosition = Vector2i(
			int(room->canonPosition.x - topLeft.x),
			int(bottomRight.y - room->canonPosition.y)
		);
		int layerXOffset = 10;
		int layerYOffset = (2 - room->layer) * layerHeight + 10;

		if (hasMapFile) {
			mapFile << toUpper(room->roomName) << ": " << (roomPosition.x + layerXOffset) << "," << (roomPosition.y + layerYOffset) << "," << room->Width() << "," << room->Height() << "\n";
		}
		
		for (int ox = 0; ox < room->Width(); ox++) {
			for (int oy = 0; oy < room->Height(); oy++) {
				if (roomPosition.x + ox + layerXOffset < 0) {
					continue;
				}
				if (roomPosition.x + ox + layerXOffset >= textureWidth) {
					continue;
				}
				if (roomPosition.y + oy + layerYOffset < 0) {
					continue;
				}
				if (roomPosition.y + oy + layerYOffset >= textureHeight) {
					continue;
				}

				int i = ((roomPosition.y + layerYOffset + oy) * textureWidth + roomPosition.x + layerXOffset + ox) * 3;
				unsigned int tileType = room->getTile(ox, oy) % 16;
				unsigned int tileData = room->getTile(ox, oy) / 16;
				
				int r = 0;
				int g = 0;
				int b = 0;

				if (tileType == 0 || tileType == 4 || tileType == 5) {
					r = 255; g = 0; // #FF0000
				}
				if (tileType == 1) {
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

	Backup::backup(outputPath);
	if (stbi_write_png(outputPath.generic_u8string().c_str(), textureWidth, textureHeight, 3, image.data(), textureWidth * 3)) {
		Logger::info("Image file exported");
	} else {
		Logger::error("Exporting image failed");
	}
	
	if (hasMapFile) mapFile.close();
}

void WorldExporter::exportPropertiesFile(std::filesystem::path outputPath) {
	Logger::info("Exporting properties file");

	Backup::backup(outputPath);
	std::fstream propertiesFile(outputPath, std::ios::out | std::ios::trunc);
	
	propertiesFile << EditorState::region.extraProperties;

	for (std::string subregion : EditorState::subregions) {
		propertiesFile << "Subregion: " << subregion << "\n";
	}

	for (Room *room : EditorState::rooms) {
		if (room->isOffscreen()) continue;
		
		if (room->data.attractiveness.empty()) continue;
		
		
		propertiesFile << "Room_Attr: " << toUpper(room->roomName) << ": ";
		for (std::pair<std::string, RoomAttractiveness> attractivenss : room->data.attractiveness)  {
			propertiesFile << attractivenss.first << "-";
			if (attractivenss.second == RoomAttractiveness::NEUTRAL) propertiesFile << "Neutral";
			if (attractivenss.second == RoomAttractiveness::FORBIDDEN) propertiesFile << "Forbidden";
			if (attractivenss.second == RoomAttractiveness::AVOID) propertiesFile << "Avoid";
			if (attractivenss.second == RoomAttractiveness::LIKE) propertiesFile << "Like";
			if (attractivenss.second == RoomAttractiveness::STAY) propertiesFile << "Stay";
			propertiesFile << ",";
		}
		propertiesFile << "\n";
	}

	for (std::pair<const int, Colour> item : EditorState::region.overrideSubregionColors) {
		propertiesFile << "//FloodForge|SubregionColorOverride|" << item.first << "|" << colourToString(item.second) << "\n";
	}

	propertiesFile.close();
}
