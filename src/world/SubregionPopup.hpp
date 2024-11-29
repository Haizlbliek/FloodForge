#ifndef SUBREGION_POPUP_HPP
#define SUBREGION_POPUP_HPP

#include "../Window.hpp"
#include "../Utils.hpp"

#include "Popups.hpp"
#include "Room.hpp"
#include "SubregionNewPopup.hpp"

class SubregionPopup : public Popup {
    public:
        SubregionPopup(Window *window, Room *room) : Popup(window), room(room) {
        }

		~SubregionPopup() {}

		void draw(double mouseX, double mouseY, bool mouseInside) {
            Popup::draw(mouseX, mouseY, mouseInside);
			// setThemeColour(THEME_TEXT_COLOUR);
            // Fonts::rainworld->write("Subregion", -0.494, 0.494, 0.04);
            
            if (room != nullptr) {
			    setThemeColour(THEME_TEXT_COLOUR);
			    Fonts::rainworld->writeCentred(room->RoomName(), 0.0, 0.4, 0.04, CENTRE_XY);

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
        }

        void mouseClick(double mouseX, double mouseY) {
            Popup::mouseClick(mouseX, mouseY);

            int button = getButtonIndex(mouseX, mouseY);

            if (button == -1) {
            } else {
                if (mouseX <= 0.325) {
                    if (button == 0) {
                        room->Subregion(-1);
                        close();
                    } else if (button <= subregions.size()) {
                        room->Subregion(button - 1);
                        close();
                    } else if (button == subregions.size() + 1) {
                        addPopup(new SubregionNewPopup(window, room));
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
                            addPopup(new WarningPopup(window, "Can't remove subregion\nRooms still use it"));
                        }
                    } else if (button == subregions.size() + 1) {
                    }
                }
            }
        }
        
		bool canStack(std::string popupName) { return popupName == "SubregionNewPopup" || popupName == "WarningPopup"; }
		std::string PopupName() { return "SubregionPopup"; }

    private:
        Room *room;

        int getButtonIndex(double mouseX, double mouseY) {
            if (mouseX < -0.4 || mouseX > 0.4) return -1;
            if (mouseY > 0.35) return -1;
            if (std::fmod(-mouseY + 0.35, 0.075) > 0.05) return -1;

            return floor((-mouseY + 0.35) / 0.075);
        }

        void drawSubregionButton(int subregionId, std::string subregion, double y, double mouseX, double mouseY) {
            setThemeColour(THEME_BUTTON_COLOUR);
            fillRect(-0.4, y, 0.325, y - 0.05);
            fillRect(0.35, y, 0.4, y - 0.05);

            if (room->Subregion() == subregionId) {
                setThemeColour(THEME_BORDER_HIGHLIGHT_COLOUR);
            } else {
                setThemeColour(THEME_TEXT_COLOUR);
            }
            Fonts::rainworld->writeCentred(subregion, -0.025, y - 0.02, 0.04, CENTRE_XY);            
            Fonts::rainworld->writeCentred("-", 0.37, y, 0.04, CENTRE_X);

            if (Rect(-0.4, y, 0.325, y - 0.05).inside(mouseX, mouseY)) {
                setThemeColour(THEME_BORDER_HIGHLIGHT_COLOUR);
                strokeRect(-0.4, y, 0.325, y - 0.05);
            } else {
                setThemeColour(THEME_BORDER_COLOUR);
                strokeRect(-0.4, y, 0.325, y - 0.05);
            }
            
            if (Rect(0.35, y, 0.4, y - 0.05).inside(mouseX, mouseY)) {
                setThemeColour(THEME_BORDER_HIGHLIGHT_COLOUR);
                strokeRect(0.35, y, 0.4, y - 0.05);
            } else {
                setThemeColour(THEME_BORDER_COLOUR);
                strokeRect(0.35, y, 0.4, y - 0.05);
            }
        }
};

#endif