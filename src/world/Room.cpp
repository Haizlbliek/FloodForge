#include "Room.hpp"

#include "DenPopup.hpp"

Colour RoomHelpers::RoomAir;
Colour RoomHelpers::RoomSolid;
Colour RoomHelpers::RoomPole;
Colour RoomHelpers::RoomPlatform;
Colour RoomHelpers::RoomShortcutEnterance;
Colour RoomHelpers::RoomShortcutDot;
Colour RoomHelpers::RoomShortcutRoom;
Colour RoomHelpers::RoomShortcutDen;
Colour RoomHelpers::RoomConnection;
Colour RoomHelpers::RoomConnectionHover;

void RoomHelpers::loadColours() {
	RoomAir               = currentTheme[ThemeColour::RoomAir];
	RoomSolid             = currentTheme[ThemeColour::RoomSolid];
	RoomPole              = currentTheme[ThemeColour::RoomPole];
	RoomPlatform          = currentTheme[ThemeColour::RoomPlatform];
	RoomShortcutEnterance = currentTheme[ThemeColour::RoomShortcutEnterance];
	RoomShortcutDot       = currentTheme[ThemeColour::RoomShortcutDot];
	RoomShortcutRoom      = currentTheme[ThemeColour::RoomShortcutRoom];
	RoomShortcutDen       = currentTheme[ThemeColour::RoomShortcutDen];
	RoomConnection        = currentTheme[ThemeColour::RoomConnection];
	RoomConnectionHover   = currentTheme[ThemeColour::RoomConnectionHover];
}

void RoomHelpers::drawTexture(GLuint texture, double rectX, double rectY, double scale) {
	Draw::color(1.0, 1.0, 1.0);
	glEnable(GL_BLEND);
	Draw::useTexture(texture);
	Draw::begin(Draw::QUADS);

	int w, h;
	glBindTexture(GL_TEXTURE_2D, texture);
	glGetTexLevelParameteriv(GL_TEXTURE_2D, 0, GL_TEXTURE_WIDTH,  &w);
	glGetTexLevelParameteriv(GL_TEXTURE_2D, 0, GL_TEXTURE_HEIGHT, &h);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
	glBindTexture(GL_TEXTURE_2D, 0);

	float ratio = (float(w) / float(h) + 1.0) * 0.5;
	float uvx = 1.0 / ratio;
	float uvy = ratio;
	if (uvx < 1.0) {
		uvy /= uvx;
		uvx = 1.0;
	}
	if (uvy < 1.0) {
		uvx /= uvy;
		uvy = 1.0;
	}
	uvx *= 0.5;
	uvy *= 0.5;
	
	double centreX = rectX + 0.5;
	double centreY = rectY - 0.5;
	Draw::texCoord(0.5 - uvx, 0.5 + uvy); Draw::vertex(centreX - scale * 0.5, centreY - scale * 0.5);
	Draw::texCoord(0.5 + uvx, 0.5 + uvy); Draw::vertex(centreX + scale * 0.5, centreY - scale * 0.5);
	Draw::texCoord(0.5 + uvx, 0.5 - uvy); Draw::vertex(centreX + scale * 0.5, centreY + scale * 0.5);
	Draw::texCoord(0.5 - uvx, 0.5 - uvy); Draw::vertex(centreX - scale * 0.5, centreY + scale * 0.5);
	Draw::end();
	Draw::useTexture(0);
	glDisable(GL_BLEND);
}

Room::Room(std::string path, std::string name) {
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

	images = 0;
	data = ExtraRoomData();

	loadGeometry();
	generateVBO();
	checkImages();
}

Room::~Room() {
	if (geometry != nullptr) delete[] geometry;

	geometry = nullptr;

	glDeleteBuffers(2, vbo);
	glDeleteVertexArrays(1, &vao);
}

