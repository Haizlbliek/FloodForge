#pragma once

#include "../popup/Popups.hpp"

#include "DenPopup.hpp"

class RoomAttractivenessPopup : public Popup {
	public:
		RoomAttractivenessPopup(Window *window, std::set<Room *> rooms) : rooms(rooms), Popup(window) {
			bounds = Rect(-0.35, -0.35, 0.375 + 0.1, 0.35);
			scroll = 0.0;
			scrollTo = 0.0;

			window->addScrollCallback(this, scrollCallback);
		}

		~RoomAttractivenessPopup() {
		}

		void close();

		void draw(double mouseX, double mouseY, bool mouseInside, Vector2 screenBounds);

		void mouseClick(double mouseX, double mouseY);

		static void scrollCallback(void *object, double deltaX, double deltaY) {
			RoomAttractivenessPopup *popup = static_cast<RoomAttractivenessPopup*>(object);

			if (!popup->hovered) return;

			popup->scrollTo += deltaY * 0.06;

			popup->clampScroll();
		}

		std::string PopupName() { return "RoomAttractivenessPopup"; }

	private:
		const static RoomAttractiveness attractivenessIds[6];
		const static Colour attractivenessColors[6];
		const static std::string attractivenessNames[6];

		RoomAttractiveness selectAttractiveness = RoomAttractiveness::NEUTRAL;
		std::set<Room *> rooms;

		double scroll;
		double scrollTo;

		void clampScroll() {
			double width = 0.5;
			double height = 0.5;

			double buttonSize = std::min(width / 7.0, height / 7.0);
			double buttonPadding = 0.02;

			int items = CreatureTextures::creatures.size() / CREATURE_ROWS - 1;
			double size = items * (buttonSize + buttonPadding);

			if (scrollTo < -size) {
				scrollTo = -size;
				if (scroll <= -size + 0.06) {
					scroll = -size - 0.03;
				}
			}

			if (scrollTo > 0) {
				scrollTo = 0;
				if (scroll >= -0.06) {
					scroll = 0.03;
				}
			}
		}
		
		void setAllTo(RoomAttractiveness attr, std::string creature) {
			for (Room *room : rooms) {
				if (room->isOffscreen()) continue;

				if (attr == RoomAttractiveness::DEFAULT) {
					room->data.attractiveness.erase(creature);
				} else {
					room->data.attractiveness[creature] = attr;
				}
			}
		}
};