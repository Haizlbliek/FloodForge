#include "MenuItems.hpp"

#include "../popup/Popups.hpp"
#include "../popup/FilesystemPopup.hpp"
#include "../popup/InfoPopup.hpp"
#include "../popup/ConfirmPopup.hpp"
#include "popup/AcronymPopup.hpp"
#include "popup/ChangeAcronymPopup.hpp"

#include "../ui/UI.hpp"

#include "RecentFiles.hpp"
#include "WorldParser.hpp"
#include "WorldExporter.hpp"

#include "droplet/DropletWindow.hpp"

std::vector<Button*> MenuItems::buttons;
std::vector<Button*> MenuItems::layerButtons;
int MenuItems::currentLayer = MENU_LAYER_FLOOD_FORGE;

double MenuItems::currentButtonX = 0.01;

Button::Button(std::string text, Rect rect, int layer) : rect(rect), text(text), layer(layer) {
	Text(text);
}

Button *Button::OnPress(std::function<void(Button*)> listener) {
	this->listener = listener;
	return this;
}

void Button::draw() {
	UI::TextButtonMods mods = UI::TextButtonMods();
	if (darken) mods.TextColor(currentTheme[ThemeColour::TextDisabled]);

	if (UI::TextButton(Rect(rect.x0 - UI::screenBounds.x, rect.y0 + UI::screenBounds.y, rect.x1 - UI::screenBounds.x, rect.y1 + UI::screenBounds.y), text, mods)) {
		listener(this);
	}
}

void Button::Text(const std::string text) {
	this->text = text;
	rect = Rect::fromSize(rect.x0, rect.y0, Fonts::rainworld->getTextWidth(text, 0.03) + 0.02, rect.y1 - rect.y0);
}

std::string Button::Text() const { return text; }


Room *copyRoom(std::filesystem::path fromFile, std::filesystem::path toFile) {
	std::string fromRoom = fromFile.stem().generic_u8string();
	std::string toRoom = toFile.stem().generic_u8string();

	if (std::filesystem::exists(toFile)) {
		return nullptr;
		// Popups::addPopup(new InfoPopup(window, "Couldn't complete the copy!\nRoom already exists!"));
	} else {
		std::filesystem::copy_file(fromFile, toFile);

		bool initial = Settings::getSetting<bool>(Settings::Setting::WarnMissingImages);
		Settings::settings[Settings::Setting::WarnMissingImages] = false;
		Room *room = new Room(fromFile, toRoom);
		room->canonPosition = EditorState::cameraOffset;
		room->devPosition = EditorState::cameraOffset;
		EditorState::rooms.push_back(room);
		Settings::settings[Settings::Setting::WarnMissingImages] = initial;

		for (int i = 0; i < room->cameras; i++) {
			std::string imagePath = fromRoom + "_" + std::to_string(i + 1) + ".png";
			std::filesystem::path image = findFileCaseInsensitive(fromFile.parent_path(), imagePath);

			if (image.empty()) {
				EditorState::fails.push_back("Can't find '" + imagePath + "'");
			} else {
				std::filesystem::copy_file(image, toFile.parent_path() / (toRoom + "_" + std::to_string(i + 1) + ".png"));
			}
		}

		return room;
	}
}


Button &MenuItems::addButton(std::string text, int layer) {
	Button *button = new Button(text, Rect::fromSize(currentButtonX, -0.05, 0.0, 0.04), layer);
	currentButtonX = button->rect.x1 + 0.01;
	buttons.push_back(button);
	return *button;
}

