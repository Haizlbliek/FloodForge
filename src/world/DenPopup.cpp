#include "DenPopup.hpp"

#include <algorithm>

#include "../Theme.hpp"
#include "../font/Fonts.hpp"
#include "../Settings.hpp"

DenPopup::DenPopup(Window *window, Room *room, int den) : Popup(window) {
	bounds = Rect(-0.35, -0.35, 0.375 + 0.1, 0.35);
	scrollA = 0.0;
	scrollATo = 0.0;
	scrollB = 0.0;
	scrollBTo = 0.0;

	window->addScrollCallback(this, scrollCallback);

	this->room = room;
	this->den = den;

	ensureFlag();
}

void DenPopup::draw(double mouseX, double mouseY, bool mouseInside, Vector2 screenBounds) {
	bool hasHover = false;
	std::string hoverText = "";

	mouseOnRight = mouseX > (bounds.x1 - 0.2);
	
	Den &den = room->CreatureDen(this->den);
	bool unknown = !CreatureTextures::known(den.type);

	bool hasSlider = den.tag == "MEAN" || den.tag == "SEED" || den.tag == "LENGTH" || den.tag == "RotType";
	double sliderAt = den.data;

	if (hasSlider) {
		bounds.x1 = bounds.x0 + 0.8 + 0.1;
	} else {
		bounds.x1 = bounds.x0 + 0.8;
	}

	Popup::draw(mouseX, mouseY, mouseInside, screenBounds);
	
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

	if (hasSlider) {
		setThemeColour(ThemeColour::Border);
		Draw::begin(Draw::LINES);
		Draw::vertex(bounds.x0 + 0.85, bounds.y0 + 0.05);
		Draw::vertex(bounds.x0 + 0.85, bounds.y1 - 0.1);
		Draw::end();

		double progress = (sliderAt - sliderMin) / (sliderMax - sliderMin);
		double sliderY = ((bounds.y1 - bounds.y0 - 0.2) * progress) + bounds.y0 + 0.075;
		fillRect(bounds.x0 + 0.825, sliderY - 0.005, bounds.x0 + 0.875, sliderY + 0.005);
	}

	
	scrollA += (scrollATo - scrollA) * Settings::getSetting<double>(Settings::Setting::PopupScrollSpeed);
	scrollB += (scrollBTo - scrollB) * Settings::getSetting<double>(Settings::Setting::PopupScrollSpeed);
	
	double centreX = bounds.x0 + 0.305;
	double width = 0.5;
	double height = 0.5;

	double buttonSize = std::min(width / 7.0, height / 7.0);
	double buttonPadding = 0.02;

	setThemeColour(ThemeColour::Text);
	glLineWidth(1);
	Fonts::rainworld->writeCentred("Creature type:", centreX, bounds.y1 - 0.07, 0.035, CENTRE_X);
	Fonts::rainworld->writeCentred("Tag:", bounds.x0 + 0.7, bounds.y1 - 0.07, 0.035, CENTRE_X);

	double countX = 0.0;
	double countY = 0.0;


	int windowWidth;
	int windowHeight;
	glfwGetWindowSize(window->getGLFWWindow(), &windowWidth, &windowHeight);

	glEnable(GL_SCISSOR_TEST);
	double clipBottom = ((bounds.y0 + buttonPadding + screenBounds.y) * 0.5) * windowHeight;
	double clipTop = ((bounds.y1 - 0.1 - buttonPadding + screenBounds.y) * 0.5) * windowHeight;
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
			double rectY = (bounds.y1 - 0.1 - buttonPadding * 0.5) - (y + 1) * (buttonSize + buttonPadding) - scrollA;

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

			if (rect.inside(mouseX, mouseY)) {
				hasHover = true;
				hoverText = "Creature - " + creatureType;
			}

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


	double tagCentreX = bounds.x0 + 0.7;
	int countB = CreatureTextures::creatureTags.size();

	for (int y = 0; y <= (countB / 2); y++) {
		for (int x = 0; x < 2; x++) {
			int id = x + y * 2;

			if (id >= countB) break;

			std::string creatureTag = CreatureTextures::creatureTags[id];
			
			bool isSelected = den.tag == creatureTag;

			double rectX = tagCentreX + (x - 1.0) * (buttonSize + buttonPadding) + buttonPadding * 0.5;
			double rectY = (bounds.y1 - 0.1 - buttonPadding * 0.5) - (y + 1) * (buttonSize + buttonPadding) - scrollB;

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
			
			bool inside = rect.inside(mouseX, mouseY);

			if (inside) {
				hasHover = true;
				hoverText = "Tag - " + creatureTag;
			}

			if (inside || isSelected) {
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
		if (mouseX >= bounds.x0 + 0.825 && mouseX <= bounds.x0 + 0.875) {
			if (mouseY >= bounds.y0 + 0.05 && mouseY <= bounds.y1 - 0.1) {
				double P = (mouseY - bounds.y0 - 0.075) / (bounds.y1 - bounds.y0 - 0.2);
				P = std::clamp(P, 0.0, 1.0);
				den.data = P * (sliderMax - sliderMin) + sliderMin;
				if (sliderType == SliderType::SLIDER_INT) {
					den.data = round(den.data);
				}
			}
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


void DenPopup::mouseClick(double mouseX, double mouseY) {
	Popup::mouseClick(mouseX, mouseY);

	Den &den = room->CreatureDen(this->den);
	
	double centreX = bounds.x0 + 0.305;
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
			double rectY = (bounds.y1 - 0.1 - buttonPadding * 0.5) - (y + 1) * (buttonSize + buttonPadding) - scrollA;

			Rect rect = Rect(rectX, rectY, rectX + buttonSize, rectY + buttonSize);

			if (rect.inside(mouseX, mouseY)) {
				std::string type = CreatureTextures::creatures[id];
				if (type == "CLEAR") {
					den.type = "";
					den.count = 0;
				} else {
					if (den.type == type || type == "UNKNOWN") {
						if (window->modifierPressed(GLFW_MOD_SHIFT)) {
							den.count -= 1;
							if (den.count <= 0) {
								den.type = "";
								den.count = 0;
							}
						} else {
							den.count += 1;
						}
					} else {
						den.type = type;
						den.count = 1;
					}
				}
				ensureFlag();
			}
		}
	}


	double tagCentreX = bounds.x0 + 0.7;
	int countB = CreatureTextures::creatureTags.size();

	for (int y = 0; y <= (countB / 2); y++) {
		for (int x = 0; x < 2; x++) {
			int id = x + y * 2;

			if (id >= countB) break;

			double rectX = tagCentreX + (x - 1.0) * (buttonSize + buttonPadding) + buttonPadding * 0.5;
			double rectY = (bounds.y1 - 0.1 - buttonPadding * 0.5) - (y + 1) * (buttonSize + buttonPadding) - scrollB;

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

void DenPopup::accept() {}

void DenPopup::reject() {
	close();
}

void DenPopup::close() {
	Popups::removePopup(this);
	
	window->removeScrollCallback(this, scrollCallback);
}

void DenPopup::scrollCallback(void *object, double deltaX, double deltaY) {
	DenPopup *popup = static_cast<DenPopup*>(object);

	if (!popup->hovered) return;

	if (popup->mouseOnRight) {
		popup->scrollBTo += deltaY * 0.06;
	} else {
		popup->scrollATo += deltaY * 0.06;
	}
	
	popup->clampScroll();
}


void DenPopup::clampScroll() {
	double width = 0.5;
	double height = 0.5;

	double buttonSize = std::min(width / 7.0, height / 7.0);
	double buttonPadding = 0.02;

	int itemsA = CreatureTextures::creatures.size() / CREATURE_ROWS - 1;
	double sizeA = itemsA * (buttonSize + buttonPadding);

	if (scrollATo < -sizeA) {
		scrollATo = -sizeA;
		if (scrollA <= -sizeA + 0.06) {
			scrollA = -sizeA - 0.03;
		}
	}

	if (scrollATo > 0) {
		scrollATo = 0;
		if (scrollA >= -0.06) {
			scrollA = 0.03;
		}
	}
	
	int itemsB = CreatureTextures::creatureTags.size() / 2;
	double sizeB = itemsB * (buttonSize + buttonPadding);

	if (scrollBTo < -sizeB) {
		scrollBTo = -sizeB;
		if (scrollB <= -sizeB + 0.06) {
			scrollB = -sizeB - 0.03;
		}
	}

	if (scrollBTo > 0) {
		scrollBTo = 0;
		if (scrollB >= -0.06) {
			scrollB = 0.03;
		}
	}
}

void DenPopup::ensureFlag() {
	Den &den = room->CreatureDen(this->den);

	bool isNotLizard =
		den.type != "BlackLizard" &&
		den.type != "BlueLizard" &&
		den.type != "CyanLizard" &&
		den.type != "GreenLizard" &&
		den.type != "PinkLizard" &&
		den.type != "RedLizard" &&
		den.type != "WhiteLizard" &&
		den.type != "YellowLizard" &&
		den.type != "Salamander" &&
		den.type != "EelLizard" &&
		den.type != "SpitLizard" &&
		den.type != "TrainLizard" &&
		den.type != "ZoopLizard" &&
		den.type != "BasiliskLizard" &&
		den.type != "BlizzardLizard" &&
		den.type != "IndigoLizard";

	if (den.tag == "MEAN") {
		if (isNotLizard) {
			den.tag = "";
		}
	}

	if (den.tag == "LENGTH") {
		if (den.type != "PoleMimic" && den.type != "Centipede") {
			den.tag = "";
		}
	}

	if (den.tag == "Winter") {
		if (den.type != "BigSpider" && den.type != "SpitterSpider" && den.type != "Yeek" && isNotLizard) {
			den.tag = "";
		}
	}

	if (den.tag == "Voidsea") {
		if (den.type != "RedLizard" && den.type != "RedCentipede" && den.type != "BigSpider" && den.type != "DaddyLongLegs" && den.type != "BrotherLongLegs" && den.type != "TerrorLongLegs" && den.type != "BigEel" && den.type != "CyanLizard") {
			den.tag = "";
		}
	}

	if (den.tag != "MEAN" && den.tag != "LENGTH" && den.tag != "SEED") den.data = 0.0;

	sliderType = SliderType::SLIDER_FLOAT;
	if (den.tag == "MEAN") {
		sliderMin = -1.0;
		sliderMax = 1.0;
	} else if (den.tag == "LENGTH") {
		if (den.type == "Centipede") {
			sliderMin = 0.1;
			sliderMax = 1.0;
		} else {
			sliderMin = 1;
			sliderMax = 32;
		}
	} else if (den.tag == "SEED") {
		sliderMin = 0;
		sliderMax = 65536;
		sliderType = SliderType::SLIDER_INT;
	} else if (den.tag == "RotType") {
		if (isNotLizard) {
			den.tag = "";
		} else {
			sliderMin = 0;
			sliderMax = 3;
		}
		sliderType = SliderType::SLIDER_INT;
	}
}