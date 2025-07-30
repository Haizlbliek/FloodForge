#include "MenuItems.hpp"

#include "../popup/Popups.hpp"
#include "../popup/FilesystemPopup.hpp"
#include "../popup/InfoPopup.hpp"
#include "../popup/ConfirmPopup.hpp"
#include "AcronymPopup.hpp"
#include "ChangeAcronymPopup.hpp"

#include "RecentFiles.hpp"
#include "WorldParser.hpp"
#include "WorldExporter.hpp"

std::vector<Button*> MenuItems::buttons;
std::vector<Button*> MenuItems::layerButtons;

double MenuItems::currentButtonX = 0.01;

GLuint MenuItems::textureButtonNormal = 0;
GLuint MenuItems::textureButtonNormalHover = 0;
GLuint MenuItems::textureButtonPress = 0;
GLuint MenuItems::textureButtonPressHover = 0;
GLuint MenuItems::textureBar = 0;

Button::Button(std::string text, double x, double y, double width, double height, Font *font) : x(x), y(y), width(width), height(height), text(text), font(font) {
}

Button *Button::OnLeftPress(std::function<void(Button*)> listener) {
	listenersLeft.push_back(listener);
	return this;
}

Button *Button::OnRightPress(std::function<void(Button*)> listener) {
	listenersRight.push_back(listener);
	return this;
}

bool Button::isHovered(Mouse *mouse, Vector2 screenBounds) {
	double mouseX = mouse->X() / 512.0 + screenBounds.x - 1.0;
	double mouseY = -(mouse->Y() / 512.0 + screenBounds.y - 1.0);

	return mouseX >= (x - 0.005) && mouseX <= (x + 0.005) + width && mouseY <= (y + 0.005) && mouseY >= (y - 0.005) - height;
}

void Button::update(Mouse *mouse, Vector2 screenBounds) {
	pressed = isHovered(mouse, screenBounds) && (mouse->Left() || mouse->Right());

	if (isHovered(mouse, screenBounds) && mouse->JustLeft()) {
		press();
	}

	if (isHovered(mouse, screenBounds) && mouse->JustRight()) {
		pressRight();
	}
}

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

void Button::Text(const std::string text) {
	this->text = text;
	width = font->getTextWidth(text, height - 0.01) + 0.02;
}

std::string Button::Text() const { return text; }

void Button::X(const double x) {
	this->x = x;
}

Button &Button::Darken(bool newDarken) {
	darken = newDarken;

	return *this;
}

const double Button::Width() const {
	return width;
}

void Button::press() {
	for (const auto &listener : listenersLeft) {
		listener(this);
	}
}

void Button::pressRight() {
	for (const auto &listener : listenersRight) {
		listener(this);
	}
}


Room *copyRoom(std::filesystem::path fromFile, std::filesystem::path toFile) {
	std::string fromRoom = fromFile.filename().string();
	fromRoom = fromRoom.substr(0, fromRoom.find_last_of('.'));
	std::string toRoom = toFile.filename().string();
	toRoom = toRoom.substr(0, toRoom.find_last_of('.'));

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
		
		for (int i = 0; i < room->Images(); i++) {
			std::string imagePath = fromRoom + "_" + std::to_string(i + 1) + ".png";
			std::string image = findFileCaseInsensitive(fromFile.parent_path().string(), imagePath);
			
			if (image.empty()) {
				EditorState::fails.push_back("Can't find '" + imagePath + "'");
			} else {
				std::filesystem::copy_file(image, toFile.parent_path() / (toRoom + "_" + std::to_string(i + 1) + ".png"));
			}
		}

		return room;
	}
}



void MenuItems::loadTextures() {
	std::filesystem::path texturePath = BASE_PATH / "assets" / "themes" / currentThemeName;

	textureButtonNormal = loadTexture((texturePath / "ButtonNormal.png").string());
	textureButtonNormalHover = loadTexture((texturePath / "ButtonNormalHover.png").string());
	textureButtonPress = loadTexture((texturePath / "ButtonPress.png").string());
	textureButtonPressHover = loadTexture((texturePath / "ButtonPressHover.png").string());
	textureBar = loadTexture((texturePath / "Bar.png").string());
}

Button &MenuItems::addButton(std::string text) {
	double x = currentButtonX;
	double width = Fonts::rainworld->getTextWidth(text, 0.03) + 0.02;

	currentButtonX += width + 0.04;
	Button *button = new Button(text, x, -0.01, width, 0.04, Fonts::rainworld);
	buttons.push_back(button);

	return *button;
}

