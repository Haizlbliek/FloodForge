#include "ResizeLevelPopup.hpp"

#include "../../ui/UI.hpp"
#include "DropletWindow.hpp"

ResizeLevelPopup::ResizeLevelPopup() : Popup() {
	bounds = Rect(-0.5, -0.21, 0.5, 0.21);
}

void ResizeLevelPopup::draw() {
	Popup::draw();

	if (this->minimized) return;

	static UI::TextInputEditable width(UI::TextInputEditableType::UnsignedInteger, "48");
	static UI::TextInputEditable height(UI::TextInputEditableType::UnsignedInteger, "35");
	static UI::TextInputEditable screenWidth(UI::TextInputEditableType::UnsignedFloat, "1.000", 3);
	static UI::TextInputEditable screenHeight(UI::TextInputEditableType::UnsignedFloat, "1.000", 3);
	static Vector2i resizeAnchor(0, 0);
	static bool stretchRoom = false;

	if (initial) {
		width.value = std::to_string(DropletWindow::room->width);
		height.value = std::to_string(DropletWindow::room->height);
		screenWidth.value = toFixed((DropletWindow::room->width + 4) / 52.0, screenWidth.floatDecimalCount);
		screenHeight.value = toFixed((DropletWindow::room->height + 5) / 40.0, screenHeight.floatDecimalCount);
		resizeAnchor = Vector2i(0, 0);
		stretchRoom = false;

		initial = false;
	}
	if (this->slatedForDeletion) {
		width.submit();
		height.submit();
		screenWidth.submit();
		screenHeight.submit();
	}

	double y = bounds.y1 - 0.05;
	y -= 0.05;
	setThemeColor(ThemeColour::Text);
	Fonts::rainworld->writeCentered("---- Room Size ----", bounds.x0 + 0.01, y + 0.025, 0.03, CENTER_Y);

	y -= 0.06;
	UI::TextInputResponse widthResponse = UI::TextInput(Rect::fromSize(bounds.x0 + 0.01, y, 0.25, 0.05), width);
	if (UI::TextureButton(UVRect::fromSize(bounds.x0 + 0.27, y, 0.05, 0.05).uv(0.0, 0.5, 0.25, 0.75), UI::TextureButtonMods().TextureId(UI::uiTexture).Disabled(widthResponse.focused))) {
		width.value = std::to_string(std::stoi(width.value) - 1);
		widthResponse.submitted = true;
	}
	if (UI::TextureButton(UVRect::fromSize(bounds.x0 + 0.33, y, 0.05, 0.05).uv(0.25, 0.5, 0.5, 0.75), UI::TextureButtonMods().TextureId(UI::uiTexture).Disabled(widthResponse.focused))) {
		width.value = std::to_string(std::stoi(width.value) + 1);
		widthResponse.submitted = true;
	}
	setThemeColor(ThemeColour::Text);
	Fonts::rainworld->writeCentered("Width (Tiles)", bounds.x0 + 0.40, y + 0.025, 0.03, CENTER_Y);

	y -= 0.06;
	UI::TextInputResponse heightResponse = UI::TextInput(Rect::fromSize(bounds.x0 + 0.01, y, 0.25, 0.05), height);
	if (UI::TextureButton(UVRect::fromSize(bounds.x0 + 0.27, y, 0.05, 0.05).uv(0.0, 0.5, 0.25, 0.75), UI::TextureButtonMods().TextureId(UI::uiTexture).Disabled(heightResponse.focused))) {
		height.value = std::to_string(std::stoi(height.value) - 1);
		heightResponse.submitted = true;
	}
	if (UI::TextureButton(UVRect::fromSize(bounds.x0 + 0.33, y, 0.05, 0.05).uv(0.25, 0.5, 0.5, 0.75), UI::TextureButtonMods().TextureId(UI::uiTexture).Disabled(heightResponse.focused))) {
		height.value = std::to_string(std::stoi(height.value) + 1);
		heightResponse.submitted = true;
	}
	setThemeColor(ThemeColour::Text);
	Fonts::rainworld->writeCentered("Height (Tiles)", bounds.x0 + 0.40, y + 0.025, 0.03, CENTER_Y);

	y -= 0.06;
	UI::TextInputResponse screenWidthResponse = UI::TextInput(Rect::fromSize(bounds.x0 + 0.01, y, 0.25, 0.05), screenWidth);
	if (UI::TextureButton(UVRect::fromSize(bounds.x0 + 0.27, y, 0.05, 0.05).uv(0.0, 0.5, 0.25, 0.75), UI::TextureButtonMods().TextureId(UI::uiTexture).Disabled(screenWidthResponse.focused))) {
		screenWidth.value = toFixed(std::stod(screenWidth.value) - 0.5, screenWidth.floatDecimalCount);
		screenWidthResponse.submitted = true;
	}
	if (UI::TextureButton(UVRect::fromSize(bounds.x0 + 0.33, y, 0.05, 0.05).uv(0.25, 0.5, 0.5, 0.75), UI::TextureButtonMods().TextureId(UI::uiTexture).Disabled(screenWidthResponse.focused))) {
		screenWidth.value = toFixed(std::stod(screenWidth.value) + 0.5, screenWidth.floatDecimalCount);
		screenWidthResponse.submitted = true;
	}
	setThemeColor(ThemeColour::Text);
	Fonts::rainworld->writeCentered("Width (Screens)", bounds.x0 + 0.40, y + 0.025, 0.03, CENTER_Y);

	y -= 0.06;
	UI::TextInputResponse screenHeightResponse = UI::TextInput(Rect::fromSize(bounds.x0 + 0.01, y, 0.25, 0.05), screenHeight);
	if (UI::TextureButton(UVRect::fromSize(bounds.x0 + 0.27, y, 0.05, 0.05).uv(0.0, 0.5, 0.25, 0.75), UI::TextureButtonMods().TextureId(UI::uiTexture).Disabled(screenHeightResponse.focused))) {
		screenHeight.value = toFixed(std::stod(screenHeight.value) - 0.5, screenHeight.floatDecimalCount);
		screenHeightResponse.submitted = true;
	}
	if (UI::TextureButton(UVRect::fromSize(bounds.x0 + 0.33, y, 0.05, 0.05).uv(0.25, 0.5, 0.5, 0.75), UI::TextureButtonMods().TextureId(UI::uiTexture).Disabled(screenHeightResponse.focused))) {
		screenHeight.value = toFixed(std::stod(screenHeight.value) + 0.5, screenHeight.floatDecimalCount);
		screenHeightResponse.submitted = true;
	}
	setThemeColor(ThemeColour::Text);
	Fonts::rainworld->writeCentered("Height (Screens)", bounds.x0 + 0.40, y + 0.025, 0.03, CENTER_Y);

	if (widthResponse.submitted) {
		screenWidth.value = toFixed((std::stoi(width.value) + 4) / 52.0, screenWidth.floatDecimalCount);
	}
	if (heightResponse.submitted) {
		screenHeight.value = toFixed((std::stoi(height.value) + 5) / 40.0, screenHeight.floatDecimalCount);
	}
	if (screenWidthResponse.submitted) {
		width.value = std::to_string(int(std::stod(screenWidth.value) * 52.0 - 4.0));
	}
	if (screenHeightResponse.submitted) {
		height.value = std::to_string(int(std::stod(screenHeight.value) * 40.0 - 5.0));
	}

	if (UI::TextureButton(UVRect::fromSize(bounds.x1 - 0.05, bounds.y1 - 0.11, 0.04, 0.04).uv(0.5, 0.0, 0.75, 0.25), UI::TextureButtonMods().TextureId(UI::uiTexture))) {
		initial = true;
	}

	for (int x = -1; x <= 1; x++) {
		for (int y = -1; y <= 1; y++) {
			UVRect rect = UVRect::fromSize(bounds.x1 - 0.17 + x * 0.08, bounds.y1 - 0.27 + y * 0.08, 0.07, 0.07);
			int diff = std::abs(x - resizeAnchor.x) + std::abs(y - resizeAnchor.y);
			if (diff == 0) {
				rect.uv(0.5, 0.25, 0.75, 0.5);
			} else if (diff >= 2) {
				rect.uv(0.0, 0.0, 0.0, 0.0);
			} else if (x < resizeAnchor.x) {
				rect.uv(0.5, 0.5, 0.75, 0.75);
			} else if (x > resizeAnchor.x) {
				rect.uv(0.75, 0.5, 1.0, 0.75);
			} else if (y < resizeAnchor.y) {
				rect.uv(0.5, 0.75, 0.75, 1.0);
			} else if (y > resizeAnchor.y) {
				rect.uv(0.75, 0.75, 1.0, 1.0);
			}
			if (UI::TextureButton(rect, UI::TextureButtonMods().Selected(x == resizeAnchor.x && y == resizeAnchor.y))) {
				resizeAnchor.x = x;
				resizeAnchor.y = y;
			}
		}
	}

	UI::ButtonResponse response = UI::TextButton(Rect::fromSize(bounds.x0 + 0.01, bounds.y1 - 0.41, 0.3, 0.05), "Resize room", UI::TextButtonMods().Disabled(widthResponse.focused || heightResponse.focused || screenWidthResponse.focused || screenHeightResponse.focused));
	if (response.clicked) {
		close();
	}

	setThemeColor(ThemeColour::Text);
	Fonts::rainworld->writeCentered("Stretch room", bounds.x1 - 0.07, bounds.y1 - 0.4, 0.03, RIGHT_X | BOTTOM_Y);
	UI::CheckBox(Rect::fromSize(bounds.x1 - 0.06, bounds.y1 - 0.41, 0.05, 0.05), stretchRoom);
}

void ResizeLevelPopup::close() {
	Popup::close();
}