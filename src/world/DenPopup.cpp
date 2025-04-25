#include "DenPopup.hpp"

#include <algorithm>

#include "../Theme.hpp"
#include "../font/Fonts.hpp"
#include "../Settings.hpp"

std::unordered_map<std::string, GLuint> CreatureTextures::creatureTextures;
std::unordered_map<std::string, GLuint> CreatureTextures::creatureTagTextures;
std::vector<std::string> CreatureTextures::creatures;
std::vector<std::string> CreatureTextures::creatureTags;
std::unordered_map<std::string, std::string> CreatureTextures::parseMap;
GLuint CreatureTextures::UNKNOWN = 0;

bool validExtension(std::string extension) {
	return extension == ".png";
}

void CreatureTextures::loadCreaturesFromFolder(std::string path, bool include) {
	loadCreaturesFromFolder(path, "", include);
}

void CreatureTextures::loadCreaturesFromFolder(std::string path, std::string prefix, bool include) {
	for (const auto& entry : std::filesystem::directory_iterator(path)) {
		if (std::filesystem::is_regular_file(entry.path()) && validExtension(entry.path().extension().string())) {
			std::string creature = prefix + entry.path().stem().string();
			if (include) creatures.push_back(creature);
			creatureTextures[creature] = loadTexture(entry.path().string());
		}
	}
}

void CreatureTextures::init() {
	loadCreaturesFromFolder("assets/creatures/", true);
	loadCreaturesFromFolder("assets/creatures/downpour/", true);
	loadCreaturesFromFolder("assets/creatures/watcher/", true);
	loadCreaturesFromFolder("assets/creatures/room/", "room-", false);
	
	for (const auto& entry : std::filesystem::directory_iterator("assets/creatures/TAGS/")) {
		if (std::filesystem::is_regular_file(entry.path()) && validExtension(entry.path().extension().string())) {
			std::string tag = entry.path().stem().string();
			creatureTags.push_back(tag);
			creatureTagTextures[tag] = loadTexture(entry.path().string());
		}
	}

	auto CLEAR_it = std::find(creatures.begin(), creatures.end(), "CLEAR");
	if (CLEAR_it != creatures.end()) {
		std::swap(*CLEAR_it, *(creatures.begin()));
	}

	auto UNKNOWN_it = std::find(creatures.begin(), creatures.end(), "UNKNOWN");
	UNKNOWN = creatureTextures["UNKNOWN"];
	if (UNKNOWN_it != creatures.end()) {
		std::swap(*UNKNOWN_it, *(creatures.end() - 1));
	}

	std::fstream parseFile("assets/creatures/parse.txt");
	if (!parseFile.is_open()) return;

	std::string line;
	while (std::getline(parseFile, line)) {
		std::string from = line.substr(0, line.find_first_of(">"));
		std::string to = line.substr(line.find_first_of(">") + 1);

		parseMap[from] = to;
	}
}

GLuint CreatureTextures::getTexture(std::string type) {
	if (type == "") return 0;

	if (creatureTagTextures.find(type) != creatureTagTextures.end()) {
		return creatureTagTextures[type];
	}

	if (creatureTextures.find(type) == creatureTextures.end()) {
		return creatureTextures["UNKNOWN"];
	}

	return creatureTextures[type];
}

std::string CreatureTextures::parse(std::string originalName) {
	if (parseMap.find(originalName) == parseMap.end()) {
		return originalName;
	}

	return parseMap[originalName];
}

bool CreatureTextures::known(std::string type) {
	if (type == "") return true;

	return creatureTextures.find(parse(type)) != creatureTextures.end();
}

void DenPopup::close() {
	Popups::removePopup(this);
	
	window->removeScrollCallback(this, scrollCallback);
}

