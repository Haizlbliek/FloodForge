#pragma once

#include <vector>

#include "../Constants.hpp"
#include "../Window.hpp"
#include "../Utils.hpp"
#include "../math/Rect.hpp"
#include "../math/Vector.hpp"

class Popup {
	public:
		Popup();

		virtual ~Popup() {
			delete window;
		}

		virtual void draw(double mouseX, double mouseY, bool mouseInside, Vector2 screenBounds);

		virtual void mouseClick(double mouseX, double mouseY);

		virtual const Rect Bounds() {
			if (minimized) {
				return Rect(bounds.X0(), bounds.Y1() - 0.05, bounds.X1(), bounds.Y1());
			}

			return bounds;
		}

		virtual void close();

		virtual void finalCleanup() {};

		virtual void accept() { close(); }
		virtual void reject() { close(); }

		virtual bool canStack(std::string popupName) { return false; }
		virtual std::string PopupName() { return "Popup"; }

		virtual bool drag(double mouseX, double mouseY);

		void offset(Vector2 offset);

		Window *window;

	protected:
		bool hovered;
		bool minimized = false;

		Rect bounds;
};

class Popups {
	public:
		static void init(Window *window) {
			sharedWindow = window;
			textureUI = loadTexture(BASE_PATH + "assets/ui.png");
		}

		static GLuint textureUI;
		static std::vector<Popup*> popupTrash;
		static std::vector<Popup*> popups;

		static void cleanup();

		static void draw(Vector2 mouse, Vector2 screenBounds);
		
		static void addPopup(Popup *popup);

		static void removePopup(Popup *popup);

		static bool hasPopup(std::string popupName);
	
		static Window *sharedWindow;
};