void MenuItems::addLayerButton(std::string buttonName, int layer) {
	Button *btn = MenuItems::addButton(buttonName)
	.OnLeftPress(
		[layer](Button *button) {
			EditorState::visibleLayers[layer] = !EditorState::visibleLayers[layer];
			button->Darken(!EditorState::visibleLayers[layer]);
		}
	)
	->OnRightPress(
		[layer](Button *button) {
			bool alreadySolo = true;
			for (int i = 0; i < LAYER_COUNT; i++) {
				if (EditorState::visibleLayers[i] != (i == layer)) {
					alreadySolo = false;
					break;
				}
			}
			
			for (int i = 0; i < LAYER_COUNT; i++) {
				if (i == layer) {
					EditorState::visibleLayers[i] = true;
				} else {
					EditorState::visibleLayers[i] = alreadySolo;
				}
				MenuItems::layerButtons[i]->Darken(!EditorState::visibleLayers[i]);
			}
		}
	);
	
	MenuItems::layerButtons.push_back(btn);
}

void MenuItems::init(Window *window) {
	MenuItems::loadTextures();
	EditorState::region.acronym = "";

	addButton("New").OnLeftPress(
		[window](Button *button) {
			Popups::addPopup(new AcronymPopup(window));
		}
	);

	addButton("Add Room").OnLeftPress(
		[window](Button *button) {
			if (EditorState::region.acronym == "") {
				Popups::addPopup(new InfoPopup(window, "You must create or import a region\nbefore adding rooms."));
				return;
			}

			Popups::addPopup((new FilesystemPopup(window, std::regex("(([^._-]+)_[a-zA-Z0-9]+\\.txt)|(gate_([^._-]+)_([^._-]+)\\.txt)"), "xx_a01.txt",
				[&](std::set<std::string> pathStrings) {
					if (pathStrings.empty()) return;

					for (std::string pathString : pathStrings) {
						std::filesystem::path roomFilePath = pathString;
						std::string acronym = roomFilePath.parent_path().filename().string();
						acronym = acronym.substr(0, acronym.find_last_of('-'));
						
						if (acronym == "gates") {
							std::vector<std::string> names = split(roomFilePath.filename().string(), '_');
							names[2] = names[2].substr(0, names[2].find('.'));
							if (toLower(names[1]) == EditorState::region.acronym || toLower(names[2]) == EditorState::region.acronym) {
								std::string roomName = names[0].substr(0, names[0].find_last_of('.')); // Remove .txt
		
								Room *room = new Room(roomFilePath, roomName);
								room->canonPosition = EditorState::cameraOffset;
								room->devPosition = EditorState::cameraOffset;
								EditorState::rooms.push_back(room);
							} else {
								Popups::addPopup((new ConfirmPopup(window, "Change which acronym?"))
								->OkayText(toUpper(names[2]))
								->OnOkay([names, roomFilePath, &window]() {
									std::string roomPath = "gate_" + EditorState::region.acronym + "_" + names[1] + ".txt";

									copyRoom(roomFilePath, roomFilePath.parent_path() / roomPath)->SetTag("GATE");
								})
								->CancelText(toUpper(names[1]))
								->OnCancel([names, roomFilePath, &window]() {
									std::string roomPath = "gate_" + names[2] + "_" + EditorState::region.acronym + ".txt";

									copyRoom(roomFilePath, roomFilePath.parent_path() / roomPath)->SetTag("GATE");
								}));
							}
						} else {
							if (acronym == EditorState::region.acronym || EditorState::region.exportDirectory.empty()) {
								std::string roomName = roomFilePath.filename().string();
								roomName = roomName.substr(0, roomName.find_last_of('.')); // Remove .txt
		
								Room *room = new Room(roomFilePath, roomName);
								room->canonPosition = EditorState::cameraOffset;
								room->devPosition = EditorState::cameraOffset;
								EditorState::rooms.push_back(room);
							} else {
								Popups::addPopup((new ConfirmPopup(window, "Copy room to " + EditorState::region.acronym + "-rooms?"))
								->CancelText("Just Add")
								->OnCancel([roomFilePath]() {
									std::string roomName = roomFilePath.filename().string();
									roomName = roomName.substr(0, roomName.find_last_of('.')); // Remove .txt
			
									Room *room = new Room(roomFilePath, roomName);
									room->canonPosition = EditorState::cameraOffset;
									room->devPosition = EditorState::cameraOffset;
									EditorState::rooms.push_back(room);
								})
								->OkayText("Yes")
								->OnOkay([roomFilePath, &window]() {
									std::string roomPath = roomFilePath.filename().string();
									roomPath = EditorState::region.acronym + roomPath.substr(roomPath.find('_'));

									std::filesystem::path output = EditorState::region.exportDirectory;
									output = output.parent_path() / EditorState::region.roomsDirectory;
	
									copyRoom(roomFilePath, output / roomPath);
								}));
							}
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

					WorldParser::importWorldFile(path);
				}
			));
		}
	);

	addButton("Export").OnLeftPress(
		[window](Button *button) {
			std::filesystem::path lastExportDirectory = EditorState::region.exportDirectory;

			if (!Settings::getSetting<bool>(Settings::Setting::UpdateWorldFiles)) {
				EditorState::region.exportDirectory = BASE_PATH / "worlds" / EditorState::region.acronym;
				Logger::log("Special exporting to directory: ", EditorState::region.exportDirectory.string());
				if (!std::filesystem::exists(EditorState::region.exportDirectory)) {
					std::filesystem::create_directories(EditorState::region.exportDirectory);
				}
			}

			if (EditorState::region.exportDirectory.string().length() > 0) {
				WorldExporter::exportMapFile();
				WorldExporter::exportWorldFile();
				WorldExporter::exportImageFile(EditorState::region.exportDirectory / ("map_" + EditorState::region.acronym + ".png"), EditorState::region.exportDirectory / ("map_" + EditorState::region.acronym + "_2.png"));
				WorldExporter::exportPropertiesFile(EditorState::region.exportDirectory / "properties.txt");
				Popups::addPopup(new InfoPopup(window, "Exported successfully!"));
			} else {
				if (EditorState::region.acronym == "") {
					Popups::addPopup(new InfoPopup(window, "You must create or import a region\nbefore exporting."));
					return;
				}

				Popups::addPopup(new FilesystemPopup(window, TYPE_FOLDER, "YOUR_MOD/world/xx/",
					[window](std::set<std::string> pathStrings) {
						if (pathStrings.empty()) return;

						EditorState::region.exportDirectory = *pathStrings.begin();

						WorldExporter::exportMapFile();
						WorldExporter::exportWorldFile();
						WorldExporter::exportImageFile(EditorState::region.exportDirectory / ("map_" + EditorState::region.acronym + ".png"), EditorState::region.exportDirectory / ("map_" + EditorState::region.acronym + "_2.png"));
						WorldExporter::exportPropertiesFile(EditorState::region.exportDirectory / "properties.txt");
						Popups::addPopup(new InfoPopup(window, "Exported successfully!"));
					}
				));
			}
			
			EditorState::region.exportDirectory = lastExportDirectory;
		}
	);

	addButton("No Colours").OnLeftPress(
		[window](Button *button) {
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
		addLayerButton(std::to_string(i + 1), i);
	}

	addButton("Dev Items: Hidden")
	.OnLeftPress(
		[window](Button *button) {
			EditorState::visibleDevItems = !EditorState::visibleDevItems;
			button->Text(EditorState::visibleDevItems ? "Dev Items: Shown" : "Dev Items: Hidden");
		}
	);

	addButton("Refresh Region").OnLeftPress(
		[window](Button *button) {
			if (EditorState::region.acronym.empty() || EditorState::region.exportDirectory.empty()) {
				Popups::addPopup(new InfoPopup(window, "You must create or import a region\nbefore refreshing"));
				return;
			}
			
			WorldParser::importWorldFile(findFileCaseInsensitive(EditorState::region.exportDirectory.string(), "world_" + EditorState::region.acronym + ".txt"));
		}
	);

	addButton("Canon")
	.OnLeftPress(
		[window](Button *button) {
			if (EditorState::roomPositionType == CANON_POSITION) {
				EditorState::roomPositionType = DEV_POSITION;
				button->Text("Dev");
			} else {
				EditorState::roomPositionType = CANON_POSITION;
				button->Text("Canon");
			}
		}
	);

	// addButton("Change Region Acronym").OnLeftPress(
	// 	[window](Button *button) {
	// 		if (EditorState::region.acronym == "") {
	// 			Popups::addPopup(new InfoPopup(window, "You must create or import a region\nbefore changing the acronym."));
	// 			return;
	// 		}

	// 		Popups::addPopup(new ChangeAcronymPopup(window));
	// 	}
	// );
}

void MenuItems::cleanup() {
	for (Button *button : buttons) {
		delete button;
	}

	buttons.clear();
}

void MenuItems::draw(Mouse *mouse, Vector2 screenBounds) {
	Draw::color(1.0, 1.0, 1.0);

	Draw::useTexture(MenuItems::textureBar);

	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	Draw::begin(Draw::QUADS);
	Draw::texCoord(-screenBounds.x, 0.0f); Draw::vertex(-screenBounds.x, screenBounds.y);
	Draw::texCoord( screenBounds.x, 0.0f); Draw::vertex( screenBounds.x, screenBounds.y);
	Draw::texCoord( screenBounds.x, 1.0f); Draw::vertex( screenBounds.x, screenBounds.y - 0.09f);
	Draw::texCoord(-screenBounds.x, 1.0f); Draw::vertex(-screenBounds.x, screenBounds.y - 0.09f);
	Draw::end();
	glDisable(GL_BLEND);
	Draw::useTexture(0);

	glLineWidth(1);

	for (Button *button : buttons) {
		button->update(mouse, screenBounds);
		button->draw(mouse, screenBounds);
	}
}

void MenuItems::repositionButtons() {
	currentButtonX = 0.01;

	for (Button *button : buttons) {
		button->X(currentButtonX);

		currentButtonX += button->Width() + 0.04;
	}
}