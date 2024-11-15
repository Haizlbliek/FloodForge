#ifndef POPUPS_HPP
#define POPUPS_HPP

#include <vector>

#include "../Constants.hpp"
#include "../Window.hpp"
#include "../Utils.hpp"
#include "../math/Rect.hpp"

class Popup {
	public:
		Popup(Window *window);

		virtual void draw(double mouseX, double mouseY, bool mouseInside);

		virtual void mouseClick(double mouseX, double mouseY);

		virtual const Rect Bounds() { return bounds; }

		virtual void close();

		virtual void accept() { close(); }
		virtual void reject() { close(); }

	protected:
		Window *window;
		Rect bounds;
};

class Popups {
	public:
		static void init() {
			textureUI = loadTexture(BASE_PATH + "assets/ui.png");
		}

		static GLuint textureUI;
		static std::vector<Popup*> popupTrash;

		static void cleanup();
};

extern std::vector<Popup*> popups;

void addPopup(Popup *popup);

void removePopup(Popup *popup);

#endif