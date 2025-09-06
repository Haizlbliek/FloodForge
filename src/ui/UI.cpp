#include "UI.hpp"

#include "../font/Fonts.hpp"
#include "../Theme.hpp"
#include "../Utils.hpp"

UIMouse UI::mouse = UIMouse(0, 0);
bool UI::clipped = false;
Rect UI::clipRect;

UI::ButtonResponse::operator bool() const {
	return clicked;
}

void UI::init() {}

void UI::cleanup() {}

UI::ButtonResponse UI::Button(Rect rect, ButtonMods mods) {
	bool can = UI::canClick();
	bool highlight = can && rect.inside(UI::mouse);

	setThemeColour(ThemeColour::Button);
	fillRect(rect);

	setThemeColor((highlight || mods.selected) ? ThemeColour::BorderHighlight : ThemeColour::Border);
	strokeRect(rect);

	return {
		highlight && UI::mouse.justClicked(),
		highlight
	};
}

UI::ButtonResponse UI::TextButton(Rect rect, std::string text, ButtonMods mods) {
	bool can = UI::canClick();
	bool highlight = can && rect.inside(UI::mouse);

	setThemeColour(mods.disabled ? ThemeColour::ButtonDisabled : ThemeColour::Button);
	fillRect(rect);

	setThemeColor(mods.disabled ? ThemeColour::TextDisabled : ((highlight || mods.selected) ? ThemeColour::TextHighlight : ThemeColour::Text));
	Fonts::rainworld->writeCentered(text, rect.CenterX(), rect.CenterY(), 0.03, CENTER_XY);

	setThemeColor(mods.disabled ? ThemeColour::Border : ((highlight || mods.selected) ? ThemeColour::BorderHighlight : ThemeColour::Border));
	strokeRect(rect);

	return {
		highlight && UI::mouse.justClicked() && !mods.disabled,
		highlight
	};
}

UI::ButtonResponse UI::TextureButton(UVRect rect, TextureButtonMods mods) {
	bool can = canClick();
	bool highlight = can && rect.inside(UI::mouse);

	setThemeColour(mods.disabled ? ThemeColour::ButtonDisabled : ThemeColour::Button);
	fillRect(rect);

	glEnable(GL_BLEND);
	Draw::useTexture(mods.textureId);
	Draw::color(mods.textureColor);
	Draw::begin(Draw::QUADS);
	Draw::texCoord(rect.uv0.x, rect.uv0.y); Draw::vertex(rect.x0, rect.y0);
	Draw::texCoord(rect.uv1.x, rect.uv1.y); Draw::vertex(rect.x1, rect.y0);
	Draw::texCoord(rect.uv2.x, rect.uv2.y); Draw::vertex(rect.x1, rect.y1);
	Draw::texCoord(rect.uv3.x, rect.uv3.y); Draw::vertex(rect.x0, rect.y1);
	Draw::end();
	Draw::useTexture(0);
	glDisable(GL_BLEND);

	setThemeColor(mods.disabled ? ThemeColour::Border : ((highlight || mods.selected) ? ThemeColour::BorderHighlight : ThemeColour::Border));
	strokeRect(rect);

	return {
		highlight && UI::mouse.justClicked() && !mods.disabled,
		highlight
	};
}

bool UI::canClick() {
	if (UI::mouse.disabled) return false;
	if (!clipped) return true;

	return clipRect.inside(UI::mouse);
}

void UI::clip() {
	clipped = false;
}

void UI::clip(const Rect rect) {
	clipped = true;
	clipRect = rect;
}