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

#include "../popup/MarkdownPopup.hpp"

#include "Globals.hpp"
#include "Room.hpp"
#include "OffscreenRoom.hpp"
#include "popup/DenPopup.hpp"

#include "ExtraRoomData.hpp"

#define MENU_LAYER_FLOOD_FORGE 0
#define MENU_LAYER_DROPLET 1

enum class ButtonAlignment {
	LEFT, 
	RIGHT   
};

class Button {
	public:
		Button(std::string text, Rect rect, int layer, ButtonAlignment alignment = ButtonAlignment::LEFT);

		Button *OnPress(std::function<void(Button*)> listener);
		Button *SetAlignment(ButtonAlignment align);

		void draw();

		void Text(const std::string text);

		std::string Text() const;

		Rect rect;
		bool darken = false;
		int layer;
		ButtonAlignment alignment;

	private:
		std::function<void(Button*)> listener;

		std::string text;
};

class MenuItems {
	public:
		static Button &addButton(std::string text, int layer, ButtonAlignment alignment = ButtonAlignment::LEFT	);

		static void addLayerButton(std::string buttonName, int worldLayer, int layer, ButtonAlignment alignment = ButtonAlignment::LEFT);

		static void init();
		static void initFloodForge();
		static void initDroplet();

		static void cleanup();

		static void draw();

		static void setLayer(int layer);

		static void checkWindowResize();

	private:
		static void repositionButtons();

		static std::vector<Button*> buttons;
		static std::vector<Button*> layerButtons;

		static double currentButtonX;
		static double currentButtonXRight;
		static int currentLayer;

		static double lastWindowWidth;
		static double lastWindowHeight;
};