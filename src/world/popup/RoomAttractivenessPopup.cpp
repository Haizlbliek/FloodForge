#include "RoomAttractivenessPopup.hpp"

#include "../../ui/UI.hpp"

const RoomAttractiveness RoomAttractivenessPopup::attractivenessIds   [6] = { RoomAttractiveness::DEFAULT, RoomAttractiveness::NEUTRAL, RoomAttractiveness::FORBIDDEN, RoomAttractiveness::AVOID, RoomAttractiveness::LIKE, RoomAttractiveness::STAY };
const Colour             RoomAttractivenessPopup::attractivenessColors[6] = { Colour(0.5, 0.5, 0.5),       Colour(1.0, 1.0, 1.0),       Colour(1.0, 0.0, 0.0),         Colour(1.0, 1.0, 0.0),     Colour(0.0, 1.0, 0.0),    Colour(0.0, 1., 1.0)     };
const std::string        RoomAttractivenessPopup::attractivenessNames [6] = { "DEFAULT",                   "NEUTRAL",                   "FORBIDDEN",                   "AVOID",                   "LIKE",                   "STAY"                   };

RoomAttractivenessPopup::RoomAttractivenessPopup(std::set<Room *> rooms) : rooms(rooms), Popup() {
	bounds = Rect(-0.35, -0.35, 0.375 + 0.1, 0.35);
	currentScroll = 0.0;
	targetScroll = 0.0;

	UI::window->addScrollCallback(this, scrollCallback);
}

RoomAttractivenessPopup::~RoomAttractivenessPopup() {
}

void RoomAttractivenessPopup::close() {
	Popup::close();

	UI::window->removeScrollCallback(this, scrollCallback);
}

void RoomAttractivenessPopup::draw() {
	std::string hoverText = "";

	Popup::draw();

	if (minimized) return;

	if (hovered) {
		setThemeColour(ThemeColour::BorderHighlight);
	} else {
		setThemeColour(ThemeColour::Border);
	}
	Draw::begin(Draw::LINES);
	Draw::vertex(bounds.x0 + 0.6, bounds.y0);
	Draw::vertex(bounds.x0 + 0.6, bounds.y1);
	Draw::end();

	currentScroll += (targetScroll - currentScroll) * Settings::getSetting<double>(Settings::Setting::PopupScrollSpeed);

	double centreX = bounds.x0 + 0.305;

	double buttonSize = 0.5 / 7.0;
	double buttonPadding = 0.02;

	setThemeColour(ThemeColour::Text);
	glLineWidth(1);
	Fonts::rainworld->writeCentered("Creature type:", centreX, bounds.y1 - 0.07, 0.035, CENTER_X);
	Fonts::rainworld->writeCentered("Attract:", bounds.x0 + 0.72, bounds.y1 - 0.07, 0.035, CENTER_X);

	double countX = 0.0;
	double countY = 0.0;

	glEnable(GL_SCISSOR_TEST);
	double clipBottom = ((bounds.y0 + buttonPadding + UI::screenBounds.y) * 0.5) * UI::window->Height();
	double clipTop = ((bounds.y1 - 0.1 - buttonPadding + UI::screenBounds.y) * 0.5) * UI::window->Height();
	glScissor(0, clipBottom, UI::window->Width(), clipTop - clipBottom);

	int countA = CreatureTextures::creatures.size();
	countA -= 2;
	
	Room *room = *rooms.begin();

	for (int y = 0; y <= (countA / CREATURE_ROWS); y++) {
		for (int x = 0; x < CREATURE_ROWS; x++) {
			int id = x + y * CREATURE_ROWS + 1;

			if (id >= countA) break;

			std::string creatureType = CreatureTextures::creatures[id];

			double rectX = centreX + (x - 0.5 * CREATURE_ROWS) * (buttonSize + buttonPadding) + buttonPadding * 0.5;
			double rectY = (bounds.y1 - 0.1 - buttonPadding * 0.5) - (y + 1) * (buttonSize + buttonPadding) - currentScroll;

			UVRect rect = UVRect::fromSize(rectX, rectY, buttonSize, buttonSize);

			GLuint texture = CreatureTextures::getTexture(CreatureTextures::creatures[id]);
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

			rect.uv(0.5 - uvx, 0.5 + uvy, 0.5 + uvx, 0.5 - uvy);
			UI::ButtonResponse response = UI::TextureButton(rect, UI::TextureButtonMods().TextureId(texture));
			if (response.hovered) {
				hoverText = creatureType;
			}
			if (response.clicked) {
				setAllTo(selectAttractiveness, creatureType);
			}

			std::unordered_map<std::string, RoomAttractiveness>::iterator index = room->data.attractiveness.find(creatureType);
			if (index == room->data.attractiveness.end()) {
				Draw::color(attractivenessColors[0]);
			} else {
				Draw::color(attractivenessColors[(int) index->second]);
			}
			fillRect(rectX - 0.005, rectY - 0.005, rectX + 0.015, rectY + 0.015);
		}
	}

	glDisable(GL_SCISSOR_TEST);

	for (int i = 0; i < 6; i++) {
		double y = bounds.y1 - 0.165 - i * 0.09;

		Rect rect(bounds.x0 + 0.605, y - 0.02, bounds.x1 - 0.01, y + 0.02);
		if (UI::TextButton(rect, attractivenessNames[i], UI::TextButtonMods().Selected(selectAttractiveness == attractivenessIds[i]))) {
			selectAttractiveness = attractivenessIds[i];
		}
	}

	// Hovers

	if (!hoverText.empty() && hovered) {
		double width = Fonts::rainworld->getTextWidth(hoverText, 0.04) + 0.02;
		Rect rect = Rect::fromSize(UI::mouse.x, UI::mouse.y, width, 0.06);
		setThemeColour(ThemeColour::Popup);
		fillRect(rect);
		setThemeColour(ThemeColour::Border);
		strokeRect(rect);
		setThemeColour(ThemeColour::Text);
		Fonts::rainworld->writeCentered(hoverText, UI::mouse.x + 0.01, UI::mouse.y + 0.03, 0.04, CENTER_Y);
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