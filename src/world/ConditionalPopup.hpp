#pragma once

#include "../popup/Popups.hpp"
#include "Connection.hpp"

class ConditionalPopup : public Popup {
	public:
		ConditionalPopup(Window *window, Connection *connection, std::set<Room*> rooms);

		void draw(double mouseX, double mouseY, bool mouseInside, Vector2 screenBounds);

		void mouseClick(double mouseX, double mouseY);

		std::string PopupName() { return "ConditionalPopup"; }

	private:
		void drawButton(Rect rect, std::string text, bool selected, double mouseX, double mouseY);

		bool connectionType;

		Connection *connection;
		std::set<Room*> rooms;

		double scroll;
};