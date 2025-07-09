#pragma once

#include <unordered_map>
#include <filesystem>
#include <vector>
#include <iostream>

#include "../gl.h"

enum SliderType {
	SLIDER_INT,
	SLIDER_FLOAT
};

#include "../popup/Popups.hpp"
#include "Room.hpp"
#include "Globals.hpp"
#include "CreatureTextures.hpp"

class DenPopup : public Popup {
	public:
		DenPopup(Window *window, Room *room, int den);

		void draw(double mouseX, double mouseY, bool mouseInside, Vector2 screenBounds);

		void mouseClick(double mouseX, double mouseY);

		void accept();

		void reject();

		void close();
		
		static void scrollCallback(void *object, double deltaX, double deltaY);

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
		

		void clampScroll();

		void ensureFlag();
};