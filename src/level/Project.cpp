#include "Project.hpp"

#include <iostream>
#include <filesystem>
#include <set>
#include <vector>

#include "Constants.hpp"
#include "Render.hpp"

void Project::save() {
	while (filePath == "") {
		filePath = OpenNewFileDialog();

		if (!endsWith(filePath, ".level")) {
			filePath = "";

			int result = verifyBox("The file must end in .level.\nTry again?");

			if (!result) return;

			continue;
		}

		name = (std::filesystem::path(filePath)).stem().string();

		break;
	}


	std::ofstream file;
	file.open(filePath);
	file << std::to_string(layer1->Width()) << "|" << std::to_string(layer1->Height()) << "|" << name << "\n";

	for (int i = 1; i <= 3; i++) {
		for (unsigned int x = 0; x < width; x++) {
			for (unsigned int y = 0; y < height; y++) {
				unsigned int tile = GetLayer(i)->getTile(x, y);

				file << tile;

				for (const uint8_t &item : GetLayer(i)->getData(x, y)) {
					file << "," << std::to_string(item);
				}

				file << "|";

			}
		}

		file << "|\n";
	}

	file.close();
}

void Project::render() {
	renderCollision();

	Render::render(this, (OUTPUT_PATH + name + "_1.png").c_str());
}

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

Project *Project::load(std::string name) {
	return Project::loadFromPath(SAVE_PATH + name + ".level");
}

Project *Project::loadFromPath(std::string path) {
	std::ifstream file;
	file.open(path);

	if (!file.is_open()) {
		std::cerr << "Failed to open file!" << std::endl;
		return nullptr;
	}

	std::string temp;
	std::getline(file, temp, '|');
	unsigned int width = std::stoi(temp);
	std::getline(file, temp, '|');
	unsigned int height = std::stoi(temp);
	std::getline(file, temp);

	Project *project = new Project(temp, width, height);
	project->filePath = path;
	project->name = temp;

	unsigned int x = 0;
	unsigned int y = 0;
	unsigned int layer = 1;
	while (std::getline(file, temp, '|')) {
		if (temp == "\n") continue;
		
		if (temp == "") {
			x = 0;
			y = 0;
			layer++;
			continue;
		}

		std::vector<uint8_t> values = parseStringToUint8Vector(temp);

		project->GetLayer(layer)->setTile(x, y, values[0]);

		for (int i = 1; i < values.size(); i++) {
			project->GetLayer(layer)->addData(x, y, values[i]);
		}

		y++;
		if (y >= height) {
			y = 0;
			x++;
		}
	}

	return project;
}

unsigned int parseDrizzleTile(Grid *layer, unsigned int x, unsigned int y) {
	unsigned int tile = layer->getTile(x, y);

	if (layer->hasData(x, y, DATA_SHORTCUT) && getShortcutTile(layer, x, y) <= 3) return 7;

	switch (tile) {
		case 0: return 0;
		case 1: return 1;
		case 2: return 6;

		// A
		// AA
		case 3: return 2;

		// AA
		// A
		case 4: return 4;

		// AA
		//  A
		case 5: return 5;

		//  A
		// AA
		case 6: return 3;

		// Glass / Border
		case 11: return 9;
	}

	return 0;
}

uint8_t parseDrizzleFlag(uint8_t flag) {
	switch (flag) {
		case DATA_VERTICAL_POLE: return 2;
		case DATA_HORIZONTAL_POLE: return 1;
		case DATA_WORMGRASS: return 20;
		case DATA_SHORTCUT: return 5;
		case DATA_ROOM_EXIT: return 6;
		case DATA_SPEAR: return 10;
		case DATA_ROCK: return 9;
	}

	return 0;
}

std::string parseDrizzleData(Grid *layer, unsigned int x, unsigned int y) {
	std::string output = "[";

	bool comma = false;
	for (const uint8_t &flag : layer->getData(x, y)) {
		if (comma) output += ",";

		output += std::to_string(parseDrizzleFlag(flag));
		comma = true;
	}

	if (layer->hasData(x, y, DATA_SHORTCUT) && getShortcutTile(layer, x, y) <= 3) {
		if (comma) output += ",";

		output += "4";
	}

	return output + "]";
}

