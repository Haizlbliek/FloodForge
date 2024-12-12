#include "../Window.hpp"
#include "../font/Fonts.hpp"
#include "../Theme.hpp"

#include "Globals.hpp"

namespace DebugData {
    void draw(Window *window, Vector2 mouse) {
        Connection *hoveringConnection = nullptr;
        Room *hoveringRoom = nullptr;

        for (auto it = connections.rbegin(); it != connections.rend(); it++) {
            Connection *connection = *it;

            if (connection->collides(mouse)) {
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
            debugText.push_back("Connection A: " + hoveringConnection->ConnectionA());
            debugText.push_back("Connection B: " + hoveringConnection->ConnectionB());
        }

        if (hoveringRoom != nullptr) {
            debugText.push_back("    Room:");
            debugText.push_back("Name: " + hoveringRoom->RoomName());
            debugText.push_back("Width: " + std::to_string(hoveringRoom->Width()));
            debugText.push_back("Height: " + std::to_string(hoveringRoom->Height()));
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

            Fonts::rainworld->write(line, -1.0, -1.0 + i*0.04 - 0.003, 0.03);
            Fonts::rainworld->write(line, -1.0, -1.0 + i*0.04 + 0.003, 0.03);
            Fonts::rainworld->write(line, -1.0 - 0.003, -1.0 + i*0.04, 0.03);
            Fonts::rainworld->write(line, -1.0 + 0.003, -1.0 + i*0.04, 0.03);
            i++;
        }
        
        i = 1;

        setThemeColour(THEME_TEXT_COLOUR);
        for (auto it = debugText.rbegin(); it != debugText.rend(); it++) {
            std::string line = *it;

            Fonts::rainworld->write(line, -1.0, -1.0 + i*0.04, 0.03);
            i++;
        }
    }
}