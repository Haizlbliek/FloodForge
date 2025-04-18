#include "SubregionNewPopup.hpp"

#include "MenuItems.hpp"

void SubregionNewPopup::mouseClick(double mouseX, double mouseY) {
	Popup::mouseClick(mouseX, mouseY);

	mouseX -= bounds.X0() + 0.25;
	mouseY -= bounds.Y0() + 0.08;

	if (Rect(-0.2, -0.03, -0.05, 0.03).inside(mouseX, mouseY)) {
		reject();
	}

	if (Rect(0.05, -0.03, 0.2, 0.03).inside(mouseX, mouseY)) {
		accept();
	}
}

void SubregionNewPopup::accept() {
	if (std::find(subregions.begin(), subregions.end(), text) != subregions.end())
		return;

	subregions.push_back(text);
	for (Room *room : rooms) {
		room->subregion = std::distance(subregions.begin(), std::find(subregions.begin(), subregions.end(), text));
	}

	close();
}