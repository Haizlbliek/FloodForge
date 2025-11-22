#include "Popups.hpp"

#include <algorithm>

#include "../ui/UI.hpp"
#include "../Theme.hpp"
#include "../Draw.hpp"

#include "FilesystemPopup.hpp"

namespace {

	Popup *interactingPopup = nullptr;
	Popup *mousePopup = nullptr;

}

std::vector<Popup*> Popups::popupTrash;
std::vector<Popup*> Popups::popups;
Popup *Popups::holdingPopup;
Vector2 Popups::holdingStart;

void Popups::init() {
}

void Popups::cleanup() {
	for (Popup *popup : Popups::popupTrash) {
		popup->finalCleanup();
		Popups::popups.erase(std::remove(Popups::popups.begin(), Popups::popups.end(), popup), Popups::popups.end());
		
		delete popup;
	}

	Popups::popupTrash.clear();
}

Popup::Popup() : bounds(Rect(-0.5, -0.5, 0.5, 0.5)) {
}

void Popup::draw() {
	hovered = Bounds().inside(UI::mouse);

	if (minimized) {
		setThemeColor(ThemeColor::Popup);
		fillRect(bounds.x0, bounds.y1 - 0.05, bounds.x1, bounds.y1);
	} else {
		setThemeColor(ThemeColor::Popup);
		fillRect(bounds.x0, bounds.y0, bounds.x1, bounds.y1);
	}

	setThemeColor(ThemeColor::PopupHeader);
	fillRect(bounds.x0, bounds.y1 - 0.05, bounds.x1, bounds.y1);

	if (hovered) {
		setThemeColor(ThemeColor::BorderHighlight);
	} else {
		setThemeColor(ThemeColor::Border);
	}
	if (minimized) {
		strokeRect(bounds.x0, bounds.y1 - 0.05, bounds.x1, bounds.y1);
	} else {
		strokeRect(bounds.x0, bounds.y0, bounds.x1, bounds.y1);
	}

	if (UI::TextureButton(UVRect(bounds.x1 - 0.05, bounds.y1 - 0.05, bounds.x1, bounds.y1).uv(0.0, 0.0, 0.25, 0.25), UI::TextureButtonMods().TextureId(UI::uiTexture))) {
		close();
	}

	UVRect minimizeButton = UVRect(bounds.x1 - 0.1, bounds.y1 - 0.05, bounds.x1 - 0.05, bounds.y1);
	if (minimized) {
		minimizeButton.uv(0.25, 0.5, 0.5, 0.75);
	} else {
		minimizeButton.uv(0.0, 0.5, 0.25, 0.75);
	}
	if (UI::TextureButton(minimizeButton, UI::TextureButtonMods().TextureId(UI::uiTexture))) {
		minimized = !minimized;
	}
}

const Rect Popup::Bounds() {
	if (minimized) {
		return Rect(bounds.x0, bounds.y1 - 0.05, bounds.x1, bounds.y1);
	}

	return bounds;
}

void Popup::close() {
	Popups::removePopup(this);
	this->slatedForDeletion = true;
}

bool Popup::drag(double mouseX, double mouseY) {
	if (mouseX >= bounds.x1 - 0.1 && mouseY >= bounds.y1 - 0.05)
		return false;

	return (mouseY >= bounds.y1 - 0.05);
}

void Popup::offset(Vector2 offset) {
	bounds.offset(offset);
}

void Popups::addPopup(Popup *popup) {
	bool canStack = true;
	for (Popup *otherPopup : Popups::popups) {
		if (!otherPopup->canStack(popup->PopupName())) {
			canStack = false;
			break;
		}
	}
	
	if (canStack) {
		Popups::popups.push_back(popup);
	} else {
		popup->close();
	}
}

void Popups::removePopup(Popup *popup) {
	Popups::popupTrash.push_back(popup);
}

void Popups::block() {
	if (!UI::mouse.leftMouse && !UI::mouse.rightMouse && !UI::mouse.middleMouse) {
		interactingPopup = nullptr;
	}

	for (int i = Popups::popups.size() - 1; i >= 0; i--) {
		Popup *popup = popups[i];

		if (popup->Bounds().inside(UI::mouse)) {
			if ((UI::mouse.leftMouse && !UI::mouse.lastLeftMouse) || (UI::mouse.rightMouse && !UI::mouse.lastRightMouse) || (UI::mouse.middleMouse && !UI::mouse.lastMiddleMouse)) {
				interactingPopup = popup;
			}
			break;
		}
	}

	UI::mouse.disabled = interactingPopup != nullptr;
}

void Popups::draw() {
	UI::mouse.disabled = false;

	for (int i = Popups::popups.size() - 1; i >= 0; i--) {
		Popup *popup = popups[i];

		if (popup->Bounds().inside(UI::mouse)) {
			if (UI::mouse.justClicked() && popup->drag(UI::mouse.x, UI::mouse.y)) {
				Popups::holdingPopup = popup;
				Popups::holdingStart.x = UI::mouse.x;
				Popups::holdingStart.y = UI::mouse.y;
			}
			mousePopup = popup;
			break;
		}
	}

	if (Popups::holdingPopup != nullptr) {
		if (UI::mouse.leftMouse) {
			if (holdingPopup != nullptr) {
				holdingPopup->offset(Vector2 { UI::mouse.x, UI::mouse.y } - holdingStart);
				holdingStart.x = UI::mouse.x;
				holdingStart.y = UI::mouse.y;
			}
		} else {
			holdingPopup = nullptr;
		}
	}

	for (Popup *popup : Popups::popups) {
		UI::mouse.disabled = popup != mousePopup;
		popup->draw();
	}
	UI::mouse.disabled = interactingPopup != nullptr;
}

bool Popups::hasPopup(std::string popupName) {
	for (Popup *popup : Popups::popups) {
		if (popup->PopupName() == popupName) return true;
	}

	return false;
}


std::filesystem::path FilesystemPopup::previousDirectory;