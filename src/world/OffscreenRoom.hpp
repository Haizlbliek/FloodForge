#ifndef OFFSCREEN_ROOM_HPP
#define OFFSCREEN_ROOM_HPP

#include <algorithm>

#include "../font/Fonts.hpp"

#include "Room.hpp"
#include "DenPopup.hpp"

class OffscreenRoom : public Room {
	public:
		OffscreenRoom(std::string path, std::string name) {
			this->path = path;
			this->roomName = name;

			position = new Vector2(
				0.0f,
				0.0f
			);

			width = 72;
			height = 43;

			valid = false;
			
			geometry = nullptr;

			layer = 0;
			water = -1;
			subregion = -1;
			
			tag = "OffscreenRoom";
			hidden = false;
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
			Draw::color(1.00f, 1.00f, 1.00f);
			fillRect(position.x, position.y, position.x + width, position.y - height);

			Draw::color(0.00f, 0.00f, 0.00f);
			Fonts::rainworld->writeCentred(this->roomName, position.x + (width * 0.5), position.y - (height * 0.5), 5, CENTRE_XY);


			for (int i = 0; i < denEntrances.size(); i++) {
				if (dens[i].type == "" || dens[i].count == 0) continue;

				double rectX = position.x + width * 0.5 - denEntrances.size() * 2.0 + i * 4.0 + 2.0;
				double rectY = position.y - height * 0.25;
				
				double size = 2.0;

				if (Rect(rectX - size, rectY - size, rectX + size, rectY + size).inside(mousePosition.x, mousePosition.y)) {
					Draw::color(1.0, 1.0, 1.0);
				} else {
					Draw::color(0.75, 0.75, 0.75);
				}
				GLuint texture = CreatureTextures::getTexture(dens[i].type);
				glEnable(GL_BLEND);
				Draw::useTexture(texture);			
				Draw::begin(Draw::QUADS);

				int w, h;
				glBindTexture(GL_TEXTURE_2D, texture);
				glGetTexLevelParameteriv(GL_TEXTURE_2D, 0, GL_TEXTURE_WIDTH,  &w);
				glGetTexLevelParameteriv(GL_TEXTURE_2D, 0, GL_TEXTURE_HEIGHT, &h);
				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
				glBindTexture(GL_TEXTURE_2D, 0);

				float ratio = (float(w) / float(h) + 1.0) * 0.5;
				float uvx = 1.0 / ratio;
				float uvy = ratio;
				if (uvx < 1.0) {
					uvy /= uvx;
					uvx = 1.0;
				}
				if (uvy < 1.0) {
					uvx /= uvy;
					uvy = 1.0;
				}
				uvx *= 0.5;
				uvy *= 0.5;
				Draw::texCoord(0.5 - uvx, 0.5 + uvy); Draw::vertex(rectX - size, rectY - size);
				Draw::texCoord(0.5 + uvx, 0.5 + uvy); Draw::vertex(rectX + size, rectY - size);
				Draw::texCoord(0.5 + uvx, 0.5 - uvy); Draw::vertex(rectX + size, rectY + size);
				Draw::texCoord(0.5 - uvx, 0.5 - uvy); Draw::vertex(rectX - size, rectY + size);
				Draw::end();
				Draw::useTexture(0);
				glDisable(GL_BLEND);

				Draw::color(1.0, 0.0, 0.0);
				Fonts::rainworld->writeCentred(std::to_string(dens[i].count), rectX + size, rectY - size, 1.0 * size, CENTRE_XY);
			}


			if (inside(mousePosition)) {
				Draw::color(0.00f, 0.75f, 0.00f);
			} else {
				Draw::color(0.75f, 0.75f, 0.75f);
			}
			strokeRect(position.x, position.y, position.x + width, position.y - height);
		}
};

#endif