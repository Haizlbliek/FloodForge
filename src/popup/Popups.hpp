#ifndef POPUPS_HPP
#define POPUPS_HPP

#include <vector>

#include "../Constants.hpp"
#include "../Window.hpp"
#include "../Utils.hpp"
#include "../math/Rect.hpp"
#include "../math/Vector.hpp"

class Popup {
	public:
		Popup(Window *window);

		virtual void draw(double mouseX, double mouseY, bool mouseInside);

		virtual void mouseClick(double mouseX, double mouseY);

		virtual const Rect Bounds() { return bounds; }

		virtual void close();

		virtual void accept() { close(); }
		virtual void reject() { close(); }

		virtual bool canStack(std::string popupName) { return false; }
		virtual std::string PopupName() { return "Popup"; }

		virtual bool drag(double mouseX, double mouseY);

		void offset(Vector2 offset);

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