void Room::drawBlack(Vector2 mousePosition, double lineSize, Vector2 screenBounds) {
	if (data.hidden) {
		Draw::color(RoomHelpers::RoomSolid.r, RoomHelpers::RoomSolid.g, RoomHelpers::RoomSolid.b, 0.5);
	} else {
		Draw::color(RoomHelpers::RoomSolid.r, RoomHelpers::RoomSolid.g, RoomHelpers::RoomSolid.b, 1.0);
	}
	
	fillRect(position.x, position.y - height, position.x + width, position.y);
}

void Room::draw(Vector2 mousePosition, double lineSize, Vector2 screenBounds) {
	if (!valid) {
		Draw::color(1.0, 0.0, 0.0);
		
		Draw::begin(Draw::LINES);
		Draw::vertex(position.x, position.y);
		Draw::vertex(position.x + width, position.y - height);

		Draw::vertex(position.x + width, position.y);
		Draw::vertex(position.x, position.y - height);
		Draw::end();
		
		strokeRect(position.x, position.y, position.x + width, position.y - height);

		return;
	}
	
	Colour tint = Colour(1.0, 1.0, 1.0);

	if (EditorState::roomColours == 1) {
		if (layer == 0) tint = Colour(1.0, 0.0, 0.0);
		if (layer == 1) tint = Colour(1.0, 1.0, 1.0);
		if (layer == 2) tint = Colour(0.0, 1.0, 0.0);
	}
	
	if (EditorState::roomColours == 2) {
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

	glEnable(GL_BLEND);
	
	glBindVertexArray(vao);
	glUseProgram(Shaders::roomShader);

	GLuint projLoc = glGetUniformLocation(Shaders::roomShader, "projection");
	GLuint modelLoc = glGetUniformLocation(Shaders::roomShader, "model");
	GLuint tintLoc = glGetUniformLocation(Shaders::roomShader, "tintColour");

	glUniformMatrix4fv(projLoc, 1, GL_FALSE, projectionMatrix(EditorState::cameraOffset, EditorState::cameraScale * screenBounds).m);
	glUniformMatrix4fv(modelLoc, 1, GL_FALSE, modelMatrix(position.x, position.y).m);
	if (data.hidden) {
		glUniform4f(tintLoc, tint.r, tint.g, tint.b, 0.5f);
	} else {
		glUniform4f(tintLoc, tint.r, tint.g, tint.b, tint.a);
	}

	glDrawElements(GL_TRIANGLES, indices.size(), GL_UNSIGNED_INT, nullptr);

	glBindVertexArray(0);
	glUseProgram(0);

	if (water != -1) {
		Draw::color(0.0, 0.0, 0.5, 0.5);
		fillRect(position.x, position.y - (height - std::min(water, height)), position.x + width, position.y - height);
	}

	glDisable(GL_BLEND);

	if (EditorState::visibleDevItems) {
		for (DevItem item : data.devItems) {
			if (item.texture == 0) continue;
	
			double rectX = position.x + item.position.x;
			double rectY = position.y - height + item.position.y;
			
			RoomHelpers::drawTexture(item.texture, rectX, rectY, 0.5);
		}
	}

	for (int i = 0; i < denEntrances.size(); i++) {
		if (dens[i].type == "" || dens[i].count == 0) continue;

		double rectX = position.x + denEntrances[i].x;
		double rectY = position.y - denEntrances[i].y;
		double scale = EditorState::selectorScale;
		
		if (i == hoveredDen) scale *= 1.5;

		RoomHelpers::drawTexture(CreatureTextures::getTexture(dens[i].type), rectX, rectY, scale);

		Draw::color(1.0, 0.0, 0.0);
		Fonts::rainworld->writeCentred(std::to_string(dens[i].count), rectX + 0.5 + scale * 0.25, rectY - 0.5 - scale * 0.5, 0.5 * scale, CENTRE_XY);
	}

	if (inside(mousePosition)) {
		setThemeColour(ThemeColour::RoomBorderHighlight);
	} else {
		setThemeColour(ThemeColour::RoomBorder);
	}
	strokeRect(position.x, position.y, position.x + width, position.y - height);
}

bool Room::inside(Vector2 otherPosition) {
	return (
		otherPosition.x >= position.x &&
		otherPosition.y >= position.y - height &&
		otherPosition.x <= position.x + width &&
		otherPosition.y <= position.y
	);
}

bool Room::intersects(Vector2 corner0, Vector2 corner1) {
	Vector2 cornerMin = Vector2::min(corner0, corner1);
	Vector2 cornerMax = Vector2::max(corner0, corner1);

	return (
		cornerMin.x <= position.x + width &&
		cornerMin.y <= position.y &&
		cornerMax.x >= position.x &&
		cornerMax.y >= position.y - height
	);
}

int Room::getTile(int x, int y) const {
	if (!valid) return 1;

	if (x < 0 || y < 0) return 1;
	if (x >= width || y >= height) return 1;

	return geometry[x * height + y];
}

bool Room::tileIsShortcut(int x, int y) const {
	int tile = getTile(x, y);
	
	return (tile & (256 | 128 | 64)) > 0;
}

const std::vector<Vector2> Room::ShortcutEntranceOffsetPositions() const {
	std::vector<Vector2> transformedEntrances;

	for (const std::pair<Vector2i, int> &connection : shortcutEntrances) {
		transformedEntrances.push_back(Vector2(
			position.x + connection.first.x + 0.5,
			position.y - connection.first.y - 0.5
		));
	}

	return transformedEntrances;
}

int Room::getShortcutEntranceId(const Vector2i &searchPosition) const {
	unsigned int connectionId = 0;

	for (const std::pair<Vector2i, int> &connection : shortcutEntrances) {
		if (connection.first == searchPosition) {
			return connectionId;
		}

		connectionId++;
	}

	return -1;
}

const Vector2 Room::getRoomEntranceOffsetPosition(unsigned int connectionId) const {
	Vector2i connection = getRoomEntrancePosition(connectionId);

	return Vector2(
		position.x + connection.x + 0.5,
		position.y - connection.y - 0.5
	);
}

int Room::getRoomEntranceId(const Vector2i &searchPosition) const {
	int index = 0;
	for (const Vector2i enterance : roomEntrances) {
		if (enterance == searchPosition) {
			return index;
		}

		index++;
	}

	return -1;
}

const Vector2i Room::getRoomEntrancePosition(unsigned int connectionId) const {
	if (connectionId >= roomEntrances.size()) return Vector2i(-1, -1);

	return roomEntrances[connectionId];
}

Vector2 Room::getRoomEntranceDirectionVector(unsigned int connectionId) const {
	return MathUtils::directionToVector(getRoomEntranceDirection(connectionId));
}

Direction Room::getRoomEntranceDirection(unsigned int connectionId) const {
	Vector2i connection = roomEntrances[connectionId];

	if (tileIsShortcut(connection.x - 1, connection.y)) {
		return Direction::LEFT;
	} else if (tileIsShortcut(connection.x, connection.y + 1)) {
		return Direction::DOWN;
	} else if (tileIsShortcut(connection.x + 1, connection.y)) {
		return Direction::RIGHT;
	} else if (tileIsShortcut(connection.x, connection.y - 1)) {
		return Direction::UP;
	}

	return UNKNOWN;
}

bool Room::canConnect(unsigned int connectionId) {
	if (shortcutEntrances.size() <= connectionId) return false;
	
	return true;
}

void Room::connect(Room *room, unsigned int connectionId) {
	roomConnections.insert(std::pair<Room*, unsigned int> { room, connectionId });
}

void Room::disconnect(Room *room, unsigned int connectionId) {
	roomConnections.erase(std::pair<Room*, unsigned int> { room, connectionId });
}

bool Room::Connected(Room *room, unsigned int connectionId) const {
	return roomConnections.find(std::pair<Room*, unsigned int> { room, connectionId }) != roomConnections.end();
}

bool Room::RoomUsed(Room *room) const {
	for (std::pair<Room*, unsigned int> connection : roomConnections) {
		if (connection.first == room) return true;
	}

	return false;
}

bool Room::ConnectionUsed(unsigned int connectionId) const {
	for (std::pair<Room*, unsigned int> connection : roomConnections) {
		if (connection.second == connectionId) return true;
	}

	return false;
}

const std::vector<Room*> Room::ConnectedRooms() const {
	std::vector<Room*> connectedRooms;

	for (std::pair<Room*, unsigned int> connection : roomConnections) {
		connectedRooms.push_back(connection.first);
	}

	return connectedRooms;
}

const std::set<std::pair<Room*, unsigned int>> Room::RoomConnections() const {
	return roomConnections;
}

int Room::RoomEntranceCount() const {
	return roomEntrances.size();
}

const std::vector<std::pair<Vector2i, ShortcutType>> Room::ShortcutConnections() const {
	return shortcutEntrances;
}

const std::vector<Vector2i> Room::RoomEntrances() const {
	return roomEntrances;
}

const int Room::DenId(Vector2i coord) const {
	auto it = find(denEntrances.begin(), denEntrances.end(), coord);

	if (it == denEntrances.end()) return -1;

	return (it - denEntrances.begin()) + roomEntrances.size();
}

bool Room::CreatureDenExists(int id) {
	return CreatureDen01Exists(id - roomEntrances.size());
}

bool Room::CreatureDen01Exists(int id) {
	return (id >= 0 && id < dens.size());
}

Den &Room::CreatureDen(int id) {
	return CreatureDen01(id - roomEntrances.size());
}

Den &Room::CreatureDen01(int id) {
	if (id < 0 || id >= dens.size()) {
		Logger::log("INVALID DEN ", id);
		throw "INVALID DEN";
	}

	return dens[id];
}

const int Room::DenCount() const {
	return denEntrances.size();
}

const std::vector<Vector2i> Room::DenEntrances() const {
	return denEntrances;
}

const std::vector<Den> Room::Dens() const {
	return dens;
}

const int Room::Width() const { return width; }
const int Room::Height() const { return height; }

void Room::SetTag(const std::string newTag) { tags.clear(); if (newTag != "") tags.push_back(newTag); }
void Room::ToggleTag(const std::string newTag) {
	if (std::find(tags.begin(), tags.end(), newTag) != tags.end()) {
		tags.erase(std::remove(tags.begin(), tags.end(), newTag), tags.end());
	} else {
		tags.push_back(newTag);
	}
}
const std::vector<std::string> Room::Tags() const { return tags; }

const int Room::Images() const { return images; }

std::vector<uint8_t> Room::parseStringToUint8Vector(const std::string& input) {
	std::vector<uint8_t> result;
	std::stringstream ss(input);
	std::string token;

	while (std::getline(ss, token, ',')) {
		result.push_back(static_cast<uint8_t>(std::stoi(token)));
	}

	return result;
}

void Room::ensureConnections() {
	std::vector<std::pair<Vector2i, ShortcutType>> verifiedConnections;

	for (int i = shortcutEntrances.size() - 1; i >= 0; i--) {
		Vector2i connection = shortcutEntrances[i].first;

		Vector2i forwardDirection = Vector2i(0, 0);
		bool hasDirection = true;

		if (tileIsShortcut(connection.x - 1, connection.y)) {
			forwardDirection.x = -1;
		} else if (tileIsShortcut(connection.x, connection.y + 1)) {
			forwardDirection.y = 1;
		} else if (tileIsShortcut(connection.x + 1, connection.y)) {
			forwardDirection.x = 1;
		} else if (tileIsShortcut(connection.x, connection.y - 1)) {
			forwardDirection.y = -1;
		}

		if (forwardDirection.x == 0 && forwardDirection.y == 0) continue;

		int runs = 0;
		while (runs++ < 10000) {
			connection += forwardDirection;

			if (!tileIsShortcut(connection.x + forwardDirection.x, connection.y + forwardDirection.y)) {
				Vector2i lastDirection = Vector2i(forwardDirection);

				forwardDirection.x = 0;
				forwardDirection.y = 0;
				hasDirection = false;
				if (     lastDirection.x !=  1 && tileIsShortcut(connection.x - 1, connection.y    )) { forwardDirection.x = -1; hasDirection = true; }
				else if (lastDirection.y != -1 && tileIsShortcut(connection.x,     connection.y + 1)) { forwardDirection.y =  1; hasDirection = true; }
				else if (lastDirection.x != -1 && tileIsShortcut(connection.x + 1, connection.y    )) { forwardDirection.x =  1; hasDirection = true; }
				else if (lastDirection.y !=  1 && tileIsShortcut(connection.x,     connection.y - 1)) { forwardDirection.y = -1; hasDirection = true; }
			}

			if (getTile(connection.x, connection.y) % 16 == 4) {
				hasDirection = true;
				break;
			}
			
			if (!hasDirection) break;
		}

		if (hasDirection) verifiedConnections.push_back(std::make_pair(connection, shortcutEntrances[i].second));
	}

	std::reverse(verifiedConnections.begin(), verifiedConnections.end());

	for (size_t i = 0; i < shortcutEntrances.size(); ++i) {
		for (size_t j = 0; j < shortcutEntrances.size() - i - 1; ++j) {
			const Vector2i &a = shortcutEntrances[j].first;
			const Vector2i &b = shortcutEntrances[j + 1].first;

			if (a.y > b.y || (a.y == b.y && a.x > b.x)) {
				std::swap(shortcutEntrances[j], shortcutEntrances[j + 1]);
				std::swap(verifiedConnections[j], verifiedConnections[j + 1]);
			}
		}
	}
	
	for (std::pair<Vector2i, ShortcutType> verifiedConnection : verifiedConnections) {
		ShortcutType type = verifiedConnection.second;

		if (type == ShortcutType::ROOM) {
			roomEntrances.push_back(verifiedConnection.first);
		} else if (type == ShortcutType::DEN) {
			denEntrances.push_back(verifiedConnection.first);
		}
	}
}

void Room::loadGeometry() {
	std::fstream geometryFile(path);
	if (!geometryFile.is_open() || !std::filesystem::exists(path)) {
		EditorState::fails.push_back("Failed to load '" + roomName + "' - Doesn't exist");
		Logger::log("Failed to load '", path, "' - Doesn't exist.");
		width = 72;
		height = 43;
		water = -1;
		geometry = new int[width * height];
		for (int i = 0; i < width * height; i++) {
			geometry[i] = 0;
		}
		valid = false;
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
	
	std::getline(geometryFile, tempLine); // Cameras
	std::vector<std::string> items = split(tempLine, '|');
	images = items.size();
	
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
					shortcutEntrances.push_back(std::make_pair( Vector2i(tileId / height, tileId % height), ShortcutType::ROOM ));
					break;
				case 5: // Den
					geometry[tileId] += 256;
					shortcutEntrances.push_back(std::make_pair( Vector2i(tileId / height, tileId % height), ShortcutType::DEN ));
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

void Room::checkImages() {
	if (!Settings::getSetting<bool>(Settings::Setting::WarnMissingImages)) return;
	
	std::string imageDir = path.substr(0, path.find_last_of(std::filesystem::path::preferred_separator));
	for (int i = 0; i < images; i++) {
		std::string imagePath = roomName + "_" + std::to_string(i + 1) + ".png";
		
		std::string foundPath = findFileCaseInsensitive(imageDir, imagePath);
		
		if (foundPath.empty()) {
			EditorState::fails.push_back("Can't find '" + imagePath + "'");
		}
	}
}

void Room::generateVBO() {
	vertices.clear();
	indices.clear();
	cur_index = 0;

	glGenBuffers(2, vbo);
	glGenVertexArrays(2, &vao);

	for (int x = 0; x < width; x++) {
		for (int y = 0; y < height; y++) {
			int tileType = getTile(x, y) % 16;
			int tileData = getTile(x, y) / 16;

			float x0 = position.x + x;
			float y0 = position.y - y;
			float x1 = position.x + x + 1;
			float y1 = position.y - y - 1;
			float x2 = (x0 + x1) * 0.5;
			float y2 = (y0 + y1) * 0.5;

			// Background air
			if (tileType != 1 && tileType != 4) {
				addQuad(
					{ x0, y0, RoomHelpers::RoomAir },
					{ x1, y0, RoomHelpers::RoomAir },
					{ x1, y1, RoomHelpers::RoomAir },
					{ x0, y1, RoomHelpers::RoomAir }
				);
			}

			// Shortcut Entrance
			if (tileType == 4) {
				addQuad(
					{ x0, y0, RoomHelpers::RoomShortcutEnterance },
					{ x1, y0, RoomHelpers::RoomShortcutEnterance },
					{ x1, y1, RoomHelpers::RoomShortcutEnterance },
					{ x0, y1, RoomHelpers::RoomShortcutEnterance }
				);
			}
			
			// Slope
			if (tileType == 2) {
				int bits = 0;
				bits += (getTile(x - 1, y) == 1) ? 1 : 0;
				bits += (getTile(x + 1, y) == 1) ? 2 : 0;
				bits += (getTile(x, y - 1) == 1) ? 4 : 0;
				bits += (getTile(x, y + 1) == 1) ? 8 : 0;

				if (bits == 1 + 4) {
					addQuad(
						{ x0, y0, RoomHelpers::RoomSolid },
						{ x1, y0, RoomHelpers::RoomSolid },
						{ x0, y1, RoomHelpers::RoomSolid },
						{ x0, y0, RoomHelpers::RoomSolid }
					);
				} else if (bits == 1 + 8) {
					addQuad(
						{ x0, y1, RoomHelpers::RoomSolid },
						{ x1, y1, RoomHelpers::RoomSolid },
						{ x0, y0, RoomHelpers::RoomSolid },
						{ x0, y1, RoomHelpers::RoomSolid }
					);
				} else if (bits == 2 + 4) {
					addQuad(
						{ x1, y0, RoomHelpers::RoomSolid },
						{ x0, y0, RoomHelpers::RoomSolid },
						{ x1, y1, RoomHelpers::RoomSolid },
						{ x1, y0, RoomHelpers::RoomSolid }
					);
				} else if (bits == 2 + 8) {
					addQuad(
						{ x1, y1, RoomHelpers::RoomSolid },
						{ x0, y1, RoomHelpers::RoomSolid },
						{ x1, y0, RoomHelpers::RoomSolid },
						{ x1, y1, RoomHelpers::RoomSolid }
					);
				}
			}
			
			// One-way
			if (tileType == 3) {
				addQuad(
					{ x0, y0,               RoomHelpers::RoomPlatform },
					{ x1, y0,               RoomHelpers::RoomPlatform },
					{ x1, (y0 + y1) * 0.5f, RoomHelpers::RoomPlatform },
					{ x0, (y0 + y1) * 0.5f, RoomHelpers::RoomPlatform }
				);
			}

			// 16 - Vertical Pole
			if (tileData & 1) {
				addQuad(
					{ x0 + 0.375f, y0, RoomHelpers::RoomPole },
					{ x1 - 0.375f, y0, RoomHelpers::RoomPole },
					{ x1 - 0.375f, y1, RoomHelpers::RoomPole },
					{ x0 + 0.375f, y1, RoomHelpers::RoomPole }
				);
			}

			// 32 - Horizontal Pole
			if (tileData & 2) {
				addQuad(
					{ x0, y0 - 0.375f, RoomHelpers::RoomPole },
					{ x1, y0 - 0.375f, RoomHelpers::RoomPole },
					{ x1, y1 + 0.375f, RoomHelpers::RoomPole },
					{ x0, y1 + 0.375f, RoomHelpers::RoomPole }
				);
			}

			// 128 - Shortcut
			if (tileData & 8) {
				addQuad(
					{ x0 + 0.40625f, y0 - 0.40625f, RoomHelpers::RoomSolid },
					{ x1 - 0.40625f, y0 - 0.40625f, RoomHelpers::RoomSolid },
					{ x1 - 0.40625f, y1 + 0.40625f, RoomHelpers::RoomSolid },
					{ x0 + 0.40625f, y1 + 0.40625f, RoomHelpers::RoomSolid }
				);

				addQuad(
					{ x0 + 0.4375f, y0 - 0.4375f, RoomHelpers::RoomShortcutDot },
					{ x1 - 0.4375f, y0 - 0.4375f, RoomHelpers::RoomShortcutDot },
					{ x1 - 0.4375f, y1 + 0.4375f, RoomHelpers::RoomShortcutDot },
					{ x0 + 0.4375f, y1 + 0.4375f, RoomHelpers::RoomShortcutDot }
				);
			}
		}
	}

	for (Vector2i &shortcutEntrance : roomEntrances) {
		float x0 = position.x + shortcutEntrance.x;
		float y0 = position.y - shortcutEntrance.y;
		float x1 = position.x + shortcutEntrance.x + 1;
		float y1 = position.y - shortcutEntrance.y - 1;
		
		addQuad(
			{ x0 + 0.25f, y0 - 0.25f, RoomHelpers::RoomShortcutRoom },
			{ x1 - 0.25f, y0 - 0.25f, RoomHelpers::RoomShortcutRoom },
			{ x1 - 0.25f, y1 + 0.25f, RoomHelpers::RoomShortcutRoom },
			{ x0 + 0.25f, y1 + 0.25f, RoomHelpers::RoomShortcutRoom }
		);
	}

	for (Vector2i &shortcutEntrance : denEntrances) {
		float x0 = position.x + shortcutEntrance.x;
		float y0 = position.y - shortcutEntrance.y;
		float x1 = position.x + shortcutEntrance.x + 1;
		float y1 = position.y - shortcutEntrance.y - 1;
		
		addQuad(
			{ x0 + 0.25f, y0 - 0.25f, RoomHelpers::RoomShortcutDen },
			{ x1 - 0.25f, y0 - 0.25f, RoomHelpers::RoomShortcutDen },
			{ x1 - 0.25f, y1 + 0.25f, RoomHelpers::RoomShortcutDen },
			{ x0 + 0.25f, y1 + 0.25f, RoomHelpers::RoomShortcutDen }
		);
	}

	glBindVertexArray(vao);
	glBindBuffer(GL_ARRAY_BUFFER, vbo[0]);
	glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(Vertex), vertices.data(), GL_STATIC_DRAW);

	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, vbo[1]);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(uint32_t), indices.data(), GL_STATIC_DRAW);

	glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)0);
	glEnableVertexAttribArray(0);

	glVertexAttribPointer(1, 4, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)(sizeof(float) * 2));
	glEnableVertexAttribArray(1);

	glBindVertexArray(0);
}

void Room::addQuad(const Vertex &a, const Vertex &b, const Vertex &c, const Vertex &d) {
	vertices.push_back(a);
	vertices.push_back(b);
	vertices.push_back(c);
	vertices.push_back(d);

	indices.push_back(cur_index + 0);
	indices.push_back(cur_index + 1);
	indices.push_back(cur_index + 2);
	indices.push_back(cur_index + 2);
	indices.push_back(cur_index + 3);
	indices.push_back(cur_index + 0);
	cur_index += 4;
}