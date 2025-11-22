#pragma once

#include <unordered_map>
#include <filesystem>
#include <vector>
#include <iostream>

#include "../../gl.h"

enum class SliderType {
	SLIDER_INT,
	SLIDER_FLOAT
};

#include "../../popup/Popups.hpp"
#include "../Room.hpp"
#include "../Globals.hpp"
#include "../CreatureTextures.hpp"

class DenPopup : public Popup {
	public:
		DenPopup(Den &den);

		void draw() override;

		void accept() override;

		void close() override;

		static void scrollCallback(void *object, double deltaX, double deltaY);

		std::string PopupName() { return "DenPopup"; }

		bool canStack(std::string popupName) { return popupName == "ConditionalPopup"; }

	private:
		double scrollCreatures = 0.0;
		double scrollCreaturesTo = 0.0;
		double scrollTags = 0.0;
		double scrollTagsTo = 0.0;
		double scrollLineages = 0.0;
		double scrollLinagesTo = 0.0;
		int scrollLineagesMax = 0;

		double sliderMin = 0.0;
		double sliderMax = 1.0;
		SliderType sliderType = SliderType::SLIDER_FLOAT;

		Den &den;

		bool hasSlider;
		int mouseSection;
		bool lastMouseClickSlider;
		bool mouseClickSlider;
		int selectedCreature;
		DenCreature *selectedLineage;
		double editingLineageChance;
		DenCreature *selectedLineageChance;
		double lineageSidebarWidth;

		void clampScroll();

		void fixSlider();

		void submitChance();

		static void keyCallback(void *object, int action, int key);
};