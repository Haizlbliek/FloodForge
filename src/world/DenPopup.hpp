#pragma once

#include <unordered_map>
#include <filesystem>
#include <vector>
#include <iostream>

#include "../gl.h"

enum class SliderType {
	SLIDER_INT,
	SLIDER_FLOAT
};

#include "../popup/Popups.hpp"
#include "Room.hpp"
#include "Globals.hpp"
#include "CreatureTextures.hpp"

class DenPopup : public Popup {
	public:
		DenPopup(Window *window, Den &den);

		void draw(double mouseX, double mouseY, bool mouseInside, Vector2 screenBounds);

		void mouseClick(double mouseX, double mouseY);

		void accept();

		void close();

		static void scrollCallback(void *object, double deltaX, double deltaY);

		std::string PopupName() { return "DenPopup"; }

		bool canStack(std::string popupName) { return popupName == "ConditionalPopup"; }

	private:
		double scrollA;
		double scrollATo;
		double scrollB;
		double scrollBTo;
		double scrollL;
		double scrollLTo;
		int scrollLMax;

		double sliderMin = 0.0;
		double sliderMax = 1.0;
		SliderType sliderType = SliderType::SLIDER_FLOAT;

		Den &den;

		bool hasSlider;
		int mouseSection;
		bool mouseClickSlider;
		int selectedCreature;
		DenCreature *selectedLineage;
		DenCreature *selectedLineageChance;
		double lineageSidebarWidth;

		void clampScroll();

		void ensureFlag(DenCreature &creature);

		void submitChance();

		static void keyCallback(void *object, int action, int key);
};