#ifndef QUIT_CONFIRMATION_POPUP
#define QUIT_CONFIRMATION_POPUP

#include "../gl.h"

#include "../Window.hpp"

#include "Popups.hpp"

class QuitConfirmationPopup : public Popup {
	public:
		QuitConfirmationPopup(Window *window)
		: Popup(window) {
			bounds = Rect(-0.25, -0.15, 0.25, 0.15);
		}
		
		void draw(double mouseX, double mouseY, bool mouseInside) {
			Popup::draw(mouseX, mouseY, mouseInside);

			setThemeColour(THEME_TEXT_COLOUR);
			Fonts::rainworld->writeCentred("Exit FloodForge?", 0.0, 0.04, 0.04, CENTRE_XY);

			setThemeColour(THEME_BUTTON_COLOUR);
			fillrect(-0.2,  -0.09, -0.05, -0.03);
			fillrect( 0.05, -0.09,  0.2,  -0.03);

			setThemeColour(THEME_TEXT_COLOUR);
			Fonts::rainworld->writeCentred("Cancel", -0.125, -0.06, 0.03, CENTRE_XY);
			Fonts::rainworld->writeCentred("Exit", 0.125, -0.06, 0.03, CENTRE_XY);

			if (Rect(-0.2, -0.09, -0.05, -0.03).inside(mouseX, mouseY)) {
				setThemeColour(THEME_BORDER_HIGHLIGHT_COLOUR);
				strokerect(-0.2, -0.09, -0.05, -0.03);
			} else {
				setThemeColour(THEME_BORDER_COLOUR);
				strokerect(-0.2, -0.09, -0.05, -0.03);
			}

			if (Rect(0.05, -0.09, 0.2, -0.03).inside(mouseX, mouseY)) {
				setThemeColour(THEME_BORDER_HIGHLIGHT_COLOUR);
				strokerect(0.05, -0.09, 0.2, -0.03);
			} else {
				setThemeColour(THEME_BORDER_COLOUR);
				strokerect(0.05, -0.09, 0.2, -0.03);
			}
		}

		void accept() {
			window->close();
		}

		void reject() {
			close();
		}

		void mouseClick(double mouseX, double mouseY) {
			Popup::mouseClick(mouseX, mouseY);

			if (Rect(-0.2, -0.09, -0.05, -0.03).inside(mouseX, mouseY)) {
				reject();
			}

			if (Rect(0.05, -0.09, 0.2, -0.03).inside(mouseX, mouseY)) {
				accept();
			}
		}
};

#endif