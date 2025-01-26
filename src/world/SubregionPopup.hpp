#ifndef SUBREGION_POPUP_HPP
#define SUBREGION_POPUP_HPP

#include "../Window.hpp"
#include "../Utils.hpp"

#include "../popup/Popups.hpp"
#include "../popup/WarningPopup.hpp"

#include "Room.hpp"
#include "SubregionNewPopup.hpp"

class SubregionPopup : public Popup {
    public:
        SubregionPopup(Window *window, std::set<Room*> newRooms) : Popup(window) {
            for (Room *room : newRooms) rooms.insert(room);
        }

		~SubregionPopup() {}

		void draw(double mouseX, double mouseY, bool mouseInside, Vector2 screenBounds) {
            Popup::draw(mouseX, mouseY, mouseInside, screenBounds);

			mouseX -= bounds.X0() + 0.5;
			mouseY -= bounds.Y0() + 0.5;

			Draw::pushMatrix();

			Draw::translate(bounds.X0() + 0.5, bounds.Y0() + 0.5);

            if (rooms.size() > 0) {
			    setThemeColour(ThemeColour::Text);
                if (rooms.size() == 1) {
			        Fonts::rainworld->writeCentred((*rooms.begin())->RoomName(), 0.0, 0.4, 0.04, CENTRE_XY);
                } else {
                    Fonts::rainworld->writeCentred("Selected Rooms", 0.0, 0.4, 0.04, CENTRE_XY);
                }

                double y = 0.35;
                drawSubregionButton(-1, "None", y, mouseX, mouseY);
                y -= 0.075;

                int id = 0;
                for (std::string subregion : subregions) {
                    drawSubregionButton(id, subregion, y, mouseX, mouseY);

                    y -= 0.075;
                    id++;
                }
                
                // int button = getButtonIndex(mouseX, mouseY);
                drawSubregionButton(-2, "+ new subregion +", y, mouseX, mouseY);
            }

            Draw::popMatrix();
        }

        void setSubregion(int subregion) {
            for (Room *room : rooms) room->Subregion(subregion);
        }

        void mouseClick(double mouseX, double mouseY) {
            Popup::mouseClick(mouseX, mouseY);

			mouseX -= bounds.X0() + 0.5;
			mouseY -= bounds.Y0() + 0.5;

            int button = getButtonIndex(mouseX, mouseY);

            if (button == -1) {
            } else {
                if (mouseX <= 0.325) {
                    if (button == 0) {
                        setSubregion(-1);
                        close();
                    } else if (button <= subregions.size()) {
                        setSubregion(button - 1);
                        close();
                    } else if (button == subregions.size() + 1) {
                        Popups::addPopup(new SubregionNewPopup(window, rooms));
                        close();
                    }
                } else if (mouseX >= 0.35) {
                    if (button == 0) {
                    } else if (button <= subregions.size()) {
                        bool canRemove = true;
                        for (Room *otherRoom : rooms) {
                            if (otherRoom->Subregion() == button - 1) {
                                canRemove = false;
                                break;
                            }
                        }

                        if (canRemove) {
                            subregions.erase(subregions.begin() + (button - 1));
                        } else {
                            Popups::addPopup(new WarningPopup(window, "Can't remove subregion\nRooms still use it"));
                        }
                    } else if (button == subregions.size() + 1) {
                    }
                }
            }
        }
        
		bool canStack(std::string popupName) { return popupName == "SubregionNewPopup" || popupName == "WarningPopup"; }
		std::string PopupName() { return "SubregionPopup"; }

    private:
        std::set<Room*> rooms;

        int getButtonIndex(double mouseX, double mouseY) {
            if (mouseX < -0.4 || mouseX > 0.4) return -1;
            if (mouseY > 0.35) return -1;
            if (std::fmod(-mouseY + 0.35, 0.075) > 0.05) return -1;

            return floor((-mouseY + 0.35) / 0.075);
        }

        void drawSubregionButton(int subregionId, std::string subregion, double y, double mouseX, double mouseY) {
            setThemeColour(ThemeColour::Button);
            fillRect(-0.4, y, 0.325, y - 0.05);
            fillRect(0.35, y, 0.4, y - 0.05);

            if (rooms.size() == 1) {
                if ((*rooms.begin())->Subregion() == subregionId) {
                    setThemeColour(ThemeColour::BorderHighlight);
                } else {
                    setThemeColour(ThemeColour::Text);
                }
            } else {
                setThemeColour(ThemeColour::Text);
            }
            Fonts::rainworld->writeCentred(subregion, -0.025, y - 0.02, 0.04, CENTRE_XY);            
            Fonts::rainworld->writeCentred("-", 0.37, y, 0.04, CENTRE_X);

            if (Rect(-0.4, y, 0.325, y - 0.05).inside(mouseX, mouseY)) {
                setThemeColour(ThemeColour::BorderHighlight);
                strokeRect(-0.4, y, 0.325, y - 0.05);
            } else {
                setThemeColour(ThemeColour::Border);
                strokeRect(-0.4, y, 0.325, y - 0.05);
            }
            
            if (Rect(0.35, y, 0.4, y - 0.05).inside(mouseX, mouseY)) {
                setThemeColour(ThemeColour::BorderHighlight);
                strokeRect(0.35, y, 0.4, y - 0.05);
            } else {
                setThemeColour(ThemeColour::Border);
                strokeRect(0.35, y, 0.4, y - 0.05);
            }
        }
};

#endif