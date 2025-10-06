#include "SubregionPopup.hpp"

#include "../../ui/UI.hpp"
#include "ColorEditPopup.hpp"

SubregionPopup::SubregionPopup(std::set<Room*> newRooms) : Popup() {
	for (Room *room : newRooms) rooms.insert(room);
	
	scroll = 0.0;
	scrollTo = 0.0;

	UI::window->addScrollCallback(this, scrollCallback);
}

SubregionPopup::~SubregionPopup() {}

void SubregionPopup::draw() {
	Popup::draw();
	
	if (minimized) return;

	scroll += (scrollTo - scroll) * Settings::getSetting<double>(Settings::Setting::PopupScrollSpeed);
	
	double centreX = (bounds.x0 + bounds.x1) * 0.5;

	if (rooms.size() > 0) {
		setThemeColour(ThemeColour::Text);
		if (rooms.size() == 1) {
			Fonts::rainworld->writeCentered((*rooms.begin())->roomName, centreX, bounds.y1 - 0.09, 0.04, CENTER_XY);
		} else {
			Fonts::rainworld->writeCentered("Selected Rooms", centreX, bounds.y1 - 0.07, 0.04, CENTER_XY);
		}
	
		glEnable(GL_SCISSOR_TEST);
		double clipBottom = ((bounds.y0 + 0.01 + UI::screenBounds.y) * 0.5) * UI::window->Height();
		double clipTop = ((bounds.y1 - 0.14 + UI::screenBounds.y) * 0.5) * UI::window->Height();
		glScissor(0, clipBottom, UI::window->Width(), clipTop - clipBottom);


		double y = bounds.y1 - 0.15 - scroll;
		drawSubregionButton(-1, "None", centreX, y);
		y -= 0.075;

		int id = 0;
		for (std::string subregion : EditorState::subregions) {
			drawSubregionButton(id, subregion, centreX, y);

			y -= 0.075;
			id++;
		}

		drawSubregionButton(-2, "+ new subregion +", centreX, y);
	
		glDisable(GL_SCISSOR_TEST);
	}
}

void SubregionPopup::setSubregion(int subregion) {
	for (Room *room : rooms) room->subregion = subregion;
}

void SubregionPopup::close() {
	Popups::removePopup(this);

	UI::window->removeScrollCallback(this, scrollCallback);
}

int SubregionPopup::getButtonIndex(double mouseX, double mouseY) {
	if (mouseX < -0.4 || mouseX > 0.4) return -1;
	if (mouseY > 0.35) return -1;
	if (std::fmod(-mouseY + 0.35, 0.075) > 0.05) return -1;

	return floor((-mouseY + 0.35) / 0.075);
}

void SubregionPopup::drawSubregionButton(int subregionId, std::string subregion, double centerX, double y) {
	Rect rect(-0.325 + centerX, y, 0.325 + centerX, y - 0.05);
	bool selected = false;
	if (rooms.size() == 1) {
		selected = (*rooms.begin())->subregion == subregionId;
	}
	if (UI::TextButton(rect, subregion, UI::TextButtonMods().Selected(selected))) {
		if (subregionId == -1) {
			setSubregion(-1);
			close();
		} else if (subregionId <= EditorState::subregions.size()) {
			setSubregion(subregionId);
			close();
		} else {
			Popups::addPopup(new SubregionNewPopup(rooms));
			close();
		}
	}

	if (subregionId >= 0) {
		if (UI::TextButton(Rect(bounds.x0 + 0.01, y, -0.335 + centerX, y - 0.05), "Edit")) {
			Popups::addPopup(new SubregionNewPopup(rooms, subregionId));
			close();
		}

		if (UI::TextButton(Rect(0.335 + centerX, y, 0.385 + centerX, y - 0.05), "X")) {
			if (UI::window->modifierPressed(GLFW_MOD_SHIFT)) {
				EditorState::subregions.erase(EditorState::subregions.begin() + subregionId);

				for (Room *otherRoom : EditorState::rooms) {
					if (otherRoom->subregion == subregionId) {
						otherRoom->subregion = -1;
					} else if (otherRoom->subregion > subregionId) {
						otherRoom->subregion--;
					}
				}
			} else {
				bool canRemove = true;
				for (Room *otherRoom : EditorState::rooms) {
					if (otherRoom->subregion == subregionId) {
						canRemove = false;
						break;
					}
				}

				if (canRemove) {
					EditorState::subregions.erase(EditorState::subregions.begin() + subregionId);

					for (Room *otherRoom : EditorState::rooms) {
						if (otherRoom->subregion >= subregionId) {
							otherRoom->subregion--;
						}
					}
				} else {
					Popups::addPopup(new InfoPopup("Cannot remove subregion if assigned to rooms\n(Hold shift to force)"));
				}
			}
		}
	}

	if (subregionId >= -1) {
		std::vector<Colour> colors = Settings::getSetting<std::vector<Colour>>(Settings::Setting::SubregionColors);
		Colour subregionColor = Settings::getSetting<Colour>(Settings::Setting::NoSubregionColor);
		if (colors.size() != 0 && subregionId != -1) {
			subregionColor = colors[subregionId % colors.size()];
		}
		bool exists = EditorState::region.overrideSubregionColors.count(subregionId) > 0;
		if (exists) {
			subregionColor = EditorState::region.overrideSubregionColors[subregionId];
		}
		if (UI::TextureButton(UVRect(0.395 + centerX, y, 0.445 + centerX, y - 0.05).uv(exists ? 0.75 : 0.5, 0.25, exists ? 1.0 : 0.75, 0.5), UI::TextureButtonMods().TextureColor(subregionColor))) {
			if (!exists) {
				EditorState::region.overrideSubregionColors[subregionId] = subregionColor;
			}
	
			Popups::addPopup(new ColorEditPopup(EditorState::region.overrideSubregionColors[subregionId]));
		}
		if (exists) {
			if (UI::TextureButton(UVRect::fromSize(0.455 + centerX, y - 0.01, 0.03, -0.03).uv(0.5, 0.25, 0.75, 0.0))) {
				EditorState::region.overrideSubregionColors.erase(subregionId);
			}
		}
	}
}

void SubregionPopup::scrollCallback(void *object, double deltaX, double deltaY) {
	SubregionPopup *popup = static_cast<SubregionPopup*>(object);

	if (!popup->hovered) return;

	popup->scrollTo += deltaY * 0.075;
	
	popup->clampScroll();
}


void SubregionPopup::clampScroll() {
	double width = 0.5;
	double height = 0.5;

	double maxScroll = (EditorState::subregions.size() - 3) * -0.075;

	if (scrollTo < maxScroll) {
		scrollTo = maxScroll;
		if (scroll <= maxScroll + 0.06) {
			scroll = maxScroll - 0.03;
		}
	}

	if (scrollTo > 0) {
		scrollTo = 0;
		if (scroll >= -0.06) {
			scroll = 0.03;
		}
	}
}