#include "SplashArtPopup.hpp"

#include "../WorldParser.hpp"
#include "../../ui/UI.hpp"

SplashArtPopup::SplashArtPopup() : Popup() {
	bounds = Rect(-1.0, -1.0, 1.0, 1.0);

	splashart = new Texture(getPath("splash.png"));
	uiIcons = new Texture(getPath("uiIcons.png"));

	std::ifstream versionFile(ASSETS_PATH / "version.txt");
	std::getline(versionFile, version);
	versionFile.close();
}

SplashArtPopup::~SplashArtPopup() {
	delete splashart;
	delete uiIcons;
}

const Rect SplashArtPopup::Bounds() {
	return Rect(-100.0, -100.0, 100.0, 100.0);
}

void SplashArtPopup::draw() {
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
	Fonts::rodondo->writeCentered("FloodForge", 0.0, 0.3, 0.2, CENTER_XY);
	Fonts::rainworld->writeCentered("World Editor", 0.0, 0.1, 0.1, CENTER_XY);
	Fonts::rainworld->write(version, -0.88, 0.63, 0.04);
	
	Draw::color(0.8f, 0.8f, 0.8f);
	Fonts::rainworld->writeCentered("Recent worlds:", -0.88, -0.28, 0.03, CENTER_Y);
	
	for (int i = 0; i < 8; i++) {
		if (i >= RecentFiles::recents.size()) break;

		std::string recent = RecentFiles::recentsNames[i];
		if (recent.empty()) {
			recent = RecentFiles::recents[i].stem().generic_u8string();
		}

		double y = -0.33 - i * 0.04;
		Rect rect(-0.89, y - 0.02, -0.4, y + 0.015);
		if (rect.inside(UI::mouse)) {
			Draw::color(0.25f, 0.25f, 0.25f);
			fillRect(rect);

			if (UI::mouse.justClicked()) {
				close();
				WorldParser::importWorldFile(RecentFiles::recents[i]);
				return;
			}
		}
		Draw::color(1.0f, 1.0f, 1.0f);
		Fonts::rainworld->writeCentered(recent, -0.88, y, 0.03, CENTER_Y);
	}

	strokeRect(-0.9, -0.65, 0.9, 0.65);
	Draw::begin(Draw::LINES);
	Draw::vertex(-0.9, -0.25);
	Draw::vertex(0.9, -0.25);
	Draw::end();

	{
		for (int i = 0; i < (EditorState::showAnniversary ? 2 : 1); i++) {
			Rect hoverRect = Rect::fromSize(0.31, -0.31 - i * 0.06, 0.59, 0.05);
			Rect rect = Rect::fromSize(0.305, -0.315 - i * 0.06, 0.5905, 0.06);
			if (hoverRect.inside(UI::mouse)) {
				Draw::color(0.25f, 0.25f, 0.25f);
				fillRect(rect);
	
				if (UI::mouse.justClicked()) {
					close();
					if (i == 0) {
						openURL("https://discord.gg/k5BExadp4x");
					} else if (i == 1) {
						Popups::addPopup(new MarkdownPopup(BASE_PATH / "docs" / "anniversary.md"));
					}
					return;
				}
			}
		}
	
		Draw::useTexture(uiIcons->ID());
		Draw::color(1.0, 1.0, 1.0);
		fillRect(UVRect::fromSize(0.31, -0.31, 0.05, 0.05).uv(0.0, 0.0, 0.25, 0.25));
		if (EditorState::showAnniversary) fillRect(UVRect::fromSize(0.31, -0.37, 0.05, 0.05).uv(0.25, 0.0, 0.5, 0.25));
		Draw::useTexture(0);
		Fonts::rainworld->writeCentered("Discord Server", 0.37, -0.285, 0.03, CENTER_Y);
		if (EditorState::showAnniversary) Fonts::rainworld->writeCentered("Anniversary Event", 0.37, -0.345, 0.03, CENTER_Y);
	}

	if (UI::mouse.justClicked()) {
		close();

		if (!Settings::getSetting<bool>(Settings::Setting::HideTutorial)) {
			Popups::addPopup(new MarkdownPopup(BASE_PATH / "docs" / "controls.md"));
		}
	}
}