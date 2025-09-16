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
		virtual ~Popup() {}

		virtual void draw();

		virtual const Rect Bounds();

		virtual void close();

		virtual void finalCleanup() {};

		virtual void accept() { close(); }
		virtual void reject() { close(); }

		virtual bool canStack(std::string popupName) { return false; }
		virtual std::string PopupName() { return "Popup"; }

		virtual bool drag(double mouseX, double mouseY);

		void offset(Vector2 offset);

	protected:
		bool hovered;
		bool minimized = false;

		Rect bounds;
};

class Popups {
	public:
		static void init();

		static std::vector<Popup*> popupTrash;
		static std::vector<Popup*> popups;
		static Popup *holdingPopup;
		static Vector2 holdingStart;

		static void cleanup();

		static void draw(Vector2 screenBounds);
		
		static void addPopup(Popup *popup);

		static void removePopup(Popup *popup);

		static bool hasPopup(std::string popupName);
};