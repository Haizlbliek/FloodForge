#pragma once

#include "../gl.h"

#include <iostream>
#include <algorithm>
#include <cctype>

#include "../Window.hpp"
#include "../Theme.hpp"

#include "../popup/Popups.hpp"

#include "Globals.hpp"
#include "Room.hpp"

class SubregionNewPopup : public Popup {
	public:
		SubregionNewPopup(Window *window, std::set<Room*> rooms, int editIndex = -1);

		void draw(double mouseX, double mouseY, bool mouseInside, Vector2 screenBounds);

		void mouseClick(double mouseX, double mouseY);

		void accept();

		void reject();

		void close();

		static void keyCallback(void *object, int action, int key);
		
		bool canStack(std::string popupName) { return false; }
		std::string PopupName() { return "SubregionNewPopup"; }

	private:
		std::string text;

		std::set<Room*> rooms;
		
		int editIndex;
};