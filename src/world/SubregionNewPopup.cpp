#include "SubregionNewPopup.hpp"

#include "MenuItems.hpp"

void SubregionNewPopup::mouseClick(double mouseX, double mouseY) {
	Popup::mouseClick(mouseX, mouseY);

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
	room->Subregion(std::distance(subregions.begin(), std::find(subregions.begin(), subregions.end(), text)));

	close();
}