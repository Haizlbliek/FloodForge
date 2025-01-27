#include "MenuItems.hpp"

#include "../popup/Popups.hpp"
#include "../popup/FilesystemPopup.hpp"
#include "../popup/WarningPopup.hpp"
#include "AcronymPopup.hpp"

std::vector<Button*> MenuItems::buttons;

Window *MenuItems::window = nullptr;

double MenuItems::currentButtonX = 0.01;

std::filesystem::path MenuItems::exportDirectory = "";
std::string MenuItems::worldAcronym = "";

std::string MenuItems::extraProperties = "";
std::string MenuItems::extraWorld = "";

GLuint MenuItems::textureButtonNormal = 0;
GLuint MenuItems::textureButtonNormalHover = 0;
GLuint MenuItems::textureButtonPress = 0;
GLuint MenuItems::textureButtonPressHover = 0;
GLuint MenuItems::textureBar = 0;

void Button::draw(Mouse *mouse, Vector2 screenBounds) {
    Draw::color(1.0, 1.0, 1.0);
    
    Draw::useTexture(isHovered(mouse, screenBounds) ? (pressed ? MenuItems::textureButtonPressHover : MenuItems::textureButtonNormalHover) : (pressed ? MenuItems::textureButtonNormalHover : MenuItems::textureButtonNormal));

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    nineSlice(x - 0.005 - screenBounds.x, y + 0.005 + screenBounds.y, x + 0.005 + width - screenBounds.x, y - 0.005 - height + screenBounds.y, 0.02);
    glDisable(GL_BLEND);
    Draw::useTexture(0);

    setThemeColour(ThemeColour::Text);
    font->writeCentred(text, x - screenBounds.x + (width * 0.5), y + height * -0.5 + screenBounds.y + 0.003, height - 0.01, CENTRE_XY);
}

void MenuItems::init(Window *window) {
    MenuItems::window = window;
    MenuItems::loadTextures();
    worldAcronym = "";

    addButton("New",
        [window](Button *button) {
            Popups::addPopup(new AcronymPopup(window));
        }
    );

    addButton("Add Room",
        [window](Button *button) {
            if (worldAcronym == "") {
                Popups::addPopup(new WarningPopup(window, "You must create or import a region\nbefore adding rooms."));
                return;
            }

            Popups::addPopup((new FilesystemPopup(window, std::regex("([^.]+)_[a-zA-Z0-9]+\\.txt"),
                [](std::set<std::string> pathStrings) {
                    if (pathStrings.empty()) return;

                    for (std::string pathString : pathStrings) {
                        std::filesystem::path path = pathString;

                        std::string roomName = path.filename().string();
                        roomName = roomName.substr(0, roomName.find_last_of('.'));

                        Room *room = new Room(path.string().substr(0, path.string().find_last_of('.')), roomName);
                        room->Position(cameraOffset);
                        rooms.push_back(room);
                    }
                }
            ))->AllowMultiple());
        }
    );

    addButton("Import",
        [window](Button *button) {
            Popups::addPopup(new FilesystemPopup(window, std::regex("world_([^.]+)\\.txt"),
                [](std::set<std::string> pathStrings) {
                    if (pathStrings.empty()) return;

                    std::filesystem::path path = *pathStrings.begin();

                    exportDirectory = path.parent_path();
                    worldAcronym = toLower(path.filename().string());
                    worldAcronym = worldAcronym.substr(worldAcronym.find_last_of('_') + 1, worldAcronym.find_last_of('.') - worldAcronym.find_last_of('_') - 1);

                    std::cout << "Opening world " << worldAcronym << std::endl;

                    std::filesystem::path mapFilePath = findFileCaseInsensitive(exportDirectory.string(), "map_" + worldAcronym + ".txt");

                    std::string propertiesFilePath = findFileCaseInsensitive(exportDirectory.string(), "properties.txt");

                    for (Room *room : rooms) delete room;
                    rooms.clear();
                    for (Connection *connection : connections) delete connection;
                    connections.clear();
                    subregions.clear();
                    if (offscreenDen != nullptr) delete offscreenDen;
                    offscreenDen = nullptr;
                    extraProperties = "";
                    extraWorld = "";

                    if (std::filesystem::exists(propertiesFilePath)) {
                        std::cout << "Found properties file, loading subregions" << std::endl;

                        parseProperties(propertiesFilePath);
                    }

                    if (std::filesystem::exists(mapFilePath)) {
                        parseMap(mapFilePath, exportDirectory);
                    } else {
                        std::cout << "Map file not found, loading world file" << std::endl;
                    }

                    parseWorld(path, exportDirectory);
                }
            ));
        }
    );

    addButton("Export",
        [window](Button *button) {
            if (exportDirectory.string().length() > 0) {
                exportMapFile();
                exportWorldFile();
                exportImageFile(exportDirectory / ("map_" + worldAcronym + ".png"), exportDirectory / ("map_" + worldAcronym + "_2.png"));
                exportPropertiesFile(exportDirectory / "properties.txt");
                Popups::addPopup(new WarningPopup(window, "Exported successfully!"));
            } else {
                if (worldAcronym == "") {
                    Popups::addPopup(new WarningPopup(window, "You must create or import a region\nbefore exporting."));
                    return;
                }

                Popups::addPopup(new FilesystemPopup(window, TYPE_FOLDER,
                    [window](std::set<std::string> pathStrings) {
                        if (pathStrings.empty()) return;

                        exportDirectory = *pathStrings.begin();

                        exportMapFile();
                        exportWorldFile();
                        exportImageFile(exportDirectory / ("map_" + worldAcronym + ".png"), exportDirectory / ("map_" + worldAcronym + "_2.png"));
                        exportPropertiesFile(exportDirectory / "properties.txt");
                        Popups::addPopup(new WarningPopup(window, "Exported successfully!"));
                    }
                ));
            }
        }
    );

    addButton("No Colours",
        [window](Button *button) {
            ::roomColours = (::roomColours + 1) % 3;

            if (::roomColours == 0) {
                button->Text("No Colours");
            } else if (::roomColours == 1) {
                button->Text("Layer Colours");
            } else {
                button->Text("Subregion Colours");
            }

            repositionButtons();
        }
    );

    // addButton("Tile Snap",
    //     [window](Button *button) {
    //         if (::roomSnap == ROOM_SNAP_NONE) {
    //             ::roomSnap = ROOM_SNAP_TILE;
    //             button->Text("Tile Snap");
    //         } else {
    //             ::roomSnap = ROOM_SNAP_NONE;
    //             button->Text("No Snap");
    //         }
            
    //         repositionButtons();
    //     }
    // );
}