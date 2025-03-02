#pragma once

#include "../Window.hpp"
#include "../font/Fonts.hpp"
#include "../Theme.hpp"

#include "Globals.hpp"

namespace DebugData {
    void draw(Window *window, Vector2 mouse, double lineSize, Vector2 screenBounds) {
        Connection *hoveringConnection = nullptr;
        Room *hoveringRoom = nullptr;

        for (auto it = connections.rbegin(); it != connections.rend(); it++) {
            Connection *connection = *it;

            if (connection->hovered(mouse, lineSize)) {
                hoveringConnection = connection;

                break;
            }
        }

        for (auto it = rooms.rbegin(); it != rooms.rend(); it++) {
            Room *room = *it;

            if (room->inside(mouse)) {
                hoveringRoom = room;
                break;
            }
        }

        std::vector<std::string> debugText;

        if (hoveringConnection != nullptr) {
            debugText.push_back("    Connection:");
            debugText.push_back("Room A: " + hoveringConnection->RoomA()->RoomName());
            debugText.push_back("Room B: " + hoveringConnection->RoomB()->RoomName());
            debugText.push_back("Connection A: " + std::to_string(hoveringConnection->ConnectionA()));
            debugText.push_back("Connection B: " + std::to_string(hoveringConnection->ConnectionB()));
        }

        if (hoveringRoom != nullptr) {
            std::string tags = "";
            for (std::string tag : hoveringRoom->Tags()) tags += " " + tag;
            debugText.push_back("    Room:");
            debugText.push_back("Name: " + hoveringRoom->RoomName());
            debugText.push_back("Tags:" + tags);
            debugText.push_back("Width: " + std::to_string(hoveringRoom->Width()));
            debugText.push_back("Height: " + std::to_string(hoveringRoom->Height()));
            debugText.push_back("Dens: " + std::to_string(hoveringRoom->DenCount()));
            if (hoveringRoom->Hidden()) {
                debugText.push_back("Layer: Hidden - " + std::to_string(hoveringRoom->Layer()));
            } else {
                debugText.push_back("Layer: " + std::to_string(hoveringRoom->Layer()));
            }
            if (hoveringRoom->Subregion() == -1) {
                debugText.push_back("Subregion: ");
            } else {
                debugText.push_back("Subregion: " + subregions[hoveringRoom->Subregion()]);
            }
        }

        int i = 1;

        Draw::color(0.0f, 0.0f, 0.0f, 1.0f);
        for (auto it = debugText.rbegin(); it != debugText.rend(); it++) {
            std::string line = *it;

            Fonts::rainworld->write(line, -screenBounds.x, -screenBounds.y + i*0.04 - 0.003, 0.03);
            Fonts::rainworld->write(line, -screenBounds.x, -screenBounds.y + i*0.04 + 0.003, 0.03);
            Fonts::rainworld->write(line, -screenBounds.x - 0.003, -screenBounds.y + i*0.04, 0.03);
            Fonts::rainworld->write(line, -screenBounds.x + 0.003, -screenBounds.y + i*0.04, 0.03);
            i++;
        }
        
        i = 1;

        setThemeColour(ThemeColour::Text);
        for (auto it = debugText.rbegin(); it != debugText.rend(); it++) {
            std::string line = *it;

            Fonts::rainworld->write(line, -screenBounds.x, -screenBounds.y + i*0.04, 0.03);
            i++;
        }
    }
}