#pragma once

#include "../../popup/Popups.hpp"
#include "../Connection.hpp"

class ConditionalPopup : public Popup {
	public:
		ConditionalPopup(Window *window, Connection *connection);

		ConditionalPopup(Window *window, std::set<Room*> rooms);

		ConditionalPopup(Window *window, DenLineage *lineage);

		void draw(double mouseX, double mouseY, bool mouseInside, Vector2 screenBounds);

		void mouseClick(double mouseX, double mouseY);

		std::string PopupName() { return "ConditionalPopup"; }

	private:
		ConditionalPopup(Window *window);

		void drawButton(Rect rect, std::string text, bool selected, double mouseX, double mouseY);

		Connection *connection;
		std::set<Room*> rooms;
		DenLineage *lineage;

		double scroll;
};