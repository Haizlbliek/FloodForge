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
			if (!visibleLayers[connection->RoomA()->layer]) continue;
			if (!visibleLayers[connection->RoomB()->layer]) continue;

            if (connection->hovered(mouse, lineSize)) {
                hoveringConnection = connection;

                break;
            }
        }

        for (auto it = rooms.rbegin(); it != rooms.rend(); it++) {
            Room *room = *it;
            if (!visibleLayers[room->layer]) continue;

            if (room->inside(mouse)) {
                hoveringRoom = room;
                break;
            }
        }

        std::vector<std::string> debugText;

        if (hoveringConnection != nullptr) {
            debugText.push_back("    Connection:");
            debugText.push_back("Room A: " + hoveringConnection->RoomA()->roomName);
            debugText.push_back("Room B: " + hoveringConnection->RoomB()->roomName);
            debugText.push_back("Connection A: " + std::to_string(hoveringConnection->ConnectionA()));
            debugText.push_back("Connection B: " + std::to_string(hoveringConnection->ConnectionB()));
        }

        if (hoveringRoom != nullptr) {
            std::string tags = "";
            for (std::string tag : hoveringRoom->Tags()) tags += " " + tag;
            debugText.push_back("    Room:");
            debugText.push_back("Name: " + hoveringRoom->roomName);
            debugText.push_back("Tags:" + tags);
            debugText.push_back("Width: " + std::to_string(hoveringRoom->Width()));
            debugText.push_back("Height: " + std::to_string(hoveringRoom->Height()));
            debugText.push_back("Dens: " + std::to_string(hoveringRoom->DenCount()));
            debugText.push_back(hoveringRoom->data.merge ? "Merge: true" : "Merge: false");
            debugText.push_back("Dev Items: " + std::to_string(hoveringRoom->data.devItems.size()));
            if (hoveringRoom->data.hidden) {
                debugText.push_back("Layer: Hidden - " + std::to_string(hoveringRoom->layer));
            } else {
                debugText.push_back("Layer: " + std::to_string(hoveringRoom->layer));
            }
            if (hoveringRoom->subregion == -1) {
                debugText.push_back("Subregion: <<NONE>>");
            } else {
                debugText.push_back("Subregion: " + subregions[hoveringRoom->subregion]);
            }
        }
        
        for (auto it = rooms.rbegin(); it != rooms.rend(); it++) {
            Room *room = *it;
            
            Vector2 roomMouse = mouse - room->Position();
            Vector2 shortcutPosition;
            
            if (room->isOffscreen()) {
                for (int i = 0; i < room->DenCount(); i++) {
                    shortcutPosition = Vector2(room->Width() * 0.5 - room->DenCount() * 2.0 + i * 4.0 + 2.5, -room->Height() * 0.25 - 0.5);
                    
                    if (roomMouse.distanceTo(shortcutPosition) < selectorScale) {
                        debugText.push_back("   Shortcut:");
                        debugText.push_back("Den: Offscreen - " + std::to_string(i));
                    }
                }
            } else {
                for (Vector2i shortcut : room->DenEntrances()) {
                    shortcutPosition = Vector2(shortcut.x + 0.5, -1 - shortcut.y + 0.5);
                    
                    if (roomMouse.distanceTo(shortcutPosition) < selectorScale) {
                        debugText.push_back("   Shortcut:");
                        debugText.push_back("Den: " + std::to_string(shortcut.x) + ", " + std::to_string(shortcut.y));
                    }
                }
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