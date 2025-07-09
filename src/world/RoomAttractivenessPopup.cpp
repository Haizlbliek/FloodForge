#include "RoomAttractivenessPopup.hpp"

const RoomAttractiveness RoomAttractivenessPopup::attractivenessIds   [6] = { RoomAttractiveness::DEFAULT, RoomAttractiveness::NEUTRAL, RoomAttractiveness::FORBIDDEN, RoomAttractiveness::AVOID, RoomAttractiveness::LIKE, RoomAttractiveness::STAY };
const Colour             RoomAttractivenessPopup::attractivenessColors[6] = { Colour(0.5, 0.5, 0.5),       Colour(1.0, 1.0, 1.0),       Colour(1.0, 0.0, 0.0),         Colour(1.0, 1.0, 0.0),     Colour(0.0, 1.0, 0.0),    Colour(0.0, 1., 1.0)     };
const std::string        RoomAttractivenessPopup::attractivenessNames [6] = { "DEFAULT",                   "NEUTRAL",                   "FORBIDDEN",                   "AVOID",                   "LIKE",                   "STAY"                   };

RoomAttractivenessPopup::RoomAttractivenessPopup(Window *window, std::set<Room *> rooms) : rooms(rooms), Popup(window) {
	bounds = Rect(-0.35, -0.35, 0.375 + 0.1, 0.35);
	currentScroll = 0.0;
	targetScroll = 0.0;

	window->addScrollCallback(this, scrollCallback);
}

RoomAttractivenessPopup::~RoomAttractivenessPopup() {
}

void RoomAttractivenessPopup::close() {
	Popup::close();

	window->removeScrollCallback(this, scrollCallback);
}

void RoomAttractivenessPopup::draw(double mouseX, double mouseY, bool mouseInside, Vector2 screenBounds) {
	bool hasHover = false;
	std::string hoverText = "";

	Popup::draw(mouseX, mouseY, mouseInside, screenBounds);

	if (minimized) return;

	if (hovered) {
		setThemeColour(ThemeColour::BorderHighlight);
	} else {
		setThemeColour(ThemeColour::Border);
	}
	Draw::begin(Draw::LINES);
	Draw::vertex(bounds.X0() + 0.6, bounds.Y0());
	Draw::vertex(bounds.X0() + 0.6, bounds.Y1());
	Draw::end();

	currentScroll += (targetScroll - currentScroll) * Settings::getSetting<double>(Settings::Setting::PopupScrollSpeed);

	double centreX = bounds.X0() + 0.305;

	double buttonSize = 0.5 / 7.0;
	double buttonPadding = 0.02;

	setThemeColour(ThemeColour::Text);
	glLineWidth(1);
	Fonts::rainworld->writeCentred("Creature type:", centreX, bounds.Y1() - 0.07, 0.035, CENTRE_X);
	Fonts::rainworld->writeCentred("Attract:", bounds.X0() + 0.72, bounds.Y1() - 0.07, 0.035, CENTRE_X);

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
	countA -= 2;
	
	Room *room = *rooms.begin();

	for (int y = 0; y <= (countA / CREATURE_ROWS); y++) {
		for (int x = 0; x < CREATURE_ROWS; x++) {
			int id = x + y * CREATURE_ROWS + 1;

			if (id >= countA) break;

			std::string creatureType = CreatureTextures::creatures[id];

			double rectX = centreX + (x - 0.5 * CREATURE_ROWS) * (buttonSize + buttonPadding) + buttonPadding * 0.5;
			double rectY = (bounds.Y1() - 0.1 - buttonPadding * 0.5) - (y + 1) * (buttonSize + buttonPadding) - currentScroll;

			Rect rect = Rect(rectX, rectY, rectX + buttonSize, rectY + buttonSize);

			setThemeColour(ThemeColour::Button);
			fillRect(rectX, rectY, rectX + buttonSize, rectY + buttonSize);

			GLuint texture = CreatureTextures::getTexture(CreatureTextures::creatures[id]);

			Draw::color(1.0f, 1.0f, 1.0f);
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

			if (rect.inside(mouseX, mouseY)) {
				setThemeColour(ThemeColour::BorderHighlight);
				hasHover = true;
				hoverText = creatureType;
			} else {
				setThemeColour(ThemeColour::Border);
			}
			strokeRect(rectX, rectY, rectX + buttonSize, rectY + buttonSize);

			std::unordered_map<std::string, RoomAttractiveness>::iterator index = room->data.attractiveness.find(creatureType);
			if (index == room->data.attractiveness.end()) {
				Draw::color(attractivenessColors[0]);
			} else {
				Draw::color(attractivenessColors[index->second]);
			}
			fillRect(rectX - 0.005, rectY - 0.005, rectX + 0.015, rectY + 0.015);
		}
	}

	glDisable(GL_SCISSOR_TEST);

	for (int i = 0; i < 6; i++) {
		double y = bounds.Y1() - 0.165 - i * 0.09;

		Draw::color(attractivenessColors[i]);
		Fonts::rainworld->writeCentred(attractivenessNames[i], bounds.X1() - 0.11, y, 0.03, CENTRE_XY);

		if (selectAttractiveness == attractivenessIds[i]) {
			strokeRect(bounds.X0() + 0.605, y - 0.02, bounds.X1() - 0.01, y + 0.02);
		}
	}

	// Hovers

	if (hasHover && mouseInside) {
		double width = Fonts::rainworld->getTextWidth(hoverText, 0.04) + 0.02;
		setThemeColour(ThemeColour::Popup);
		fillRect(mouseX, mouseY, mouseX + width, mouseY + 0.06);
		setThemeColour(ThemeColour::Border);
		strokeRect(mouseX, mouseY, mouseX + width, mouseY + 0.06);
		setThemeColour(ThemeColour::Text);
		Fonts::rainworld->writeCentred(hoverText, mouseX + 0.01, mouseY + 0.03, 0.04, CENTRE_Y);
	}
}


