#include "ConfirmPopup.hpp"

#include "../ui/UI.hpp"

ConfirmPopup::ConfirmPopup(Window *window, std::string question) : Popup(window) {
	std::istringstream stream(question);
	std::string line;

	while (std::getline(stream, line)) {
		this->question.push_back(line);
	}

	double height = this->question.size() * 0.05 + 0.06 + 0.07;
	bounds = Rect(-0.4, -height * 0.5, 0.4, height * 0.5);
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

	setThemeColour(ThemeColour::Text);
	int lineId = 0;
	for (std::string line : question) {
		double y = bounds.y1 - 0.08 - 0.05 * lineId;
		Fonts::rainworld->writeCentered(line, 0.0, y, 0.04, CENTER_XY);

		lineId++;
	}

	if (UI::TextButton(Rect(bounds.x0 + 0.01, bounds.y0 + 0.06, bounds.CenterX() - 0.005, bounds.y0 + 0.01), buttonCancel)) {
		reject();
	}

	if (UI::TextButton(Rect(bounds.CenterX() + 0.005, bounds.y0 + 0.06, bounds.x1 - 0.01, bounds.y0 + 0.01), buttonOkay)) {
		accept();
	}
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

ConfirmPopup *ConfirmPopup::OnOkay(std::function<void()> listener) {
	listenersOkay.push_back(listener);
	return this;
}

ConfirmPopup *ConfirmPopup::OnCancel(std::function<void()> listener) {
	listenersCancel.push_back(listener);
	return this;
}