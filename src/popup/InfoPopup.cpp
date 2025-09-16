#include "InfoPopup.hpp"

InfoPopup::InfoPopup() : Popup() {
	bounds = Rect(-0.9, -0.1, 0.9, 0.1);
}

InfoPopup::InfoPopup(std::string warningText) : Popup() {
	std::istringstream stream(warningText);
	std::string line;

	while (std::getline(stream, line)) {
		warning.push_back(line);
	}

	double height = std::max(0.2, warning.size() * 0.05 + 0.07);
	bounds = Rect(-0.9, -height * 0.5, 0.9, height * 0.5);
}

void InfoPopup::draw() {
	Popup::draw();
	
	if (minimized) return;

	setThemeColour(ThemeColour::Text);

	int lineId = 0;
	for (std::string line : warning) {
		double y = -((lineId - warning.size() * 0.5) * 0.05) - 0.04 + (bounds.y0 + bounds.y1) * 0.5;
		Fonts::rainworld->writeCentered(line, (bounds.x0 + bounds.x1) * 0.5, y, 0.04, CENTER_XY);

		lineId++;
	}
}