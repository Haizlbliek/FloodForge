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
#include "DenPopup.hpp"
#include "FailureController.hpp"

#include "ExtraRoomData.hpp"

class Button {
	public:
		Button(std::string text, double x, double y, double width, double height, Font *font);

		Button *OnLeftPress(std::function<void(Button*)> listener);

		Button *OnRightPress(std::function<void(Button*)> listener);

		bool isHovered(Mouse *mouse, Vector2 screenBounds);

		void update(Mouse *mouse, Vector2 screenBounds);

		void draw(Mouse *mouse, Vector2 screenBounds);

		void Text(const std::string text);

		std::string Text() const;

		void X(const double x);

		Button &Darken(bool newDarken);

		const double Width() const;

	private:
		void press();

		void pressRight();

		std::vector<std::function<void(Button*)>> listenersLeft;
		std::vector<std::function<void(Button*)>> listenersRight;

		bool pressed = false;

		double x;
		double y;
		double width;
		double height;

		std::string text;
		Font *font;

		bool darken = false;
};

class MenuItems {
	public:
		static void loadTextures();

		static Button &addButton(std::string text);

		static void addLayerButton(std::string buttonName, int layer);

		static void init(Window *window);

		static void cleanup();

		static void draw(Mouse *mouse, Vector2 screenBounds);

		static GLuint textureButtonNormal;
		static GLuint textureButtonNormalHover;
		static GLuint textureButtonPress;
		static GLuint textureButtonPressHover;
		static GLuint textureBar;

	private:
		static void repositionButtons();
	
		static std::vector<Button*> buttons;
		static std::vector<Button*> layerButtons;

		static double currentButtonX;
};