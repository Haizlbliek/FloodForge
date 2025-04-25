#pragma once

#include <unordered_map>
#include <filesystem>
#include <vector>
#include <iostream>

#include "../gl.h"

#define CREATURE_ROWS 6

namespace CreatureTextures {
	extern std::unordered_map<std::string, GLuint> creatureTextures;
	extern std::unordered_map<std::string, GLuint> creatureTagTextures;
	extern std::vector<std::string> creatures;
	extern std::vector<std::string> creatureTags;
	extern std::unordered_map<std::string, std::string> parseMap;
	
	extern GLuint UNKNOWN;

	void loadCreaturesFromFolder(std::string path, bool include);
	void loadCreaturesFromFolder(std::string path, std::string prefix, bool include);

	GLuint getTexture(std::string type);

	void init();

	std::string parse(std::string originalName);

	bool known(std::string type);
};

enum SliderType {
	SLIDER_INT,
	SLIDER_FLOAT
};

#include "../popup/Popups.hpp"
#include "Room.hpp"
#include "Globals.hpp"

class DenPopup : public Popup {
	public:
		DenPopup(Window *window, Room *room, int den) : Popup(window) {
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

		void draw(double mouseX, double mouseY, bool mouseInside, Vector2 screenBounds);

		void mouseClick(double mouseX, double mouseY);

		void accept() {};

		void reject() {
			close();
		}

		void close();
		
		static void scrollCallback(void *object, double deltaX, double deltaY) {
			DenPopup *popup = static_cast<DenPopup*>(object);

			if (!popup->hovered) return;

			if (popup->mouseOnRight) {
				popup->scrollBTo += deltaY * 0.06;
			} else {
				popup->scrollATo += deltaY * 0.06;
			}
			
			popup->clampScroll();
		}

		std::string PopupName() { return "DenPopup"; }
	
	private:
		double scrollA;
		double scrollATo;
		double scrollB;
		double scrollBTo;
		
		double sliderMin = 0.0;
		double sliderMax = 1.0;
		SliderType sliderType = SliderType::SLIDER_FLOAT;

		Room *room;
		int den;

		bool mouseOnRight;
		

		void clampScroll() {
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

		void ensureFlag() {
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
};