void Project::exportDrizzle() {
	std::ofstream file;
	file.open(OUTPUT_PATH + name + ".txt");

	// Level Data
	file << "[";
	for (unsigned int x = 0; x < width; x++) {
		if (x != 0) file << ",";

		file << "[";
		for (unsigned int y = 0; y < height; y++) {
			if (y != 0) file << ",";

			file << "[";

			unsigned int tile1 = parseDrizzleTile(layer1, x, y);
			unsigned int tile2 = parseDrizzleTile(layer2, x, y);
			unsigned int tile3 = parseDrizzleTile(layer3, x, y);

			file << "[" << tile1 << "," << parseDrizzleData(layer1, x, y) << "],";
			file << "[" << tile2 << "," << parseDrizzleData(layer2, x, y) << "],";
			file << "[" << tile3 << "," << parseDrizzleData(layer3, x, y) << "]";

			file << "]";
		}
		file << "]";
	}
	file << "]\n";

	// Tile Data
	file << "[#lastKeys:[#L:0,#m1:0,#m2:0,#w:0,#a:0,#s:0,#d:0,#c:0,#q:0],#Keys:[#L:0,#m1:0,#m2:0,#w:0,#a:0,#s:0,#d:0,#c:0,#q:0],#workLayer:1,#lstMsPs:point(1,1),";
	file << "#tlMatrix:[";
	for (unsigned int x = 0; x < width; x++) {
		if (x != 0) file << ",";

		file << "[";
		for (unsigned int y = 0; y < height; y++) {
			if (y != 0) file << ",";

			file << "[";
			file << "[#tp:\"default\",#Data:0],";
			file << "[#tp:\"default\",#Data:0],";
			file << "[#tp:\"default\",#Data:0]";
			file << "]";
		}
		file << "]";
	}
	file << "],";
	file << "#defaultMaterial: \"Steel\",#toolType: \"material\",#toolData: \"Steel\",#tmPos: point(1, 1),#tmSavPosL: [1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1],#specialEdit: 0]\n";

	file << "[#lastKeys: [#n: 0, #m1: 0, #m2: 0, #w: 0, #a: 0, #s: 0, #d: 0, #e: 0, #r: 0, #f: 0], #Keys: [#n: 0, #m1: 0, #m2: 0, #w: 0, #a: 0, #s: 0, #d: 0, #e: 0, #r: 0, #f: 0], #lstMsPs: point(40, 12), #effects: [], #emPos: point(1, 1), #editEffect: 0, #selectEditEffect: 1, #mode: \"chooseEffect\", #brushSize: 5]\n[#pos: point(520, 400), #rot: 0, #sz: point(50, 70), #col: 1, #Keys: 0, #lastKeys: 0, #lastTm: 0, #lightAngle: 180, #flatness: 1, #lightRect: rect(1000, 1000, -1000, -1000), #paintShape: \"pxl\"]\n[#timeLimit: 4800, #defaultTerrain: 1, #maxFlies: 10, #flySpawnRate: 50, #lizards: [], #ambientSounds: [], #music: \"NONE\", #tags: [], #lightType: \"Static\", #waterDrips: 1, #lightRect: rect(0, 0, 1040, 800), #Matrix: []]\n[#mouse: 1, #lastMouse: 1, #mouseClick: 0, #pal: 1, #pals: [[#detCol: color( 255, 0, 0 )]], #eCol1: 1, #eCol2: 2, #totEcols: 5, #tileSeed: 57, #colGlows: [0, 0], #size: point(72, 43), #extraTiles: [0, 0, 0, 0], #light: 1]\n[#cameras: [point(25.2709, 32.2396)], #selectedCamera: 0, #quads: [[[0, 0], [0, 0], [0, 0], [0, 0]]], #Keys: [#n: 0, #d: 0, #e: 0, #p: 1], #lastKeys: [#n: 0, #d: 0, #e: 0, #p: 1]]\n[#waterLevel: -1, #waterInFront: 0, #waveLength: 60, #waveAmplitude: 5, #waveSpeed: 10]\n[#props: [], #lastKeys: [], #Keys: [], #workLayer: 1, #lstMsPs: point(0, 0), #pmPos: point(1, 1), #pmSavPosL: [], #propRotation: 0, #propStretchX: 1, #propStretchY: 1, #propFlipX: 1, #propFlipY: 1, #depth: 0, #color: 0]\n";

	file.close();
}