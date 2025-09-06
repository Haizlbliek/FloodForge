#include "ConditionalPopup.hpp"

#include "../../ui/UI.hpp"
#include "../ConditionalTimelineTextures.hpp"

#define TIMELINE_ROWS 8

ConditionalPopup::ConditionalPopup(Window *window) : Popup(window) {
	this->connection = nullptr;
	this->lineage = nullptr;
	this->scroll = 0.0;
	this->bounds = Rect(-0.4, -0.4, 0.4, 0.4);
}

ConditionalPopup::ConditionalPopup(Window *window, Connection *connection) : ConditionalPopup(window) {
	this->connection = connection;
}

ConditionalPopup::ConditionalPopup(Window *window, std::set<Room*> rooms) : ConditionalPopup(window) {
	this->rooms = rooms;
}

ConditionalPopup::ConditionalPopup(Window *window, DenLineage *lineage) : ConditionalPopup(window) {
	this->lineage = lineage;
}

void ConditionalPopup::drawButton(Rect rect, std::string text, bool selected, double mouseX, double mouseY) {
	setThemeColour(ThemeColour::Button);
	fillRect(rect);

	setThemeColour(ThemeColour::Text);
	Fonts::rainworld->writeCentered(text, (rect.x0 + rect.x1) * 0.5, (rect.y0 + rect.y1) * 0.5, 0.03, CENTER_XY);

	if (rect.inside(mouseX, mouseY) || selected) {
		setThemeColour(ThemeColour::BorderHighlight);
	} else {
		setThemeColour(ThemeColour::Border);
	}
	strokeRect(rect);
}

void ConditionalPopup::draw(double mouseX, double mouseY, bool mouseInside, Vector2 screenBounds) {
	Popup::draw(mouseX, mouseY, mouseInside, screenBounds);

	if (minimized) return;

	double centerX = (bounds.x0 + bounds.x1) * 0.5;
	double buttonSize = 0.5 / 7.0;
	double buttonPadding = 0.02;
	double buttonY = bounds.y1 - 0.1;
	std::string hoverText = "";
	Room *firstRoom = nullptr;

	if (connection != nullptr) {
		drawButton(Rect::fromSize(bounds.x0 * 0.6 + centerX * 0.4 - 0.1, buttonY - 0.025, 0.2, 0.05), "ALL", connection->timelineType == ConnectionTimelineType::ALL, mouseX, mouseY);
		drawButton(Rect::fromSize(centerX - 0.1, buttonY - 0.025, 0.2, 0.05), "ONLY", connection->timelineType == ConnectionTimelineType::ONLY, mouseX, mouseY);
		drawButton(Rect::fromSize(bounds.x1 * 0.6 + centerX * 0.4 - 0.1, buttonY - 0.025, 0.2, 0.05), "EXCEPT", connection->timelineType == ConnectionTimelineType::EXCEPT, mouseX, mouseY);
	} else if (lineage != nullptr) {
		drawButton(Rect::fromSize(bounds.x0 * 0.6 + centerX * 0.4 - 0.1, buttonY - 0.025, 0.2, 0.05), "ALL", lineage->timelineType == ConnectionTimelineType::ALL, mouseX, mouseY);
		drawButton(Rect::fromSize(centerX - 0.1, buttonY - 0.025, 0.2, 0.05), "ONLY", lineage->timelineType == ConnectionTimelineType::ONLY, mouseX, mouseY);
		drawButton(Rect::fromSize(bounds.x1 * 0.6 + centerX * 0.4 - 0.1, buttonY - 0.025, 0.2, 0.05), "EXCEPT", lineage->timelineType == ConnectionTimelineType::EXCEPT, mouseX, mouseY);
	} else {
		firstRoom = *rooms.begin();
		drawButton(Rect::fromSize(bounds.x0 * 0.6 + centerX * 0.4 - 0.1, buttonY - 0.025, 0.2, 0.05), "DEFAULT", firstRoom->timelineType == RoomTimelineType::DEFAULT, mouseX, mouseY);
		drawButton(Rect::fromSize(centerX - 0.1, buttonY - 0.025, 0.2, 0.05), "EXCLUSIVE", firstRoom->timelineType == RoomTimelineType::EXCLUSIVE_ROOM, mouseX, mouseY);
		drawButton(Rect::fromSize(bounds.x1 * 0.6 + centerX * 0.4 - 0.1, buttonY - 0.025, 0.2, 0.05), "HIDE", firstRoom->timelineType == RoomTimelineType::HIDE_ROOM, mouseX, mouseY);
	}

	if ((connection != nullptr) ? (connection->timelineType != ConnectionTimelineType::ALL) : ((lineage == nullptr) ? (firstRoom->timelineType != RoomTimelineType::DEFAULT) : (lineage->timelineType != ConnectionTimelineType::ALL))) {
		std::string timeline;
		std::vector<std::string> unknowns;
		if (connection != nullptr) {
			for (std::string timeline : connection->timelines) {
				if (!ConditionalTimelineTextures::hasTimeline(timeline)) unknowns.push_back(timeline);
			}
		} else if (lineage != nullptr) {
			for (std::string timeline : lineage->timelines) {
				if (!ConditionalTimelineTextures::hasTimeline(timeline)) unknowns.push_back(timeline);
			}
		} else {
			for (std::string timeline : firstRoom->timelines) {
				if (!ConditionalTimelineTextures::hasTimeline(timeline)) unknowns.push_back(timeline);
			}
		}
		int count = ConditionalTimelineTextures::timelines.size() + unknowns.size();
		for (int y = 0; y <= (count / TIMELINE_ROWS); y++) {
			for (int x = 0; x < TIMELINE_ROWS; x++) {
				int id = x + y * TIMELINE_ROWS;
	
				if (id >= count) break;
	
				bool unknown = id >= ConditionalTimelineTextures::timelines.size();

				if (unknown) {
					timeline = unknowns[id - ConditionalTimelineTextures::timelines.size()];
				} else {
					timeline = ConditionalTimelineTextures::timelines[id];
				}
	
				bool isSelected;
				if (connection != nullptr) {
					isSelected = std::find(connection->timelines.begin(), connection->timelines.end(), timeline) != connection->timelines.end();
				} else if (lineage != nullptr) {
					isSelected = std::find(lineage->timelines.begin(), lineage->timelines.end(), timeline) != lineage->timelines.end();
				} else {
					isSelected = std::find(firstRoom->timelines.begin(), firstRoom->timelines.end(), timeline) != firstRoom->timelines.end();
				}

				GLuint texture = ConditionalTimelineTextures::getTexture(ConditionalTimelineTextures::timelines[id]);

				int w, h;
				glBindTexture(GL_TEXTURE_2D, texture);
				glGetTexLevelParameteriv(GL_TEXTURE_2D, 0, GL_TEXTURE_WIDTH, &w);
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

				UVRect rect = UVRect::fromSize(
					centerX + (x - 0.5 * TIMELINE_ROWS) * (buttonSize + buttonPadding) + buttonPadding * 0.5,
					(bounds.y1 - 0.12 - buttonPadding * 0.5) - (y + 1) * (buttonSize + buttonPadding) - scroll,
					buttonSize, buttonSize
				);
				rect.uv(0.5 - uvx, 0.5 + uvy, 0.5 + uvx, 0.5 - uvy);
				UI::ButtonResponse response = UI::TextureButton(rect, UI::TextureButtonMods().Selected(isSelected).TextureId(texture).TextureColor(isSelected ? Color(1.0, 1.0, 1.0) : Color(0.5, 0.5, 0.5)));

				if (response.clicked) {
					if (connection != nullptr) {
						if (connection->timelines.count(timeline)) {
							connection->timelines.erase(timeline);
						} else {
							connection->timelines.insert(timeline);
						}
					} else if (lineage != nullptr) {
						if (lineage->timelines.count(timeline)) {
							lineage->timelines.erase(timeline);
						} else {
							lineage->timelines.insert(timeline);
						}
					} else {
						for (Room *room : rooms) {
							if (room->timelines.count(timeline)) {
								room->timelines.erase(timeline);
							} else {
								room->timelines.insert(timeline);
							}
						}
					}
				}

				if (response.hovered) {
					hoverText = timeline;
				}
			}
		}
	}

	if (!hoverText.empty() && mouseInside) {
		double width = Fonts::rainworld->getTextWidth(hoverText, 0.04) + 0.02;
		setThemeColour(ThemeColour::Popup);
		fillRect(mouseX, mouseY, mouseX + width, mouseY + 0.06);
		setThemeColour(ThemeColour::Border);
		strokeRect(mouseX, mouseY, mouseX + width, mouseY + 0.06);
		setThemeColour(ThemeColour::Text);
		Fonts::rainworld->writeCentered(hoverText, mouseX + 0.01, mouseY + 0.03, 0.04, CENTER_Y);
	}
}

