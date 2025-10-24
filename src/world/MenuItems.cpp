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
#include "droplet/ResizeLevelPopup.hpp"

std::vector<Button*> MenuItems::buttons;
std::vector<Button*> MenuItems::layerButtons;
int MenuItems::currentLayer = MENU_LAYER_FLOOD_FORGE;

double MenuItems::currentButtonX = 0.01;
double MenuItems::currentButtonXRight = 2 * 1 - 0.01; // Screen bound don't define when this is initialized, so just assume 1 for now.

double MenuItems::lastWindowWidth = 0.0;
double MenuItems::lastWindowHeight = 0.0;

Button::Button(std::string text, Rect rect, int layer, ButtonAlignment alignment) : rect(rect), text(text), layer(layer), alignment(alignment) {
	Text(text);
}

Button *Button::OnPress(std::function<void(Button*)> listener) {
	this->listener = listener;
	return this;
}

Button *Button::SetAlignment(ButtonAlignment align) {
	this->alignment = align;
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


Button &MenuItems::addButton(std::string text, int layer, ButtonAlignment alignment) {
    Button *button = nullptr;

    if(alignment == ButtonAlignment::LEFT) {
        button = new Button(text, Rect::fromSize(currentButtonX, -0.05, 0.0, 0.04), layer, alignment);
        currentButtonX = button->rect.x1 + 0.01;	
        buttons.push_back(button);
    } else 
	
	if(alignment == ButtonAlignment::RIGHT) {
		button = new Button(text, Rect::fromSize(currentButtonXRight, -0.05, 0.0, 0.04), layer, alignment);
		button->rect.x0 = currentButtonXRight - (button->rect.x1 - button->rect.x0);
		button->rect.x1 = currentButtonXRight;
        currentButtonXRight = button->rect.x0 - 0.01;	
        buttons.push_back(button);
    }

    return *button;
}

void MenuItems::addLayerButton(std::string buttonName, int worldLayer, int layer, ButtonAlignment alignment) {
	Button *btn = MenuItems::addButton(buttonName, layer, alignment)
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
						if (roomFilePath.extension().generic_u8string() != ".txt") {
							Popups::addPopup(new InfoPopup("File must start with .txt"));
							return;
						}
					}

					for (std::filesystem::path roomFilePath : paths) {
						std::string acronym = roomFilePath.parent_path().filename().generic_u8string();
						acronym = acronym.substr(0, acronym.find_last_of('-'));

						if (toLower(acronym) == "gates") {
							std::vector<std::string> names = split(roomFilePath.stem().generic_u8string(), '_');
							if (toLower(names[1]) == toLower(EditorState::region.acronym) || toLower(names[2]) == toLower(EditorState::region.acronym)) {
								Room *room = new Room(roomFilePath, roomFilePath.stem().generic_u8string());
								room->SetTag("GATE");
								room->canonPosition = EditorState::cameraOffset;
								room->devPosition = EditorState::cameraOffset;
								EditorState::rooms.push_back(room);
							} else {
								Popups::addPopup((new ConfirmPopup("Change which acronym?"))
								->OkayText(names[2])
								->OnOkay([names, roomFilePath]() {
									std::string roomPath = "gate_" + names[1] + "_" + EditorState::region.acronym + ".txt";

									copyRoom(roomFilePath, roomFilePath.parent_path() / roomPath)->SetTag("GATE");
								})
								->CancelText(names[1])
								->OnCancel([names, roomFilePath]() {
									std::string roomPath = "gate_" + EditorState::region.acronym + "_" + names[2] + ".txt";

									copyRoom(roomFilePath, roomFilePath.parent_path() / roomPath)->SetTag("GATE");
								}));
							}
						} else {
							if (compareInsensitive(acronym, EditorState::region.acronym) || EditorState::region.exportDirectory.empty()) {
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

	// Tutorial button
	addButton("?", MENU_LAYER_FLOOD_FORGE, ButtonAlignment::RIGHT)
		.OnPress([](Button *button) {
			Popups::addPopup(new MarkdownPopup(BASE_PATH / "docs" / "controls.md"));
		});

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

	addButton("Resize", MENU_LAYER_DROPLET).OnPress(
		[](Button *button) {
			Popups::addPopup(new ResizeLevelPopup());
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

void MenuItems::checkWindowResize() {
	double currentWidth = 2 * UI::screenBounds.x;
	double currentHeight = 2 * UI::screenBounds.y;

	if (lastWindowWidth != currentWidth || lastWindowHeight != currentHeight) {
		lastWindowWidth = currentWidth;
		lastWindowHeight = currentHeight;

		repositionButtons();
	}
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
	currentButtonXRight = 2 * UI::screenBounds.x - 0.01;

	for (Button *button : buttons) {
		if (button->layer != currentLayer) continue;
		
		if (button->alignment == ButtonAlignment::LEFT) {
			
			button->rect.x1 = button->rect.x1 - button->rect.x0 + currentButtonX;
			button->rect.x0 = currentButtonX;
			currentButtonX = button->rect.x1 + 0.01;
		} else 
		
		if (button->alignment == ButtonAlignment::RIGHT) {

			button->rect.x0 = currentButtonXRight - (button->rect.x1 - button->rect.x0);
			button->rect.x1 = currentButtonXRight;
			currentButtonXRight = button->rect.x0 - 0.01;
		}
	}
}