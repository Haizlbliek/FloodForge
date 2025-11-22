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
		bool slatedForDeletion = false;

		Rect bounds;
};

namespace Popups {
	void init();

	void cleanup();

	void block();
	void draw();

	void addPopup(Popup *popup);

	void removePopup(Popup *popup);

	bool hasPopup(std::string popupName);

	extern std::vector<Popup*> popupTrash;
	extern std::vector<Popup*> popups;
	extern Popup *holdingPopup;
	extern Vector2 holdingStart;
};