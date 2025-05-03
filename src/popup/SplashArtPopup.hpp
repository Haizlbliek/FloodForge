#pragma once

#include "../gl.h"

#include <iostream>
#include <string>
#include <vector>
#include <sstream>

#include "MarkdownPopup.hpp"
#include "../Window.hpp"
#include "../Draw.hpp"
#include "../font/Fonts.hpp"

#include "Popups.hpp"

class SplashArtPopup : public Popup {
	public:
		SplashArtPopup(Window *window)
		: Popup(window) {
			bounds = Rect(-1.0, -1.0, 1.0, 1.0);

			splashart = new Texture(BASE_PATH + "assets/splash.png");
		}

		~SplashArtPopup() {
			delete splashart;
		}
		
		const Rect Bounds() override {
			return Rect(-100.0, -100.0, 100.0, 100.0);
		}

		void draw(double mouseX, double mouseY, bool mouseInside, Vector2 screenBounds) {
			Draw::color(0.0, 0.0, 0.0);
			fillRect(-0.9, -0.45, 0.9, 0.45);

			Draw::useTexture(splashart->ID());
			glEnable(GL_BLEND);
			glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

			Draw::begin(Draw::QUADS);

			Draw::color(0.75, 0.75, 0.75);

			Draw::texCoord(0.0f, 1.0f); Draw::vertex(-0.89, -0.44);
			Draw::texCoord(1.0f, 1.0f); Draw::vertex(0.89, -0.44);
			Draw::texCoord(1.0f, 0.0f); Draw::vertex(0.89, 0.44);
			Draw::texCoord(0.0f, 0.0f); Draw::vertex(-0.89, 0.44);

			Draw::end();

			Draw::useTexture(0);
			glDisable(GL_BLEND);

			Draw::color(1.0f, 1.0f, 1.0f);
			Fonts::rodondo->writeCentred("FloodForge", 0.0, 0.1, 0.2, CENTRE_XY);
			Fonts::rainworld->writeCentred("World Editor", 0.0, -0.1, 0.1, CENTRE_XY);
			Fonts::rainworld->write("v1.5.2", -0.88, 0.43, 0.04);

			strokeRect(-0.9, -0.45, 0.9, 0.45);
		}

		void mouseClick(double mouseX, double mouseY) {
			close();
			if (Settings::getSetting<bool>(Settings::Setting::HideTutorial)) return;

			Popups::addPopup(new MarkdownPopup(window, BASE_PATH + "docs/controls.md"));
		}
		
		bool canStack(std::string popupName) override { return popupName == "MarkdownPopup"; }
		std::string PopupName() override { return "SplashArtPopup"; }

	private:
		Texture *splashart;
};