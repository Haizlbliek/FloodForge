#include "SplashArtPopup.hpp"

#include "WorldParser.hpp"

SplashArtPopup::SplashArtPopup(Window *window) : Popup(window) {
	bounds = Rect(-1.0, -1.0, 1.0, 1.0);

	splashart = new Texture((BASE_PATH / "assets" / "splash.png").generic_u8string());
}

SplashArtPopup::~SplashArtPopup() {
	delete splashart;
}

const Rect SplashArtPopup::Bounds() {
	return Rect(-100.0, -100.0, 100.0, 100.0);
}

void SplashArtPopup::draw(double mouseX, double mouseY, bool mouseInside, Vector2 screenBounds) {
	Draw::color(0.0, 0.0, 0.0);
	fillRect(-0.9, -0.65, 0.9, 0.65);

	Draw::useTexture(splashart->ID());
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	Draw::begin(Draw::QUADS);

	Draw::color(0.75, 0.75, 0.75);

	Draw::texCoord(0.0f, 1.0f); Draw::vertex(-0.89, -0.24);
	Draw::texCoord(1.0f, 1.0f); Draw::vertex(0.89, -0.24);
	Draw::texCoord(1.0f, 0.0f); Draw::vertex(0.89, 0.64);
	Draw::texCoord(0.0f, 0.0f); Draw::vertex(-0.89, 0.64);

	Draw::end();

	Draw::useTexture(0);
	glDisable(GL_BLEND);

	Draw::color(1.0f, 1.0f, 1.0f);
	Fonts::rodondo->writeCentred("FloodForge", 0.0, 0.3, 0.2, CENTRE_XY);
	Fonts::rainworld->writeCentred("World Editor", 0.0, 0.1, 0.1, CENTRE_XY);
	Fonts::rainworld->write(FLOODFORGE_VERSION, -0.88, 0.63, 0.04);
	
	Draw::color(0.8f, 0.8f, 0.8f);
	Fonts::rainworld->writeCentred("Recent worlds:", -0.88, -0.28, 0.03, CENTRE_Y);
	
	for (int i = 0; i < 8; i++) {
		if (i >= RecentFiles::recents.size()) break;

		std::string recent = toLower(RecentFiles::recents[i].filename().generic_u8string());
		double y = -0.33 - i * 0.04;
		if (mouseX <= -0.4 && mouseY >= y - 0.015 && mouseY <= y + 0.015) {
			Draw::color(0.25f, 0.25f, 0.25f);
			fillRect(-0.89, y - 0.02, -0.4, y + 0.015);
		}
		Draw::color(1.0f, 1.0f, 1.0f);
		Fonts::rainworld->writeCentred(recent, -0.88, y, 0.03, CENTRE_Y);
	}

	strokeRect(-0.9, -0.65, 0.9, 0.65);
	Draw::begin(Draw::LINES);
	Draw::vertex(-0.9, -0.25);
	Draw::vertex(0.9, -0.25);
	Draw::end();
}

void SplashArtPopup::mouseClick(double mouseX, double mouseY) {
	if (mouseX >= -0.9 && mouseX <= 0.9 && mouseY <= -0.25 && mouseY >= -0.65) {
		for (int i = 0; i < 8; i++) {
			if (i >= RecentFiles::recents.size()) break;

			std::string recent = toLower(RecentFiles::recents[i].filename().generic_u8string());
			double y = -0.33 - i * 0.04;
			if (mouseX <= -0.4 && mouseY >= y - 0.015 && mouseY <= y + 0.015) {
				close();
				WorldParser::importWorldFile(RecentFiles::recents[i]);
			}
		}
	} else {
		close();
		
		if (!Settings::getSetting<bool>(Settings::Setting::HideTutorial)) {
			Popups::addPopup(new MarkdownPopup(window, BASE_PATH / "docs" / "controls.md"));
		}
	}
}