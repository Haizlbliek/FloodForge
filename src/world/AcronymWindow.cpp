#include "AcronymWindow.hpp"

#include "MenuItems.hpp"

void AcronymWindow::mouseClick(double mouseX, double mouseY) {
	Popup::mouseClick(mouseX, mouseY);

	if (Rect(-0.2, -0.03, -0.05, 0.03).inside(mouseX, mouseY)) {
		reject();
	}

	if (Rect(0.05, -0.03, 0.2, 0.03).inside(mouseX, mouseY)) {
		accept();
	}
}

void AcronymWindow::accept() {
	if (text.length() < 2) return;

	close();
	for (Room *room : rooms) delete room;
	rooms.clear();
	for (Connection *connection : connections) delete connection;
	connections.clear();
	rooms.push_back(new OffscreenRoom("offscreenden" + toLower(text), "OffscreenDen" + text));
	MenuItems::WorldAcronym(toLower(text));
}