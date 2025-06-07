#pragma once

#include <algorithm>

#include "../font/Fonts.hpp"

#include "Room.hpp"
#include "DenPopup.hpp"
#include "Globals.hpp"

class OffscreenRoom : public Room {
	public:
		bool isOffscreen() override { return true; }

		OffscreenRoom(std::string path, std::string name) {
			this->path = path;
			this->roomName = name;

			position = new Vector2(
				0.0f,
				0.0f
			);

			width = 72;
			height = 43;

			valid = true;
			
			geometry = nullptr;

			layer = 0;
			water = -1;
			subregion = -1;
			
			tags.push_back("OffscreenRoom");
			
			data = ExtraRoomData();
		}

		int AddDen() {
			dens.push_back(Den("", 0, "", 0.0));
			denEntrances.push_back(Vector2i(0, 0));

			return dens.size() - 1;
		}

		~OffscreenRoom() {
		}

		void cleanup() {
			int index = 0;
			denEntrances.erase(std::remove_if(denEntrances.begin(), denEntrances.end(), [&](Vector2i &denCoord) {
				Den &den = dens[index++];

				return den.count == 0 || den.type == "";
			}), denEntrances.end());
			dens.erase(std::remove_if(dens.begin(), dens.end(), [](Den &den) {
				return den.count == 0 || den.type == "";
			}), dens.end());
		}

		int denAt(double mouseX, double mouseY) {
			for (int i = 0; i < denEntrances.size(); i++) {
				if (dens[i].type == "" || dens[i].count == 0) continue;

				double rectX = position.x + width * 0.5 - denEntrances.size() * 2.0 + i * 4.0 + 2.0;
				double rectY = position.y - height * 0.25;
				
				double size = 2.0;

				Rect rect = Rect(rectX - size, rectY - size, rectX + size, rectY + size);

				if (rect.inside(mouseX, mouseY)) {
					return i;
				}
			}

			return -1;
		}

		void draw(Vector2 mousePosition, double lineSize, Vector2 screenBounds) override {
			Draw::color(RoomHelpers::RoomAir);
			fillRect(position.x, position.y, position.x + width, position.y - height);

			Draw::color(RoomHelpers::RoomSolid);
			Fonts::rainworld->writeCentred(this->roomName, position.x + (width * 0.5), position.y - (height * 0.5), 5, CENTRE_XY);


			for (int i = 0; i < denEntrances.size(); i++) {
				if (dens[i].type == "" || dens[i].count == 0) continue;

				double rectX = position.x + width * 0.5 - denEntrances.size() * 2.0 + i * 4.0 + 2.0;
				double rectY = position.y - height * 0.25;
				double scale = selectorScale;
		
				if (i == hoveredDen) scale *= 1.5;
		
				RoomHelpers::drawTexture(CreatureTextures::getTexture(dens[i].type), rectX, rectY, scale);
		
				Draw::color(1.0, 0.0, 0.0);
				Fonts::rainworld->writeCentred(std::to_string(dens[i].count), rectX + 0.5 + scale * 0.25, rectY - 0.5 - scale * 0.5, 0.5 * scale, CENTRE_XY);
		
			}


			if (inside(mousePosition)) {
				Draw::color(0.00f, 0.75f, 0.00f);
			} else {
				Draw::color(0.75f, 0.75f, 0.75f);
			}
			strokeRect(position.x, position.y, position.x + width, position.y - height);
		}
};