void DenPopup::draw(double mouseX, double mouseY, bool mouseInside, Vector2 screenBounds) {
	mouseOnRight = mouseX > (bounds.X1() - 0.2);
	
	Den &den = room->CreatureDen(this->den);
	bool unknown = !CreatureTextures::known(den.type);

	bool hasSlider = den.tag == "MEAN" || den.tag == "SEED" || den.tag == "LENGTH" || den.tag == "RotType";
	double sliderAt = den.data;

	if (hasSlider) {
		bounds.X1(bounds.X0() + 0.8 + 0.1);
	} else {
		bounds.X1(bounds.X0() + 0.8);
	}

	Popup::draw(mouseX, mouseY, mouseInside, screenBounds);
	
	if (hovered) {
		setThemeColour(ThemeColour::BorderHighlight);
	} else {
		setThemeColour(ThemeColour::Border);
	}
	Draw::begin(Draw::LINES);
	Draw::vertex(bounds.X0() + 0.6, bounds.Y0());
	Draw::vertex(bounds.X0() + 0.6, bounds.Y1());
	Draw::end();

	if (hasSlider) {
		setThemeColour(ThemeColour::Border);
		Draw::begin(Draw::LINES);
		Draw::vertex(bounds.X0() + 0.85, bounds.Y0() + 0.05);
		Draw::vertex(bounds.X0() + 0.85, bounds.Y1() - 0.1);
		Draw::end();

		double progress = (sliderAt - sliderMin) / (sliderMax - sliderMin);
		double sliderY = ((bounds.Y1() - bounds.Y0() - 0.2) * progress) + bounds.Y0() + 0.075;
		fillRect(bounds.X0() + 0.825, sliderY - 0.005, bounds.X0() + 0.875, sliderY + 0.005);
	}

	
	scrollA += (scrollATo - scrollA) * Settings::getSetting<double>(Settings::Setting::PopupScrollSpeed);
	scrollB += (scrollBTo - scrollB) * Settings::getSetting<double>(Settings::Setting::PopupScrollSpeed);
	
	double centreX = bounds.X0() + 0.305;
	double width = 0.5;
	double height = 0.5;

	double buttonSize = std::min(width / 7.0, height / 7.0);
	double buttonPadding = 0.02;

	setThemeColour(ThemeColour::Text);
	glLineWidth(1);
	Fonts::rainworld->writeCentred("Creature type:", centreX, bounds.Y1() - 0.07, 0.035, CENTRE_X);
	Fonts::rainworld->writeCentred("Tag:", bounds.X0() + 0.7, bounds.Y1() - 0.07, 0.035, CENTRE_X);

	double countX = 0.0;
	double countY = 0.0;


	int windowWidth;
	int windowHeight;
	glfwGetWindowSize(window->getGLFWWindow(), &windowWidth, &windowHeight);

	glEnable(GL_SCISSOR_TEST);
	double clipBottom = ((bounds.Y0() + buttonPadding + screenBounds.y) * 0.5) * windowHeight;
	double clipTop = ((bounds.Y1() - 0.1 - buttonPadding + screenBounds.y) * 0.5) * windowHeight;
	glScissor(0, clipBottom, windowWidth, clipTop - clipBottom);

	int countA = CreatureTextures::creatures.size();
	if (!unknown) countA--;

	for (int y = 0; y <= (countA / CREATURE_ROWS); y++) {
		for (int x = 0; x < CREATURE_ROWS; x++) {
			int id = x + y * CREATURE_ROWS;

			if (id >= countA) break;

			std::string creatureType = CreatureTextures::creatures[id];
			
			bool isSelected = den.type == creatureType || (unknown && creatureType == "UNKNOWN");

			double rectX = centreX + (x - 0.5 * CREATURE_ROWS) * (buttonSize + buttonPadding) + buttonPadding * 0.5;
			double rectY = (bounds.Y1() - 0.1 - buttonPadding * 0.5) - (y + 1) * (buttonSize + buttonPadding) - scrollA;

			Rect rect = Rect(rectX, rectY, rectX + buttonSize, rectY + buttonSize);

			setThemeColour(ThemeColour::Button);
			fillRect(rectX, rectY, rectX + buttonSize, rectY + buttonSize);

			GLuint texture = CreatureTextures::getTexture(CreatureTextures::creatures[id]);

			if (isSelected) {
				Draw::color(1.0, 1.0, 1.0);
			} else {
				Draw::color(0.5, 0.5, 0.5);
			}
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
			Draw::texCoord(0.5 - uvx, 0.5 + uvy); Draw::vertex(rectX, rectY);
			Draw::texCoord(0.5 + uvx, 0.5 + uvy); Draw::vertex(rectX + buttonSize, rectY);
			Draw::texCoord(0.5 + uvx, 0.5 - uvy); Draw::vertex(rectX + buttonSize, rectY + buttonSize);
			Draw::texCoord(0.5 - uvx, 0.5 - uvy); Draw::vertex(rectX, rectY + buttonSize);
			Draw::end();
			Draw::useTexture(0);
			glDisable(GL_BLEND);

			if (rect.inside(mouseX, mouseY) || isSelected) {
				setThemeColour(ThemeColour::BorderHighlight);
			} else {
				setThemeColour(ThemeColour::Border);
			}
			strokeRect(rectX, rectY, rectX + buttonSize, rectY + buttonSize);

			setThemeColour(ThemeColour::Text);
			if (isSelected) {
				countX = rectX + buttonSize;
				countY = rectY;
			}
		}
	}
	
	if (den.type != "") {
		Fonts::rainworld->writeCentred(std::to_string(den.count), countX, countY, 0.04, CENTRE_XY);
	}


	double tagCentreX = bounds.X0() + 0.7;
	int countB = CreatureTextures::creatureTags.size();

	for (int y = 0; y <= (countB / 2); y++) {
		for (int x = 0; x < 2; x++) {
			int id = x + y * 2;

			if (id >= countB) break;

			std::string creatureTag = CreatureTextures::creatureTags[id];
			
			bool isSelected = den.tag == creatureTag;

			double rectX = tagCentreX + (x - 1.0) * (buttonSize + buttonPadding) + buttonPadding * 0.5;
			double rectY = (bounds.Y1() - 0.1 - buttonPadding * 0.5) - (y + 1) * (buttonSize + buttonPadding) - scrollB;

			Rect rect = Rect(rectX, rectY, rectX + buttonSize, rectY + buttonSize);

			setThemeColour(ThemeColour::Button);
			fillRect(rectX, rectY, rectX + buttonSize, rectY + buttonSize);

			GLuint texture = CreatureTextures::getTexture(CreatureTextures::creatureTags[id]);

			if (isSelected) {
				Draw::color(1.0, 1.0, 1.0);
			} else {
				Draw::color(0.5, 0.5, 0.5);
			}
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
			Draw::texCoord(0.5 - uvx, 0.5 + uvy); Draw::vertex(rectX, rectY);
			Draw::texCoord(0.5 + uvx, 0.5 + uvy); Draw::vertex(rectX + buttonSize, rectY);
			Draw::texCoord(0.5 + uvx, 0.5 - uvy); Draw::vertex(rectX + buttonSize, rectY + buttonSize);
			Draw::texCoord(0.5 - uvx, 0.5 - uvy); Draw::vertex(rectX, rectY + buttonSize);
			Draw::end();
			Draw::useTexture(0);
			glDisable(GL_BLEND);

			if (rect.inside(mouseX, mouseY) || isSelected) {
				setThemeColour(ThemeColour::BorderHighlight);
			} else {
				setThemeColour(ThemeColour::Border);
			}
			strokeRect(rectX, rectY, rectX + buttonSize, rectY + buttonSize);

			setThemeColour(ThemeColour::Text);
			if (isSelected) {
				countX = rectX + buttonSize;
				countY = rectY;
			}
		}
	}

	glDisable(GL_SCISSOR_TEST);

	// UPDATE

	if (hasSlider && window->GetMouse()->Left()) {
		if (mouseX >= bounds.X0() + 0.825 && mouseX <= bounds.X0() + 0.875) {
			if (mouseY >= bounds.Y0() + 0.05 && mouseY <= bounds.Y1() - 0.1) {
				double P = (mouseY - bounds.Y0() - 0.075) / (bounds.Y1() - bounds.Y0() - 0.2);
				P = std::clamp(P, 0.0, 1.0);
				den.data = P * (sliderMax - sliderMin) + sliderMin;
				if (sliderType == SliderType::SLIDER_INT) {
					den.data = round(den.data);
				}
			}
		}
	}
}


