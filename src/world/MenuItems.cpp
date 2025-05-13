#include "MenuItems.hpp"

#include "../popup/Popups.hpp"
#include "../popup/FilesystemPopup.hpp"
#include "../popup/InfoPopup.hpp"
#include "../popup/ConfirmPopup.hpp"
#include "AcronymPopup.hpp"
#include "ChangeAcronymPopup.hpp"

std::vector<Button*> MenuItems::buttons;

Window *MenuItems::window = nullptr;

double MenuItems::currentButtonX = 0.01;

std::filesystem::path MenuItems::exportDirectory = "";
std::string MenuItems::worldAcronym = "";
std::string MenuItems::roomsDirectory = "";

std::string MenuItems::extraProperties = "";
std::string MenuItems::extraWorld = "";

GLuint MenuItems::textureButtonNormal = 0;
GLuint MenuItems::textureButtonNormalHover = 0;
GLuint MenuItems::textureButtonPress = 0;
GLuint MenuItems::textureButtonPressHover = 0;
GLuint MenuItems::textureBar = 0;

void Button::draw(Mouse *mouse, Vector2 screenBounds) {
	Draw::color(1.0, 1.0, 1.0);
	
	GLuint texture = 0;
	bool dark = darken || pressed;
	if (isHovered(mouse, screenBounds)) {
		texture = dark ? MenuItems::textureButtonPressHover : MenuItems::textureButtonNormalHover;
	} else {
		texture = dark ? MenuItems::textureButtonPress : MenuItems::textureButtonNormal;
	}

	Draw::useTexture(texture);

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

	addButton("New").OnLeftPress(
		[window](Button *button) {
			Popups::addPopup(new AcronymPopup(window));
		}
	);

	addButton("Add Room").OnLeftPress(
		[window](Button *button) {
			if (worldAcronym == "") {
				Popups::addPopup(new InfoPopup(window, "You must create or import a region\nbefore adding rooms."));
				return;
			}

			Popups::addPopup((new FilesystemPopup(window, std::regex("([^._-]+)_[a-zA-Z0-9]+\\.txt"), "xx_a01.txt",
				[&](std::set<std::string> pathStrings) {
					if (pathStrings.empty()) return;

					for (std::string pathString : pathStrings) {
						std::filesystem::path roomFilePath = pathString;
						std::string acronym = roomFilePath.parent_path().filename().string();
						acronym = acronym.substr(0, acronym.find_last_of('-'));

						if (acronym == worldAcronym || exportDirectory.empty()) {
							std::string roomName = roomFilePath.filename().string();
							roomName = roomName.substr(0, roomName.find_last_of('.'));
	
							Room *room = new Room(roomFilePath.string(), roomName);
							room->Position(cameraOffset);
							room->data.merge = Settings::getSetting<bool>(Settings::Setting::VisualMergeDefault);
							rooms.push_back(room);
						} else {
							Popups::addPopup((new ConfirmPopup(window, "Copy room to " + worldAcronym + "-rooms?"))
							->OnCancel([roomFilePath]() {
								std::string roomName = roomFilePath.filename().string();
								roomName = roomName.substr(0, roomName.find_last_of('.'));
		
								Room *room = new Room(roomFilePath.string(), roomName);
								room->Position(cameraOffset);
								room->data.merge = Settings::getSetting<bool>(Settings::Setting::VisualMergeDefault);
								rooms.push_back(room);
							})
							->OnOkay([roomFilePath, &window]() {
								std::filesystem::path fromDirectory = roomFilePath.parent_path();
								std::filesystem::path output = ((std::filesystem::path) exportDirectory.string().substr(0, exportDirectory.string().find_last_of(std::filesystem::path::preferred_separator))) / roomsDirectory;

								std::string roomName = roomFilePath.filename().string();
								roomName = roomName.substr(0, roomName.find_last_of('.'));
								std::string newRoomName = worldAcronym + roomName.substr(roomName.find_first_of('_'));

								std::filesystem::path newRoomPath = output / (newRoomName + ".txt");
								if (std::filesystem::exists(newRoomPath)) {
									Popups::addPopup(new InfoPopup(window, "Couldn't complete the copy!\nRoom already exists!"));
								} else {
									std::filesystem::copy_file(roomFilePath, newRoomPath);
									
									bool initial = Settings::getSetting<bool>(Settings::Setting::WarnMissingImages);
									Settings::settings[Settings::Setting::WarnMissingImages] = false;
									Room *room = new Room(roomFilePath.string(), newRoomName);
									room->Position(cameraOffset);
									room->data.merge = Settings::getSetting<bool>(Settings::Setting::VisualMergeDefault);
									rooms.push_back(room);
									Settings::settings[Settings::Setting::WarnMissingImages] = initial;
									
									for (int i = 0; i < room->Images(); i++) {
										std::string imagePath = roomName + "_" + std::to_string(i + 1) + ".png";
										std::string image = findFileCaseInsensitive(fromDirectory.string(), imagePath);
										
										if (image.empty()) {
											FailureController::fails.push_back("Can't find '" + imagePath + "'");
										} else {
											std::filesystem::copy_file(image, output / (newRoomName + "_" + std::to_string(i + 1) + ".png"));
										}
									}
	
									if (FailureController::fails.size() > 0) {
										std::string fails = "";
										for (std::string fail : FailureController::fails) {
											fails += fail + "\n";
										}
										Popups::addPopup(new InfoPopup(window, fails));
										FailureController::fails.clear();
									}
								}
							}));
						}
					}
				}
			))->AllowMultiple());
		}
	);

	addButton("Import").OnLeftPress(
		[window](Button *button) {
			Popups::addPopup(new FilesystemPopup(window, std::regex("world_([^._-]+)\\.txt", std::regex_constants::icase), "world_xx.txt",
				[window](std::set<std::string> pathStrings) {
					if (pathStrings.empty()) return;

					std::filesystem::path path = *pathStrings.begin();

					exportDirectory = path.parent_path();
					worldAcronym = toLower(path.filename().string());
					worldAcronym = worldAcronym.substr(worldAcronym.find_last_of('_') + 1, worldAcronym.find_last_of('.') - worldAcronym.find_last_of('_') - 1);

					std::cout << "Opening world " << worldAcronym << std::endl;

					std::filesystem::path roomsPath = findDirectoryCaseInsensitive(exportDirectory.parent_path().string(), worldAcronym + "-rooms");
					if (roomsPath.empty()) {
						roomsDirectory = "";
						FailureController::fails.push_back("Cannot find rooms directory!");
					} else {
						roomsDirectory = roomsPath.filename().string();
					}

					std::filesystem::path mapFilePath = findFileCaseInsensitive(exportDirectory.string(), "map_" + worldAcronym + ".txt");

					std::string propertiesFilePath = findFileCaseInsensitive(exportDirectory.string(), "properties.txt");

					if (std::find(rooms.begin(), rooms.end(), offscreenDen) != rooms.end()) {
						offscreenDen = nullptr;
					}

					for (Room *room : rooms) {
						delete room;
					}
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
						std::cout << "Loading map" << std::endl;

						parseMap(mapFilePath, exportDirectory);
					} else {
						std::cout << "Map file not found, loading world file" << std::endl;
					}

					std::cout << "Loading world" << std::endl;
					parseWorld(path, exportDirectory);
					
					std::cout << "Loading extra room data" << std::endl;
					
					for (Room *room : rooms) {
						if (room->isOffscreen()) continue;

						loadExtraRoomData(findFileCaseInsensitive((exportDirectory.parent_path() / roomsDirectory).string(), room->roomName + "_settings.txt"), room->data);
					}
					
					std::cout << "Extra room data - loaded" << std::endl;

					if (FailureController::fails.size() > 0) {
						std::string fails = "";
						for (std::string fail : FailureController::fails) {
							fails += fail + "\n";
						}
						Popups::addPopup(new InfoPopup(window, fails));
						FailureController::fails.clear();
					}
				}
			));
		}
	);

	addButton("Export").OnLeftPress(
		[window](Button *button) {
			if (exportDirectory.string().length() > 0) {
				exportMapFile();
				exportWorldFile();
				exportImageFile(exportDirectory / ("map_" + worldAcronym + ".png"), exportDirectory / ("map_" + worldAcronym + "_2.png"));
				exportPropertiesFile(exportDirectory / "properties.txt");
				Popups::addPopup(new InfoPopup(window, "Exported successfully!"));
			} else {
				if (worldAcronym == "") {
					Popups::addPopup(new InfoPopup(window, "You must create or import a region\nbefore exporting."));
					return;
				}

				Popups::addPopup(new FilesystemPopup(window, TYPE_FOLDER, "YOUR_MOD/world/xx/",
					[window](std::set<std::string> pathStrings) {
						if (pathStrings.empty()) return;

						exportDirectory = *pathStrings.begin();

						exportMapFile();
						exportWorldFile();
						exportImageFile(exportDirectory / ("map_" + worldAcronym + ".png"), exportDirectory / ("map_" + worldAcronym + "_2.png"));
						exportPropertiesFile(exportDirectory / "properties.txt");
						Popups::addPopup(new InfoPopup(window, "Exported successfully!"));
					}
				));
			}
		}
	);

	addButton("No Colours").OnLeftPress(
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

	addButton("1")
	.OnLeftPress(
		[window](Button *button) {
			visibleLayers[LAYER_1] = !visibleLayers[LAYER_1];
			button->Darken(!visibleLayers[LAYER_1]);
		}
	)
	->OnRightPress(
		[window](Button *button) {
			// TODO LATER
			// visibleLayers[0] = true; visibleLayers[1] = true; visibleLayers[2] = true;
		}
	);

	addButton("2")
	.OnLeftPress(
		[window](Button *button) {
			visibleLayers[LAYER_2] = !visibleLayers[LAYER_2];
			button->Darken(!visibleLayers[LAYER_2]);
		}
	)
	->OnRightPress(
		[window](Button *button) {
			// TODO LATER
			// visibleLayers[0] = true; visibleLayers[1] = true; visibleLayers[2] = true;
		}
	);

	addButton("3")
	.OnLeftPress(
		[window](Button *button) {
			visibleLayers[LAYER_3] = !visibleLayers[LAYER_3];
			button->Darken(!visibleLayers[LAYER_3]);
		}
	)
	->OnRightPress(
		[window](Button *button) {
			// TODO LATER
			// visibleLayers[0] = true; visibleLayers[1] = true; visibleLayers[2] = true;
		}
	);

	addButton("Dev Items: Hidden")
	.OnLeftPress(
		[window](Button *button) {
			visibleDevItems = !visibleDevItems;
			button->Text(visibleDevItems ? "Dev Items: Shown" : "Dev Items: Hidden");
		}
	);

	// addButton("Change Region Acronym").OnLeftPress(
	//     [window](Button *button) {
	//         if (worldAcronym == "") {
	//             Popups::addPopup(new InfoPopup(window, "You must create or import a region\nbefore changing the acronym."));
	//             return;
	//         }

	//         Popups::addPopup(new ChangeAcronymPopup(window));
	//     }
	// );

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