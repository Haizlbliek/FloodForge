#pragma once

#include "AcronymPopup.hpp"
#include "MenuItems.hpp"

class ChangeAcronymPopup : public AcronymPopup {
    public:
        ChangeAcronymPopup(Window *window) : AcronymPopup(window) {
        }

        void accept() {
            if (text.length() < 2) return;
        
            close();
            if (offscreenDen != nullptr) {
                rooms.erase(std::remove(rooms.begin(), rooms.end(), offscreenDen), rooms.end());
                OffscreenRoom *newOffscreenDen = new OffscreenRoom("offscreenden" + toLower(text), "OffscreenDen" + text);
                newOffscreenDen->Position(offscreenDen->Position());
                newOffscreenDen->layer = offscreenDen->layer;
                newOffscreenDen->hidden = offscreenDen->hidden;

                for (const Den &oldDen : offscreenDen->Dens()) {
                    Den &newDen = newOffscreenDen->CreatureDen01(newOffscreenDen->AddDen());
                    newDen.count = oldDen.count;
                    newDen.data = oldDen.data;
                    newDen.tag = oldDen.tag;
                    newDen.type = oldDen.type;
                }

                rooms.push_back(newOffscreenDen);
                delete offscreenDen;
                offscreenDen = newOffscreenDen;
            }

            for (Room *room : rooms) {
                if (room == offscreenDen) continue;

                room->roomName = toLower(text) + room->roomName.substr(room->roomName.find('_'));
            }

            MenuItems::WorldAcronym(toLower(text));
        }
};