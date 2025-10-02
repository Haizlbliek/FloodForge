#include "AcronymPopup.hpp"

#include "../MenuItems.hpp"
#include "../../ui/UI.hpp"

AcronymPopup::AcronymPopup() : Popup() {
	bounds = Rect(-0.25, -0.08, 0.25, 0.25);
}

void AcronymPopup::close() {
	if (canClose) {
		Popup::close();
	}
}

void AcronymPopup::draw() {
	Popup::draw();
	
	if (minimized) return;

	double centreX = (bounds.x0 + bounds.x1) * 0.5;

	static UI::TextInputEditable acronym = UI::TextInputEditable(UI::TextInputEditableType::Text, "").BanLetters(banLetters());
	if (needsSet) {
		acronym.value = setTo;
		needsSet = false;
	}
	UI::TextInputResponse acronymResponse = UI::TextInput(Rect(bounds.x0 + 0.01, bounds.y1 - 0.15, bounds.x1 - 0.01, bounds.y1 - 0.1), acronym);
	canClose = !acronymResponse.focused;

	if (UI::TextButton(Rect(centreX - 0.2, bounds.y1 - 0.28, centreX - 0.05, bounds.y1 - 0.22), "Cancel", UI::TextButtonMods().Disabled(acronymResponse.focused))) {
		canClose = true;
		reject();
		acronym.value.clear();
	}

	if (UI::TextButton(Rect(centreX + 0.05, bounds.y1 - 0.28, centreX + 0.2, bounds.y1 - 0.22), "Confirm", UI::TextButtonMods().Disabled(acronymResponse.focused || acronym.value.length() < minLength()))) {
		canClose = true;
		submit(acronym.value);
		acronym.value.clear();
	}
}

void AcronymPopup::submit(std::string acronym) {
	EditorState::region.reset();
	EditorState::offscreenDen = new OffscreenRoom("offscreenden" + toLower(acronym), "OffscreenDen" + toUpper(acronym));
	EditorState::rooms.push_back(EditorState::offscreenDen);
	EditorState::region.acronym = toLower(acronym);
	close();
}