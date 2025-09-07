#pragma once

#include <vector>
#include <functional>
#include <string>
#include <fstream>
#include <iostream>
#include <filesystem>
#include <tuple>
#include <iomanip>
#include <regex>

#include "../font/Fonts.hpp"
#include "../math/Rect.hpp"
#include "../math/Quadruple.hpp"

#include "../Utils.hpp"
#include "../Window.hpp"
#include "../Theme.hpp"

#include "Globals.hpp"
#include "Room.hpp"
#include "OffscreenRoom.hpp"
#include "popup/DenPopup.hpp"

#include "ExtraRoomData.hpp"

class Button {
	public:
		Button(std::string text, Rect rect);

		Button *OnPress(std::function<void(Button*)> listener);

		void draw();

		void Text(const std::string text);

		std::string Text() const;

		Rect rect;
		bool darken = false;

	private:
		std::function<void(Button*)> listener;

		std::string text;
};

class MenuItems {
	public:
		static Button &addButton(std::string text);

		static void addLayerButton(std::string buttonName, int layer);

		static void init(Window *window);

		static void cleanup();

		static void draw();

	private:
		static void repositionButtons();

		static std::vector<Button*> buttons;
		static std::vector<Button*> layerButtons;

		static double currentButtonX;
};