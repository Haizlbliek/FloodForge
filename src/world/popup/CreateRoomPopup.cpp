#include "CreateRoomPopup.hpp"

#include "../../gl.h"

#include "../../popup/InfoPopup.hpp"
#include "../droplet/LevelUtils.hpp"
#include "../droplet/DropletWindow.hpp"

CreateRoomPopup::CreateRoomPopup() : Popup() {
}

void CreateRoomPopup::draw() {
	Popup::draw();

	if (this->minimized) return;

	static UI::TextInputEditable roomName(UI::TextInputEditableType::Text, "");
	static UI::TextInputEditable width(UI::TextInputEditableType::UnsignedInteger, "48");
	static UI::TextInputEditable height(UI::TextInputEditableType::UnsignedInteger, "35");
	static UI::TextInputEditable screenWidth(UI::TextInputEditableType::UnsignedFloat, "1.000", 3);
	static UI::TextInputEditable screenHeight(UI::TextInputEditableType::UnsignedFloat, "1.000", 3);
	static bool fillLayer1 = true;
	static bool fillLayer2 = true;
	static bool placeCameras = true;

	double y = bounds.y1 - 0.05;
	static std::string errorText = "";

	y -= 0.06;
	setThemeColor(ThemeColour::Text);
	Fonts::rainworld->writeCentered("---- Room Name ----", bounds.x0 + 0.01, y + 0.025, 0.03, CENTER_Y);

	y -= 0.06;
	setThemeColor(ThemeColour::Text);
	Fonts::rainworld->writeCentered(toUpper(EditorState::region.acronym) + "_", bounds.x0 + 0.01, y + 0.025, 0.03, CENTER_Y);
	double roomNameX = Fonts::rainworld->getTextWidth(toUpper(EditorState::region.acronym) + "_", 0.03);
	UI::TextInputResponse roomNameResponse = UI::TextInput(Rect::fromSize(bounds.x0 + 0.01 + roomNameX, y, 0.35, 0.05), roomName);

	y -= 0.06;
	setThemeColor(ThemeColour::Text);
	Fonts::rainworld->writeCentered("---- Room Size ----", bounds.x0 + 0.01, y + 0.025, 0.03, CENTER_Y);

	y -= 0.06;
	UI::TextInputResponse widthResponse = UI::TextInput(Rect::fromSize(bounds.x0 + 0.01, y, 0.25, 0.05), width);
	if (UI::TextureButton(UVRect::fromSize(bounds.x0 + 0.27, y, 0.05, 0.05).uv(0.0, 0.5, 0.25, 0.75), UI::TextureButtonMods().TextureId(UI::uiTexture).Disabled(widthResponse.focused))) {
		width.value = std::to_string(std::stoi(width.value) - 1);
		widthResponse.submitted = true;
	}
	if (UI::TextureButton(UVRect::fromSize(bounds.x0 + 0.32, y, 0.05, 0.05).uv(0.25, 0.5, 0.5, 0.75), UI::TextureButtonMods().TextureId(UI::uiTexture).Disabled(widthResponse.focused))) {
		width.value = std::to_string(std::stoi(width.value) + 1);
		widthResponse.submitted = true;
	}
	setThemeColor(ThemeColour::Text);
	Fonts::rainworld->writeCentered("Width (Tiles)", bounds.x0 + 0.38, y + 0.025, 0.03, CENTER_Y);

	y -= 0.06;
	UI::TextInputResponse heightResponse = UI::TextInput(Rect::fromSize(bounds.x0 + 0.01, y, 0.25, 0.05), height);
	if (UI::TextureButton(UVRect::fromSize(bounds.x0 + 0.27, y, 0.05, 0.05).uv(0.0, 0.5, 0.25, 0.75), UI::TextureButtonMods().TextureId(UI::uiTexture).Disabled(widthResponse.focused))) {
		height.value = std::to_string(std::stoi(height.value) - 1);
		heightResponse.submitted = true;
	}
	if (UI::TextureButton(UVRect::fromSize(bounds.x0 + 0.32, y, 0.05, 0.05).uv(0.25, 0.5, 0.5, 0.75), UI::TextureButtonMods().TextureId(UI::uiTexture).Disabled(widthResponse.focused))) {
		height.value = std::to_string(std::stoi(height.value) + 1);
		heightResponse.submitted = true;
	}
	setThemeColor(ThemeColour::Text);
	Fonts::rainworld->writeCentered("Height (Tiles)", bounds.x0 + 0.38, y + 0.025, 0.03, CENTER_Y);

	y -= 0.06;
	UI::TextInputResponse screenWidthResponse = UI::TextInput(Rect::fromSize(bounds.x0 + 0.01, y, 0.25, 0.05), screenWidth);
	if (UI::TextureButton(UVRect::fromSize(bounds.x0 + 0.27, y, 0.05, 0.05).uv(0.0, 0.5, 0.25, 0.75), UI::TextureButtonMods().TextureId(UI::uiTexture).Disabled(widthResponse.focused))) {
		screenWidth.value = toFixed(std::stod(screenWidth.value) - 0.5, screenWidth.floatDecimalCount);
		screenWidthResponse.submitted = true;
	}
	if (UI::TextureButton(UVRect::fromSize(bounds.x0 + 0.32, y, 0.05, 0.05).uv(0.25, 0.5, 0.5, 0.75), UI::TextureButtonMods().TextureId(UI::uiTexture).Disabled(widthResponse.focused))) {
		screenWidth.value = toFixed(std::stod(screenWidth.value) + 0.5, screenWidth.floatDecimalCount);
		screenWidthResponse.submitted = true;
	}
	setThemeColor(ThemeColour::Text);
	Fonts::rainworld->writeCentered("Width (Screens)", bounds.x0 + 0.38, y + 0.025, 0.03, CENTER_Y);

	y -= 0.06;
	UI::TextInputResponse screenHeightResponse = UI::TextInput(Rect::fromSize(bounds.x0 + 0.01, y, 0.25, 0.05), screenHeight);
	if (UI::TextureButton(UVRect::fromSize(bounds.x0 + 0.27, y, 0.05, 0.05).uv(0.0, 0.5, 0.25, 0.75), UI::TextureButtonMods().TextureId(UI::uiTexture).Disabled(widthResponse.focused))) {
		screenHeight.value = toFixed(std::stod(screenHeight.value) - 0.5, screenHeight.floatDecimalCount);
		screenHeightResponse.submitted = true;
	}
	if (UI::TextureButton(UVRect::fromSize(bounds.x0 + 0.32, y, 0.05, 0.05).uv(0.25, 0.5, 0.5, 0.75), UI::TextureButtonMods().TextureId(UI::uiTexture).Disabled(widthResponse.focused))) {
		screenHeight.value = toFixed(std::stod(screenHeight.value) + 0.5, screenHeight.floatDecimalCount);
		screenHeightResponse.submitted = true;
	}
	setThemeColor(ThemeColour::Text);
	Fonts::rainworld->writeCentered("Height (Screens)", bounds.x0 + 0.38, y + 0.025, 0.03, CENTER_Y);

	y -= 0.06;
	setThemeColor(ThemeColour::Text);
	Fonts::rainworld->writeCentered("---- Fill Layers ----", bounds.x0 + 0.01, y + 0.025, 0.03, CENTER_Y);

	y -= 0.06;
	if (UI::TextButton(Rect::fromSize(bounds.x0 + 0.01, y, 0.05, 0.05), "1", UI::TextButtonMods().Selected(fillLayer1))) { fillLayer1 = !fillLayer1; }
	if (UI::TextButton(Rect::fromSize(bounds.x0 + 0.07, y, 0.05, 0.05), "2", UI::TextButtonMods().Selected(fillLayer2))) { fillLayer2 = !fillLayer2; }

	y -= 0.06;
	setThemeColor(ThemeColour::Text);
	Fonts::rainworld->writeCentered("---- Options ----", bounds.x0 + 0.01, y + 0.025, 0.03, CENTER_Y);

	y -= 0.06;
	setThemeColor(ThemeColour::Text);
	Fonts::rainworld->writeCentered("Auto-place Cameras", bounds.x0 + 0.07, y + 0.025, 0.03, CENTER_Y);
	UI::CheckBox(Rect::fromSize(bounds.x0 + 0.01, y, 0.05, 0.05), placeCameras);


	if (widthResponse.submitted) {
		screenWidth.value = toFixed((std::stoi(width.value) + 4) / 52.0, screenWidth.floatDecimalCount);
		EditorState::placingRoomSize.x = std::stoi(width.value);
	}
	if (heightResponse.submitted) {
		screenHeight.value = toFixed((std::stoi(height.value) + 5) / 40.0, screenHeight.floatDecimalCount);
		EditorState::placingRoomSize.y = std::stoi(height.value);
	}
	if (screenWidthResponse.submitted) {
		width.value = std::to_string(int(std::stod(screenWidth.value) * 52.0 - 4.0));
		EditorState::placingRoomSize.x = std::stoi(width.value);
	}
	if (screenHeightResponse.submitted) {
		height.value = std::to_string(int(std::stod(screenHeight.value) * 40.0 - 5.0));
		EditorState::placingRoomSize.y = std::stoi(height.value);
	}

	if (UI::TextureButton(UVRect::fromSize(bounds.x1 - 0.05, bounds.y1 - 0.11, 0.04, 0.04).uv(0.5, 0.0, 0.75, 0.25), UI::TextureButtonMods().TextureId(UI::uiTexture))) {
		roomName.value.clear();
		width.value = "48";
		height.value = "35";
		screenWidth.value = "1.000";
		screenHeight.value = "1.000";
		fillLayer1 = true;
		fillLayer2 = true;
		errorText = "";
		EditorState::placingRoomSize.x = 48;
		EditorState::placingRoomSize.y = 35;
	}

	bool canCreate = true;
	if (roomName.value.empty()) canCreate = false;
	if (widthResponse.focused) canCreate = false;
	if (std::stoi(width.value) <= 0) canCreate = false;
	if (heightResponse.focused) canCreate = false;
	if (std::stoi(height.value) <= 0) canCreate = false;

	if (roomNameResponse.focused) {
		errorText = "";
		canCreate = false;
	} else if (roomNameResponse.submitted) {
		if (roomName.value.empty()) {
			errorText = "";
			canCreate = false;
		} else {
			std::string name = toUpper(EditorState::region.acronym) + "_" + roomName.value;
			std::filesystem::path filePath = findFileCaseInsensitive(EditorState::region.roomsDirectory, name + ".txt");
			if (!filePath.empty()) {
				canCreate = false;
				errorText = "Room with that name already exists in files!";
				return;
			}
		}
	}

	if (!errorText.empty()) {
		canCreate = false;
		setThemeColor(ThemeColour::Text);
		Fonts::rainworld->writeCentered(errorText, bounds.x0 + 0.27, bounds.y0 + 0.035, 0.03, CENTER_Y);
	}

	if (UI::TextButton(Rect(bounds.x0 + 0.01, bounds.y0 + 0.06, bounds.x0 + 0.26, bounds.y0 + 0.01), "Create", UI::TextButtonMods().Disabled(!canCreate))) {
		std::string name = toUpper(EditorState::region.acronym) + "_" + roomName.value;
		std::filesystem::path filePath = findFileCaseInsensitive(EditorState::region.roomsDirectory, name + ".txt");
		if (!filePath.empty()) {
			Popups::addPopup(new InfoPopup("Room with that name already exists in files!"));
			return;
		}
		LevelUtils::createLevelFiles(EditorState::region.roomsDirectory, name, std::stoi(width.value), std::stoi(height.value), fillLayer1, fillLayer2, placeCameras);
		close();

		Room *room = new Room(EditorState::region.roomsDirectory / (name + ".txt"), name);
		room->canonPosition = EditorState::placingRoomPosition - Vector2{ EditorState::placingRoomSize.x * 0.5, EditorState::placingRoomSize.y * -0.5 };
		room->devPosition = EditorState::placingRoomPosition - Vector2{ EditorState::placingRoomSize.x * 0.5, EditorState::placingRoomSize.y * -0.5 };
		EditorState::rooms.push_back(room);
		EditorState::dropletOpen = true;
		EditorState::dropletRoom = room;
		DropletWindow::loadRoom();
	}
}

void CreateRoomPopup::close() {
	Popup::close();

	EditorState::placingRoom = false;
}