void DenPopup::mouseClick(double mouseX, double mouseY) {
	Popup::mouseClick(mouseX, mouseY);

	Den &den = room->CreatureDen(this->den);
	
	double centreX = bounds.X0() + 0.305;
	double width = 0.5;
	double height = 0.5;

	double buttonSize = std::min(width / 7.0, height / 7.0);
	double buttonPadding = 0.02;
	
	int countA = CreatureTextures::creatures.size();
	if (CreatureTextures::known(den.type)) countA--;
	for (int y = 0; y <= (countA / CREATURE_ROWS); y++) {
		for (int x = 0; x < CREATURE_ROWS; x++) {
			int id = x + y * CREATURE_ROWS;

			if (id >= countA) break;

			double rectX = centreX + (x - 0.5 * CREATURE_ROWS) * (buttonSize + buttonPadding) + buttonPadding * 0.5;
			double rectY = (bounds.Y1() - 0.1 - buttonPadding * 0.5) - (y + 1) * (buttonSize + buttonPadding) - scrollA;

			Rect rect = Rect(rectX, rectY, rectX + buttonSize, rectY + buttonSize);

			if (rect.inside(mouseX, mouseY)) {
				std::string type = CreatureTextures::creatures[id];
				if (type == "CLEAR") {
					den.type = "";
					den.count = 0;
				} else {
					if (den.type == type || type == "UNKNOWN") {
						den.count += 1;
					} else {
						den.type = type;
						den.count = 1;
					}
				}
				ensureFlag();
			}
		}
	}


	double tagCentreX = bounds.X0() + 0.7;
	int countB = CreatureTextures::creatureTags.size();

	for (int y = 0; y <= (countB / 2); y++) {
		for (int x = 0; x < 2; x++) {
			int id = x + y * 2;

			if (id >= countB) break;

			double rectX = tagCentreX + (x - 1.0) * (buttonSize + buttonPadding) + buttonPadding * 0.5;
			double rectY = (bounds.Y1() - 0.1 - buttonPadding * 0.5) - (y + 1) * (buttonSize + buttonPadding) - scrollB;

			Rect rect = Rect(rectX, rectY, rectX + buttonSize, rectY + buttonSize);

			if (rect.inside(mouseX, mouseY)) {
				std::string creatureTag = CreatureTextures::creatureTags[id];

				if (den.tag == creatureTag) {
					den.tag = "";
				} else {
					den.tag = creatureTag;
				}
				ensureFlag();
			}
		}
	}
}