void MenuItems::addLayerButton(std::string buttonName, int worldLayer, int layer) {
	Button *btn = MenuItems::addButton(buttonName, layer)
	.OnPress(
		[worldLayer](Button *button) {
			if (UI::window->modifierPressed(GLFW_MOD_SHIFT)) {
				bool alreadySolo = true;
				for (int i = 0; i < LAYER_COUNT; i++) {
					if (EditorState::visibleLayers[i] != (i == worldLayer)) {
						alreadySolo = false;
						break;
					}
				}

				for (int i = 0; i < LAYER_COUNT; i++) {
					if (i == worldLayer) {
						EditorState::visibleLayers[i] = true;
					} else {
						EditorState::visibleLayers[i] = alreadySolo;
					}
					MenuItems::layerButtons[i]->darken = !EditorState::visibleLayers[i];
				}
			} else {
				EditorState::visibleLayers[worldLayer] = !EditorState::visibleLayers[worldLayer];
				button->darken = !EditorState::visibleLayers[worldLayer];
			}
		}
	);

	MenuItems::layerButtons.push_back(btn);
}

void MenuItems::init() {
	EditorState::region.acronym = "";
	currentLayer = MENU_LAYER_FLOOD_FORGE;

	initFloodForge();
	initDroplet();
}

void MenuItems::initFloodForge() {
	addButton("New", MENU_LAYER_FLOOD_FORGE).OnPress(
		[](Button *button) {
			Popups::addPopup(new AcronymPopup());
		}
	);

	addButton("Add Room", MENU_LAYER_FLOOD_FORGE).OnPress(
		[](Button *button) {
			if (EditorState::region.acronym == "") {
				Popups::addPopup(new InfoPopup("You must create or import a region\nbefore adding rooms."));
				return;
			}

			Popups::addPopup((new FilesystemPopup(std::regex("((?!.*_settings)(?=.+_.+).+\\.txt)|(gate_([^._-]+)_([^._-]+)\\.txt)"), "xx_a01.txt",
				[&](std::set<std::filesystem::path> paths) {
					if (paths.empty()) return;

					for (std::filesystem::path roomFilePath : paths) {
						std::string acronym = roomFilePath.parent_path().filename().generic_u8string();
						acronym = acronym.substr(0, acronym.find_last_of('-'));

						if (acronym == "gates") {
							std::vector<std::string> names = split(roomFilePath.filename().generic_u8string(), '_');
							names[2] = names[2].substr(0, names[2].find('.'));
							if (toLower(names[1]) == toLower(EditorState::region.acronym) || toLower(names[2]) == toLower(EditorState::region.acronym)) {
								std::string roomName = names[0].substr(0, names[0].find_last_of('.')); // Remove .txt

								Room *room = new Room(roomFilePath, roomName);
								room->canonPosition = EditorState::cameraOffset;
								room->devPosition = EditorState::cameraOffset;
								EditorState::rooms.push_back(room);
							} else {
								Popups::addPopup((new ConfirmPopup("Change which acronym?"))
								->OkayText(names[2])
								->OnOkay([names, roomFilePath]() {
									std::string roomPath = "gate_" + EditorState::region.acronym + "_" + names[1] + ".txt";

									copyRoom(roomFilePath, roomFilePath.parent_path() / roomPath)->SetTag("GATE");
								})
								->CancelText(names[1])
								->OnCancel([names, roomFilePath]() {
									std::string roomPath = "gate_" + names[2] + "_" + EditorState::region.acronym + ".txt";

									copyRoom(roomFilePath, roomFilePath.parent_path() / roomPath)->SetTag("GATE");
								}));
							}
						} else {
							if (acronym == EditorState::region.acronym || EditorState::region.exportDirectory.empty()) {
								std::string roomName = roomFilePath.stem().generic_u8string();

								Room *room = new Room(roomFilePath, roomName);
								room->canonPosition = EditorState::cameraOffset;
								room->devPosition = EditorState::cameraOffset;
								EditorState::rooms.push_back(room);
							} else {
								Popups::addPopup((new ConfirmPopup("Copy room to " + EditorState::region.acronym + "-rooms?"))
								->CancelText("Just Add")
								->OnCancel([roomFilePath]() {
									std::string roomName = roomFilePath.stem().generic_u8string();

									Room *room = new Room(roomFilePath, roomName);
									room->canonPosition = EditorState::cameraOffset;
									room->devPosition = EditorState::cameraOffset;
									EditorState::rooms.push_back(room);
								})
								->OkayText("Yes")
								->OnOkay([roomFilePath]() {
									std::string roomPath = roomFilePath.filename().generic_u8string();
									roomPath = EditorState::region.acronym + roomPath.substr(roomPath.find('_'));

									copyRoom(roomFilePath, EditorState::region.roomsDirectory / roomPath);
								}));
							}
						}
					}
				}
			))->AllowMultiple());
		}
	);

	addButton("Import", MENU_LAYER_FLOOD_FORGE).OnPress(
		[](Button *button) {
			Popups::addPopup(new FilesystemPopup(std::regex("world_([^._-]+)\\.txt", std::regex_constants::icase), "world_xx.txt",
				[](std::set<std::filesystem::path> paths) {
					if (paths.empty()) return;

					WorldParser::importWorldFile(*paths.begin());
				}
			));
		}
	);

	addButton("Export", MENU_LAYER_FLOOD_FORGE).OnPress(
		[](Button *button) {
			std::filesystem::path lastExportDirectory = EditorState::region.exportDirectory;

			if (!Settings::getSetting<bool>(Settings::Setting::UpdateWorldFiles)) {
				EditorState::region.exportDirectory = BASE_PATH / "worlds" / EditorState::region.acronym;
				Logger::info("Special exporting to directory: ", EditorState::region.exportDirectory.generic_u8string());
				if (!std::filesystem::exists(EditorState::region.exportDirectory)) {
					std::filesystem::create_directories(EditorState::region.exportDirectory);
				}
			}

			if (!EditorState::region.exportDirectory.empty()) {
				WorldExporter::exportMapFile();
				WorldExporter::exportWorldFile();
				WorldExporter::exportImageFile(EditorState::region.exportDirectory / ("map_" + EditorState::region.acronym + ".png"), EditorState::region.exportDirectory / ("map_" + EditorState::region.acronym + "_2.png"));
				WorldExporter::exportPropertiesFile(EditorState::region.exportDirectory / "properties.txt");
				Popups::addPopup(new InfoPopup("Exported successfully!"));
			} else {
				if (EditorState::region.acronym == "") {
					Popups::addPopup(new InfoPopup("You must create or import a region\nbefore exporting."));
					return;
				}

				Popups::addPopup(new FilesystemPopup(FilesystemPopup::FilesystemType::FOLDER, "YOUR_MOD/world/",
					[](std::set<std::filesystem::path> pathStrings) {
						if (pathStrings.empty()) return;

						EditorState::region.exportDirectory = *pathStrings.begin() / EditorState::region.acronym;
						EditorState::region.roomsDirectory = *pathStrings.begin() / (EditorState::region.acronym + "-rooms");
						std::filesystem::create_directories(EditorState::region.exportDirectory);
						std::filesystem::create_directories(EditorState::region.roomsDirectory);

						WorldExporter::exportMapFile();
						WorldExporter::exportWorldFile();
						WorldExporter::exportImageFile(EditorState::region.exportDirectory / ("map_" + EditorState::region.acronym + ".png"), EditorState::region.exportDirectory / ("map_" + EditorState::region.acronym + "_2.png"));
						WorldExporter::exportPropertiesFile(EditorState::region.exportDirectory / "properties.txt");
						Popups::addPopup(new InfoPopup("Exported successfully!"));
					}
				));
			}

			EditorState::region.exportDirectory = lastExportDirectory;
		}
	);

	addButton("No Colours", MENU_LAYER_FLOOD_FORGE).OnPress(
		[](Button *button) {
			EditorState::roomColours = (EditorState::roomColours + 1) % 3;

			if (EditorState::roomColours == 0) {
				button->Text("No Colours");
			} else if (EditorState::roomColours == 1) {
				button->Text("Layer Colours");
			} else {
				button->Text("Subregion Colours");
			}

			repositionButtons();
		}
	);

	for (int i = 0; i < LAYER_COUNT; i++) {
		addLayerButton(std::to_string(i + 1), i, MENU_LAYER_FLOOD_FORGE);
	}

	addButton("Dev Items: Hidden", MENU_LAYER_FLOOD_FORGE).OnPress(
		[](Button *button) {
			EditorState::visibleDevItems = !EditorState::visibleDevItems;
			button->Text(EditorState::visibleDevItems ? "Dev Items: Shown" : "Dev Items: Hidden");
		}
	);

	addButton("Refresh Region", MENU_LAYER_FLOOD_FORGE).OnPress(
		[](Button *button) {
			if (EditorState::region.acronym.empty() || EditorState::region.exportDirectory.empty()) {
				Popups::addPopup(new InfoPopup("You must create or import a region\nbefore refreshing"));
				return;
			}

			WorldParser::importWorldFile(findFileCaseInsensitive(EditorState::region.exportDirectory, "world_" + EditorState::region.acronym + ".txt"));
		}
	);

	addButton("Canon", MENU_LAYER_FLOOD_FORGE).OnPress(
		[](Button *button) {
			if (EditorState::positionType == PositionType::CANON) {
				EditorState::positionType = PositionType::DEV;
				button->Text("Dev");
			}
			else if (EditorState::positionType == PositionType::DEV) {
				EditorState::positionType = PositionType::BOTH;
				button->Text("Both");
			}
			else {
				EditorState::positionType = PositionType::CANON;
				button->Text("Canon");
			}
		}
	);

	// addButton("Change Region Acronym").OnPress(
	// 	[window](Button *button) {
	// 		if (EditorState::region.acronym == "") {
	// 			Popups::addPopup(new InfoPopup(window, "You must create or import a region\nbefore changing the acronym."));
	// 			return;
	// 		}

	// 		Popups::addPopup(new ChangeAcronymPopup(window));
	// 	}
	// );
}