void RoomAttractivenessPopup::mouseClick(double mouseX, double mouseY) {
	Popup::mouseClick(mouseX, mouseY);

	double centreX = bounds.X0() + 0.305;
	double width = 0.5;
	double height = 0.5;

	double buttonSize = std::min(width / 7.0, height / 7.0);
	double buttonPadding = 0.02;

	int countA = CreatureTextures::creatures.size();
	countA -= 2;

	for (int y = 0; y <= (countA / CREATURE_ROWS); y++) {
		for (int x = 0; x < CREATURE_ROWS; x++) {
			int id = x + y * CREATURE_ROWS + 1;

			if (id >= countA) break;

			double rectX = centreX + (x - 0.5 * CREATURE_ROWS) * (buttonSize + buttonPadding) + buttonPadding * 0.5;
			double rectY = (bounds.Y1() - 0.1 - buttonPadding * 0.5) - (y + 1) * (buttonSize + buttonPadding) - currentScroll;

			Rect rect = Rect(rectX, rectY, rectX + buttonSize, rectY + buttonSize);

			if (rect.inside(mouseX, mouseY)) {
				std::string type = CreatureTextures::creatures[id];
				setAllTo(selectAttractiveness, type);
			}
		}
	}

	for (int i = 0; i < 6; i++) {
		double y = bounds.Y1() - 0.165 - i * 0.09;

		if (Rect(bounds.X0() + 0.605, y - 0.02, bounds.X1() - 0.01, y + 0.02).inside(mouseX, mouseY)) {
			selectAttractiveness = attractivenessIds[i];
		}
	}
}

void RoomAttractivenessPopup::scrollCallback(void *object, double deltaX, double deltaY) {
	RoomAttractivenessPopup *popup = static_cast<RoomAttractivenessPopup*>(object);

	if (!popup->hovered) return;

	popup->targetScroll += deltaY * 0.06;

	popup->clampScroll();
}

void RoomAttractivenessPopup::clampScroll() {
	double width = 0.5;
	double height = 0.5;

	double buttonSize = std::min(width / 7.0, height / 7.0);
	double buttonPadding = 0.02;

	int items = CreatureTextures::creatures.size() / CREATURE_ROWS - 1;
	double size = items * (buttonSize + buttonPadding);

	if (targetScroll < -size) {
		targetScroll = -size;
		if (currentScroll <= -size + 0.06) {
			currentScroll = -size - 0.03;
		}
	}

	if (targetScroll > 0) {
		targetScroll = 0;
		if (currentScroll >= -0.06) {
			currentScroll = 0.03;
		}
	}
}

void RoomAttractivenessPopup::setAllTo(RoomAttractiveness attr, std::string creature) {
	for (Room *room : rooms) {
		if (room->isOffscreen()) continue;

		if (attr == RoomAttractiveness::DEFAULT) {
			room->data.attractiveness.erase(creature);
		} else {
			room->data.attractiveness[creature] = attr;
		}
	}
}