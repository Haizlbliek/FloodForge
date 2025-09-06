#include "ConfirmPopup.hpp"

ConfirmPopup::ConfirmPopup(Window *window, std::string question) : Popup(window) {
	this->question = question;
	bounds = Rect(-0.3, -0.15, 0.3, 0.15);
}

ConfirmPopup *ConfirmPopup::CancelText(std::string text) {
	buttonCancel = text;
	return this;
}

ConfirmPopup *ConfirmPopup::OkayText(std::string text) {
	buttonOkay = text;
	return this;
}

void ConfirmPopup::draw(double mouseX, double mouseY, bool mouseInside, Vector2 screenBounds) {
	Popup::draw(mouseX, mouseY, mouseInside, screenBounds);

	if (minimized) return;

	mouseX -= bounds.x0 + 0.3;
	mouseY -= bounds.y0 + 0.15;

	Draw::pushMatrix();

	Draw::translate(bounds.x0 + 0.3, bounds.y0 + 0.15);

	setThemeColour(ThemeColour::Text);
	Fonts::rainworld->writeCentered(question, 0.0, 0.04, 0.04, CENTER_XY);

	setThemeColour(ThemeColour::Button);
	fillRect(-0.25,  -0.09, -0.05, -0.03);
	fillRect( 0.05, -0.09,  0.25,  -0.03);

	setThemeColour(ThemeColour::Text);
	Fonts::rainworld->writeCentered(buttonCancel, -0.15, -0.06, 0.03, CENTER_XY);
	Fonts::rainworld->writeCentered(buttonOkay, 0.15, -0.06, 0.03, CENTER_XY);

	if (Rect(-0.2, -0.09, -0.05, -0.03).inside(mouseX, mouseY)) {
		setThemeColour(ThemeColour::BorderHighlight);
		strokeRect(-0.25, -0.09, -0.05, -0.03);
	} else {
		setThemeColour(ThemeColour::Border);
		strokeRect(-0.25, -0.09, -0.05, -0.03);
	}

	if (Rect(0.05, -0.09, 0.2, -0.03).inside(mouseX, mouseY)) {
		setThemeColour(ThemeColour::BorderHighlight);
		strokeRect(0.05, -0.09, 0.25, -0.03);
	} else {
		setThemeColour(ThemeColour::Border);
		strokeRect(0.05, -0.09, 0.25, -0.03);
	}

	Draw::popMatrix();
}

void ConfirmPopup::accept() {
	close();
	for (const auto &listener : listenersOkay) {
		listener();
	}
}

void ConfirmPopup::reject() {
	close();
	for (const auto &listener : listenersCancel) {
		listener();
	}
}

void ConfirmPopup::mouseClick(double mouseX, double mouseY) {
	Popup::mouseClick(mouseX, mouseY);

	mouseX -= bounds.x0 + 0.3;
	mouseY -= bounds.y0 + 0.15;

	if (Rect(-0.25, -0.09, -0.05, -0.03).inside(mouseX, mouseY)) {
		reject();
	}

	if (Rect(0.05, -0.09, 0.25, -0.03).inside(mouseX, mouseY)) {
		accept();
	}
}

ConfirmPopup *ConfirmPopup::OnOkay(std::function<void()> listener) {
	listenersOkay.push_back(listener);
	return this;
}

ConfirmPopup *ConfirmPopup::OnCancel(std::function<void()> listener) {
	listenersCancel.push_back(listener);
	return this;
}