void MenuItems::initDroplet() {
	addButton("Export Geometry", MENU_LAYER_DROPLET).OnPress(
		[](Button *button) {
			DropletWindow::exportGeometry();
		}
	);

	addButton("Render", MENU_LAYER_DROPLET).OnPress(
		[](Button *button) {
			DropletWindow::render();
		}
	);

	addButton("Export Leditor Project", MENU_LAYER_DROPLET).OnPress(
		[](Button *button) {
			Popups::addPopup(new FilesystemPopup(FilesystemPopup::FilesystemType::FOLDER, "Data/LevelEditorProjects", [](std::set<std::filesystem::path> paths) {
				if (paths.size() != 1) return;

				DropletWindow::exportProject(*paths.begin());
			}));
		}
	);

	addButton("Show objects", MENU_LAYER_DROPLET).OnPress(
		[](Button *button) {
			DropletWindow::showObjects = !DropletWindow::showObjects;
			button->Text(DropletWindow::showObjects ? "Hide objects" : "Show objects");
		}
	);
}

void MenuItems::cleanup() {
	for (Button *button : buttons) {
		delete button;
	}

	buttons.clear();
}

void MenuItems::setLayer(int layer) {
	if (currentLayer == layer) return;

	currentLayer = layer;
	repositionButtons();
}

void MenuItems::draw() {
	glLineWidth(1);

	Rect rect = Rect(-UI::screenBounds.x, UI::screenBounds.y, UI::screenBounds.x, UI::screenBounds.y - 0.06);

	setThemeColor(ThemeColour::Popup);
	fillRect(rect);

	setThemeColor(ThemeColour::Border);
	Draw::begin(Draw::LINES);
	Draw::vertex(rect.x0, rect.y0);
	Draw::vertex(rect.x1, rect.y0);
	Draw::end();

	for (Button *button : buttons) {
		if (button->layer != currentLayer) continue;

		button->draw();
	}
}

void MenuItems::repositionButtons() {
	currentButtonX = 0.01;

	for (Button *button : buttons) {
		if (button->layer != currentLayer) continue;

		button->rect.x1 = button->rect.x1 - button->rect.x0 + currentButtonX;
		button->rect.x0 = currentButtonX;

		currentButtonX = button->rect.x1 + 0.01;
	}
}