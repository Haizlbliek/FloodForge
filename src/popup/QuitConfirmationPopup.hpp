#pragma once

#include "../gl.h"

#include "../Window.hpp"
#include "../Theme.hpp"

#include "Popups.hpp"

class QuitConfirmationPopup : public Popup {
	public:
		QuitConfirmationPopup(Window *window)
		: Popup(window) {
			bounds = Rect(-0.25, -0.15, 0.25, 0.15);
		}
		
		void draw(double mouseX, double mouseY, bool mouseInside, Vector2 screenBounds) {
			Popup::draw(mouseX, mouseY, mouseInside, screenBounds);
			
			mouseX -= bounds.X0() + 0.25;
			mouseY -= bounds.Y0() + 0.15;

			Draw::pushMatrix();

			Draw::translate(bounds.X0() + 0.25, bounds.Y0() + 0.15);

			setThemeColour(ThemeColour::Text);
			Fonts::rainworld->writeCentred("Exit FloodForge?", 0.0, 0.04, 0.04, CENTRE_XY);

			setThemeColour(ThemeColour::Button);
			fillRect(-0.2,  -0.09, -0.05, -0.03);
			fillRect( 0.05, -0.09,  0.2,  -0.03);

			setThemeColour(ThemeColour::Text);
			Fonts::rainworld->writeCentred("Cancel", -0.125, -0.06, 0.03, CENTRE_XY);
			Fonts::rainworld->writeCentred("Exit", 0.125, -0.06, 0.03, CENTRE_XY);

			if (Rect(-0.2, -0.09, -0.05, -0.03).inside(mouseX, mouseY)) {
				setThemeColour(ThemeColour::BorderHighlight);
				strokeRect(-0.2, -0.09, -0.05, -0.03);
			} else {
				setThemeColour(ThemeColour::Border);
				strokeRect(-0.2, -0.09, -0.05, -0.03);
			}

			if (Rect(0.05, -0.09, 0.2, -0.03).inside(mouseX, mouseY)) {
				setThemeColour(ThemeColour::BorderHighlight);
				strokeRect(0.05, -0.09, 0.2, -0.03);
			} else {
				setThemeColour(ThemeColour::Border);
				strokeRect(0.05, -0.09, 0.2, -0.03);
			}
			
			Draw::popMatrix();
		}

		void accept() {
			window->close();
		}

		void reject() {
			close();
		}

		void mouseClick(double mouseX, double mouseY) {
			Popup::mouseClick(mouseX, mouseY);

			mouseX -= bounds.X0() + 0.25;
			mouseY -= bounds.Y0() + 0.15;

			if (Rect(-0.2, -0.09, -0.05, -0.03).inside(mouseX, mouseY)) {
				reject();
			}

			if (Rect(0.05, -0.09, 0.2, -0.03).inside(mouseX, mouseY)) {
				accept();
			}
		}
};