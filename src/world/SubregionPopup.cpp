#include "SubregionPopup.hpp"

SubregionPopup::SubregionPopup(Window *window, std::set<Room*> newRooms) : Popup(window) {
	for (Room *room : newRooms) rooms.insert(room);
}

SubregionPopup::~SubregionPopup() {}

void SubregionPopup::draw(double mouseX, double mouseY, bool mouseInside, Vector2 screenBounds) {
	Popup::draw(mouseX, mouseY, mouseInside, screenBounds);
	
	if (minimized) return;
	
	double centreX = (bounds.x0 + bounds.x1) * 0.5;

	if (rooms.size() > 0) {
		setThemeColour(ThemeColour::Text);
		if (rooms.size() == 1) {
			Fonts::rainworld->writeCentred(toUpper((*rooms.begin())->roomName), centreX, bounds.y1 - 0.09, 0.04, CENTRE_XY);
		} else {
			Fonts::rainworld->writeCentred("Selected Rooms", centreX, bounds.y1 - 0.07, 0.04, CENTRE_XY);
		}

		double y = bounds.y1 - 0.15;
		drawSubregionButton(-1, "None", centreX, y, mouseX, mouseY);
		y -= 0.075;

		int id = 0;
		for (std::string subregion : EditorState::subregions) {
			drawSubregionButton(id, subregion, centreX, y, mouseX, mouseY);

			y -= 0.075;
			id++;
		}

		drawSubregionButton(-2, "+ new subregion +", centreX, y, mouseX, mouseY);
	}
}

void SubregionPopup::setSubregion(int subregion) {
	for (Room *room : rooms) room->subregion = subregion;
}

void SubregionPopup::mouseClick(double mouseX, double mouseY) {
	Popup::mouseClick(mouseX, mouseY);

	double centreX = (bounds.x0 + bounds.x1) * 0.5;
	mouseX -= centreX;
	mouseY -= (bounds.y0 + bounds.y1) * 0.5;

	int button = getButtonIndex(mouseX, mouseY);

	if (button == -1) {
	} else {
		if (mouseX <= -0.35 && button >= 1 && button <= EditorState::subregions.size()) {
			Popups::addPopup(new SubregionNewPopup(window, rooms, button - 1));
			close();
		} else if (mouseX <= 0.325 && mouseX >= -0.325) {
			if (button == 0) {
				setSubregion(-1);
				close();
			} else if (button <= EditorState::subregions.size()) {
				setSubregion(button - 1);
				close();
			} else if (button == EditorState::subregions.size() + 1) {
				Popups::addPopup(new SubregionNewPopup(window, rooms));
				close();
			}
		} else if (mouseX >= 0.35) {
			if (button != 0 && button <= EditorState::subregions.size()) {
				bool canRemove = true;
				for (Room *otherRoom : rooms) {
					if (otherRoom->subregion == button - 1) {
						canRemove = false;
						break;
					}
				}

				if (canRemove) {
					EditorState::subregions.erase(EditorState::subregions.begin() + (button - 1));
				} else {
					Popups::addPopup(new InfoPopup(window, "Can't remove subregion\nRooms still use it"));
				}
			}
		}
	}
}

int SubregionPopup::getButtonIndex(double mouseX, double mouseY) {
	if (mouseX < -0.4 || mouseX > 0.4) return -1;
	if (mouseY > 0.35) return -1;
	if (std::fmod(-mouseY + 0.35, 0.075) > 0.05) return -1;

	return floor((-mouseY + 0.35) / 0.075);
}

void SubregionPopup::drawSubregionButton(int subregionId, std::string subregion, double centerX, double y, double mouseX, double mouseY) {
	setThemeColour(ThemeColour::Button);
	fillRect(-0.325 + centerX, y, 0.325 + centerX, y - 0.05);

	if (rooms.size() == 1) {
		if ((*rooms.begin())->subregion == subregionId) {
			setThemeColour(ThemeColour::BorderHighlight);
		} else {
			setThemeColour(ThemeColour::Text);
		}
	} else {
		setThemeColour(ThemeColour::Text);
	}
	Fonts::rainworld->writeCentred(subregion, centerX, y - 0.02, 0.04, CENTRE_XY);

	if (Rect(-0.325 + centerX, y, 0.325 + centerX, y - 0.05).inside(mouseX, mouseY)) {
		setThemeColour(ThemeColour::BorderHighlight);
	} else {
		setThemeColour(ThemeColour::Border);
	}
	strokeRect(-0.325 + centerX, y, 0.325 + centerX, y - 0.05);

	if (subregionId >= 0) {
		setThemeColour(ThemeColour::Button);
		fillRect(-0.4 + centerX, y, -0.35 + centerX, y - 0.05);
		fillRect(0.35 + centerX, y, 0.4 + centerX, y - 0.05);

		setThemeColour(ThemeColour::Text);
		Fonts::rainworld->writeCentred("E", -0.37 + centerX, y, 0.04, CENTRE_X);
		Fonts::rainworld->writeCentred("-", 0.37 + centerX, y, 0.04, CENTRE_X);

		if (Rect(-0.4 + centerX, y, -0.35 + centerX, y - 0.05).inside(mouseX, mouseY)) {
			setThemeColour(ThemeColour::BorderHighlight);
		} else {
			setThemeColour(ThemeColour::Border);
		}
		strokeRect(-0.4 + centerX, y, -0.35 + centerX, y - 0.05);
		if (Rect(0.35 + centerX, y, 0.4 + centerX, y - 0.05).inside(mouseX, mouseY)) {
			setThemeColour(ThemeColour::BorderHighlight);
		} else {
			setThemeColour(ThemeColour::Border);
		}
		strokeRect(0.35 + centerX, y, 0.4 + centerX, y - 0.05);
	}
}