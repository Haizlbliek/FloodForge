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
#include "flood_forge/DenPopup.hpp"

#include "ExtraRoomData.hpp"

#define MENU_LAYER_FLOOD_FORGE 0
#define MENU_LAYER_DROPLET 1

class Button {
	public:
		Button(std::string text, Rect rect, int layer);

		Button *OnPress(std::function<void(Button*)> listener);

		void draw();

		void Text(const std::string text);

		std::string Text() const;

		Rect rect;
		bool darken = false;
		int layer;

	private:
		std::function<void(Button*)> listener;

		std::string text;
};

class MenuItems {
	public:
		static Button &addButton(std::string text, int layer);

		static void addLayerButton(std::string buttonName, int worldLayer, int layer);

		static void init();
		static void initFloodForge();
		static void initDroplet();

		static void cleanup();

		static void draw();

		static void setLayer(int layer);

	private:
		static void repositionButtons();

		static std::vector<Button*> buttons;
		static std::vector<Button*> layerButtons;

		static double currentButtonX;
		static int currentLayer;
};