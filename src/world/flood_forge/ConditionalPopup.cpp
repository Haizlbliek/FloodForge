#include "ConditionalPopup.hpp"

#include "../../ui/UI.hpp"
#include "../ConditionalTimelineTextures.hpp"
#include "FloodForgeWindow.hpp"

#define TIMELINE_ROWS 8

ConditionalPopup::ConditionalPopup() : Popup() {
	this->connection = nullptr;
	this->lineage = nullptr;
	this->scroll = 0.0;
	this->bounds = Rect(-0.4, -0.4, 0.4, 0.4);
}

ConditionalPopup::ConditionalPopup(Connection *connection) : ConditionalPopup() {
	this->connection = connection;
}

ConditionalPopup::ConditionalPopup(std::set<Room*> rooms) : ConditionalPopup() {
	this->rooms = rooms;
}

ConditionalPopup::ConditionalPopup(DenLineage *lineage) : ConditionalPopup() {
	this->lineage = lineage;
}

const TimelineType ConditionalPopup::timelineType() const {
	if (connection != nullptr) {
		return connection->timelineType;
	} else if (lineage != nullptr) {
		return lineage->timelineType;
	} else {
		return (*rooms.begin())->timelineType;
	}
}

void ConditionalPopup::timelineType(TimelineType type) {
	TimelineTypeChange *change = new TimelineTypeChange();

	if (connection != nullptr) {
		change->addConnection(connection, type);
	} else if (lineage != nullptr) {
		change->addLineage(lineage, type);
	} else {
		for (Room *room : rooms) {
			change->addRoom(room, type);
		}
	}

	FloodForgeWindow::history.change(change);
}

void ConditionalPopup::drawButton(Rect rect, std::string text, TimelineType type) {
	if (UI::TextButton(rect, text, UI::TextButtonMods().Selected(timelineType() == type))) {
		timelineType(type);
	}
}

void ConditionalPopup::draw() {
	Popup::draw();

	if (minimized) return;

	double centerX = (bounds.x0 + bounds.x1) * 0.5;
	double buttonSize = 0.5 / 7.0;
	double buttonPadding = 0.02;
	double buttonY = bounds.y1 - 0.1;
	std::string hoverText = "";
	Room *firstRoom = nullptr;

	if (connection != nullptr || lineage != nullptr) {
		drawButton(Rect::fromSize(bounds.x0 * 0.6 + centerX * 0.4 - 0.1, buttonY - 0.025, 0.2, 0.05), "ALL", TimelineType::ALL);
		drawButton(Rect::fromSize(centerX - 0.1, buttonY - 0.025, 0.2, 0.05), "ONLY", TimelineType::ONLY);
		drawButton(Rect::fromSize(bounds.x1 * 0.6 + centerX * 0.4 - 0.1, buttonY - 0.025, 0.2, 0.05), "EXCEPT", TimelineType::EXCEPT);
	} else {
		firstRoom = *rooms.begin();
		drawButton(Rect::fromSize(bounds.x0 * 0.6 + centerX * 0.4 - 0.1, buttonY - 0.025, 0.2, 0.05), "DEFAULT", TimelineType::ALL);
		drawButton(Rect::fromSize(centerX - 0.1, buttonY - 0.025, 0.2, 0.05), "EXCLUSIVE", TimelineType::ONLY);
		drawButton(Rect::fromSize(bounds.x1 * 0.6 + centerX * 0.4 - 0.1, buttonY - 0.025, 0.2, 0.05), "HIDE", TimelineType::EXCEPT);
	}

	if (timelineType() != TimelineType::ALL) {
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
					TimelineChange *change = new TimelineChange(!isSelected, timeline);
					if (connection != nullptr) {
						change->addConnection(connection);
					} else if (lineage != nullptr) {
						change->addLineage(lineage);
					} else {
						for (Room *room : rooms) {
							change->addRoom(room);
						}
					}
					FloodForgeWindow::history.change(change);
				}

				if (response.hovered) {
					hoverText = timeline;
				}
			}
		}
	}

	if (!hoverText.empty() && hovered) {
		double width = Fonts::rainworld->getTextWidth(hoverText, 0.04) + 0.02;
		Rect rect = Rect::fromSize(UI::mouse.x, UI::mouse.y, width, 0.06);
		setThemeColor(ThemeColor::Popup);
		fillRect(rect);
		setThemeColor(ThemeColor::Border);
		strokeRect(rect);
		setThemeColor(ThemeColor::Text);
		Fonts::rainworld->writeCentered(hoverText, UI::mouse.x + 0.01, UI::mouse.y + 0.03, 0.04, CENTER_Y);
	}
}