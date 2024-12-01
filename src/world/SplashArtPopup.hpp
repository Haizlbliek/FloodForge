#ifndef SPLASH_ART_POPUP_HPP
#define SPLASH_ART_POPUP_HPP

#include "../gl.h"

#include <iostream>
#include <string>
#include <vector>
#include <sstream>

#include "../Window.hpp"

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

		void draw(double mouseX, double mouseY, bool mouseInside) {
			glColor3f(0.0, 0.0, 0.0);
			fillRect(-0.9, -0.45, 0.9, 0.45);

			glBindTexture(GL_TEXTURE_2D, splashart->ID());

			glEnable(GL_TEXTURE_2D);
			glEnable(GL_BLEND);
			glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

			glBegin(GL_QUADS);

			glColor3f(0.75, 0.75, 0.75);

			glTexCoord2f(0.0f, 1.0f); glVertex2f(-0.89, -0.44);
			glTexCoord2f(1.0f, 1.0f); glVertex2f( 0.89, -0.44);
			glTexCoord2f(1.0f, 0.0f); glVertex2f( 0.89,  0.44);
			glTexCoord2f(0.0f, 0.0f); glVertex2f(-0.89,  0.44);

			glEnd();

			glDisable(GL_TEXTURE_2D);
			glDisable(GL_BLEND);

			glColor3f(1.0f, 1.0f, 1.0f);
			Fonts::rodondo->writeCentred("FloodForge", 0.0, 0.1, 0.2, CENTRE_XY);
			Fonts::rainworld->writeCentred("World Editor", 0.0, -0.1, 0.1, CENTRE_XY);
			Fonts::rainworld->write("v1.1", -0.88, 0.43, 0.04);
		}

		void mouseClick(double mouseX, double mouseY) {
			close();
		}

	private:
		Texture *splashart;
};

#endif