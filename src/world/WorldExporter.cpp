#include "WorldExporter.hpp"

#include "../math/Rect.hpp"

void WorldExporter::exportMapFile() {
	std::fstream file(EditorState::region.exportDirectory / ("map_" + EditorState::region.acronym + ".txt"), std::ios::out | std::ios::trunc);

	if (!file.is_open()) {
		Logger::log("Error opening map_", EditorState::region.acronym, ".txt");
		return;
	}

	Logger::log("Exporting rooms");
	for (Room *room : EditorState::rooms) {
		const Vector2 &roomPosition = room->position;
		Vector2 position = Vector2(
			(roomPosition.x + room->Width() * 0.5) * 3.0,
			(roomPosition.y - room->Height() * 0.5) * 3.0
		);

		file << std::setprecision(12);
		file << toUpper(room->roomName) << ": ";
		file << position.x << "><" << position.y << "><"; // Canon Position
		file << position.x << "><" << position.y << "><"; // Dev Position
		file << room->layer << "><";
		if (room->subregion > -1) file << EditorState::subregions[room->subregion];
		file << "\n";
	}
	
	Logger::log("Exporting extra data");
	for (Room *room : EditorState::rooms) {
		file << "//FloodForge;ROOM|" << room->roomName;
		if (room->data.hidden) file << "|hidden";
		if (room->data.merge) file << "|merge";
		file << "\n";
	}

	Logger::log("Exporting connections");
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
		file << connection->roomA->getRoomEntranceDirection(connection->connectionA) << ",";
		file << connection->roomB->getRoomEntranceDirection(connection->connectionB);
		file << "\n";
	}

	file.close();
}

void WorldExporter::exportWorldFile() {
	std::fstream file(EditorState::region.exportDirectory / ("world_" + EditorState::region.acronym + ".txt"), std::ios::out | std::ios::trunc);

	if (!file.is_open()) {
		Logger::log("Error opening world_", EditorState::region.acronym, ".txt");
		return;
	}

	file << "ROOMS\n";
	for (Room *room : EditorState::rooms) {
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
		// if (room->SetTag() != "") file << " : " << room->SetTag();

		file << "\n";
	}
	file << "END ROOMS\n\n";

	file << "CREATURES\n";

	for (Room *room : EditorState::rooms) {
		std::stringstream line;
		bool add = false;

		if (room == EditorState::offscreenDen) {
			line << "OFFSCREEN : ";
		} else {
			line << toUpper(room->roomName) << " : ";
		}

		bool first = true;
		for (int i = 0; i < room->DenCount(); i++) {
			const Den &den = room->CreatureDen01(i);
			if (den.type.empty() || den.count == 0)
				continue;

			if (!first) line << ", ";
			first = false;

			if (room == EditorState::offscreenDen) {
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
	
	file << EditorState::region.complicatedCreatures;

	file << "END CREATURES\n";

	file << EditorState::region.extraWorld;

	file.close();
}

void WorldExporter::exportImageFile(std::filesystem::path outputPath, std::filesystem::path otherPath) {
	std::fstream mapFile(EditorState::region.exportDirectory / ("map_image_" + EditorState::region.acronym + ".txt"), std::ios::out | std::ios::trunc);
	bool hasMapFile = true;

	if (!mapFile.is_open()) {
		Logger::log("Error creating map_image_", EditorState::region.acronym, ".txt");
		hasMapFile = false;
	}

	Rect bounds = Rect();
	bounds.x0 = INFINITY;
	bounds.x1 = -INFINITY;
	bounds.y0 = INFINITY;
	bounds.y1 = -INFINITY;

	for (Room *room : EditorState::rooms) {
		if (room->isOffscreen()) continue;
		if (room->data.hidden) continue;

		double left = room->position.x;
		double right = room->position.x + room->Width();
		double top = -room->position.y + room->Height();
		double bottom = -room->position.y;
		bounds.x0 = std::min(bounds.x0, left);
		bounds.x1 = std::max(bounds.x1, right);
		bounds.y0 = std::min(bounds.y0, bottom);
		bounds.y1 = std::max(bounds.y1, top);
	}

	const int padding = 10;

	const int width = std::floor(bounds.x1 - bounds.x0 + padding * 2);
	const int height = std::floor(bounds.y1 - bounds.y0 + padding * 2) * LAYER_COUNT;

	std::vector<unsigned char> image(width * height * 3);

	if (Settings::getSetting<bool>(Settings::Setting::DebugVisibleOutputPadding)) {
		for (int x = 0; x < width; x++) {
			for (int y = 0; y < height; y++) {
				int i = y * width + x;

				if (x < padding || (y % (height / LAYER_COUNT)) < padding || x >= width - padding || (y % (height / LAYER_COUNT)) >= height / LAYER_COUNT - padding) {
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
		for (int i = 0; i < width * height; i++) {
			image[i * 3 + 0] = 0;
			image[i * 3 + 1] = 255;
			image[i * 3 + 2] = 0;
		}
	}

	for (Room *room : EditorState::rooms) {
		if (room->isOffscreen()) continue;
		if (room->data.hidden) continue;

		// Top left corner
		int x = std::floor(room->position.x - bounds.x0) + padding;
		int y = std::floor(-room->position.y - bounds.y0) + padding;
		y += (2 - room->layer) * height / LAYER_COUNT;
		
		if (hasMapFile) {
			mapFile << toUpper(room->roomName) << ": " << x << "," << (height - y - room->Height()) << "," << room->Width() << "," << room->Height() << "\n";
		}

		for (int ox = 0; ox < room->Width(); ox++) {
			for (int oy = 0; oy < room->Height(); oy++) {
				int i = ((y + oy) * width + x + ox) * 3;

				unsigned int tileType = room->getTile(ox, oy) % 16;
				unsigned int tileData = room->getTile(ox, oy) / 16;

				int r = 0;
				int g = 0;
				int b = 0;

				if (tileType == 0 || tileType == 4) {
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

	if (stbi_write_png(outputPath.string().c_str(), width, height, 3, image.data(), width * 3)) {
		Logger::log("Image saved successfully!");
	} else {
		Logger::log("Error saving image!");
	}
	
	if (hasMapFile) mapFile.close();
}

void WorldExporter::exportPropertiesFile(std::filesystem::path outputPath) {
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

	propertiesFile.close();
}