void ConditionalPopup::mouseClick(double mouseX, double mouseY) {
	Popup::mouseClick(mouseX, mouseY);

	double centerX = (bounds.x0 + bounds.x1) * 0.5;
	double buttonSize = 0.5 / 7.0;
	double buttonPadding = 0.02;

	double buttonY = bounds.y1 - 0.1;

	if (Rect(bounds.x0 * 0.6 + centerX * 0.4 - 0.1, buttonY - 0.025, bounds.x0 * 0.6 + centerX * 0.4 + 0.1, buttonY + 0.025).inside(mouseX, mouseY)) {
		if (connection != nullptr) {
			connection->timelineType = ConnectionTimelineType::ALL;
		} else if (lineage != nullptr) {
			lineage->timelineType = ConnectionTimelineType::ALL;
		} else {
			for (Room *room : rooms) {
				room->timelineType = RoomTimelineType::DEFAULT;
			}
		}
	}
	if (Rect(centerX - 0.1, buttonY - 0.025, centerX + 0.1, buttonY + 0.025).inside(mouseX, mouseY)) {
		if (connection != nullptr) {
			connection->timelineType = ConnectionTimelineType::ONLY;
		} else if (lineage != nullptr) {
			lineage->timelineType = ConnectionTimelineType::ONLY;
		} else {
			for (Room *room : rooms) {
				room->timelineType = RoomTimelineType::EXCLUSIVE_ROOM;
			}
		}
	}
	if (Rect(bounds.x1 * 0.6 + centerX * 0.4 - 0.1, buttonY - 0.025, bounds.x1 * 0.6 + centerX * 0.4 + 0.1, buttonY + 0.025).inside(mouseX, mouseY)) {
		if (connection != nullptr) {
			connection->timelineType = ConnectionTimelineType::EXCEPT;
		} else if (lineage != nullptr) {
			lineage->timelineType = ConnectionTimelineType::EXCEPT;
		} else {
			for (Room *room : rooms) {
				room->timelineType = RoomTimelineType::HIDE_ROOM;
			}
		}
	}
}