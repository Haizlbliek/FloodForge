#include "UI.hpp"

#include "../font/Fonts.hpp"
#include "../Theme.hpp"
#include "../Utils.hpp"

Vector2 UI::screenBounds;
Window *UI::window = nullptr;
UIMouse UI::mouse = UIMouse(0, 0);
bool UI::clipped = false;
Rect UI::clipRect;
UI::Editable *UI::currentEditable = nullptr;
int UI::selectTime = 0;
int UI::selectIndex = 0;
Texture *UI::uiTexture = nullptr;

UI::Editable::~Editable() {
	UI::Delete(*this);
}

bool UI::Editable::focused() {
	return UI::currentEditable == this;
}

UI::ButtonResponse::operator bool() const {
	return clicked;
}

void UI::TextInputEditable::submit() {
	if (UI::currentEditable != this) return;

	UI::updateTextInput(*this);
	UI::Delete(*this);
	this->submitted = true;
}

void UI::init(Window *window) {
	UI::window = window;
	UI::window->addKeyCallback(0, UI::_keyCallback);

	uiTexture = new Texture(getPath("ui.png"));
}

void UI::update() {
	if (UI::currentEditable == nullptr) {
		selectTime = 0;
		return;
	}

	selectTime = (selectTime + 1) % 60;
}

void UI::cleanup() {
	UI::window->removeKeyCallback(0, UI::_keyCallback);

	delete uiTexture;
}

static void UI::_keyCallback(void *object, int action, int key) {
	if (action != GLFW_PRESS && action != GLFW_REPEAT) return;
	if (UI::currentEditable == nullptr) return;

	UI::TextInputEditable *editable = static_cast<UI::TextInputEditable *>(UI::currentEditable);

	if (editable == nullptr) return;

	if (key == GLFW_KEY_ENTER) {
		editable->submitted = true;
		UI::updateTextInput(*editable);
		UI::currentEditable = nullptr;
		return;
	}

	if (key == GLFW_KEY_LEFT) {
		UI::selectIndex = std::max(UI::selectIndex - 1, 0);
		UI::selectTime = 0;
	}

	if (key == GLFW_KEY_RIGHT) {
		UI::selectIndex = std::min(UI::selectIndex + 1, int(editable->value.size()));
		UI::selectTime = 0;
	}

	char write = 0;
	if (editable->type == UI::TextInputEditableType::Text) {
		if (key >= 33 && key <= 126) {
			write = parseCharacter(key, UI::window->modifierPressed(GLFW_MOD_SHIFT), UI::window->modifierPressed(GLFW_MOD_CAPS_LOCK));
		}
		if (key == GLFW_KEY_SPACE) {
			write = ' ';
		}
		if (editable->bannedLetters.find(write) != std::string::npos) {
			write = 0;
		}

		if (key == GLFW_KEY_BACKSPACE) {
			write = 1;
		}
	} else {
		if (editable->type == UI::TextInputEditableType::SignedFloat || editable->type == UI::TextInputEditableType::SignedInteger) {
			if (key == '-') {
				write = '-';
			}
		}
		if (editable->type == UI::TextInputEditableType::SignedFloat || editable->type == UI::TextInputEditableType::UnsignedFloat) {
			if (key == '.') {
				write = '.';
			}
		}

		if (key >= '0' && key <= '9') {
			write = key;
		}

		if (key == GLFW_KEY_BACKSPACE) {
			write = 1;
		}
	}

	if (write == 0) return;
	if (write == 1) {
		UI::selectTime = 0;
		if (UI::selectIndex != 0) {
			editable->value = editable->value.substr(0, selectIndex - 1) + editable->value.substr(selectIndex);
			UI::selectIndex--;
		}

		return;
	}
	editable->value = editable->value.substr(0, selectIndex) + write + editable->value.substr(selectIndex);
	UI::selectIndex++;
}

void UI::updateTextInput(TextInputEditable &edit) {
	if (edit.type == UI::TextInputEditableType::Text) return;

	if (edit.type == UI::TextInputEditableType::SignedFloat || edit.type == UI::TextInputEditableType::UnsignedFloat) {
		try {
			edit.value = toFixed(std::stod(edit.value), edit.floatDecimalCount);
		} catch (...) { // std::invalid_argument, std::out_of_range
			edit.value = "0.";
			for (int i = 0; i < edit.floatDecimalCount; i++) {
				edit.value += "0";
			}
		}
	}
	else if (edit.type == UI::TextInputEditableType::SignedInteger || edit.type == UI::TextInputEditableType::UnsignedInteger) {
		try {
			edit.value = std::to_string(std::stoi(edit.value));
		} catch (...) { // std::invalid_argument, std::out_of_range
			edit.value = "0";
		}
	}
}

UI::ButtonResponse UI::Button(Rect rect, ButtonMods mods) {
	bool can = UI::canClick();
	bool highlight = can && rect.inside(UI::mouse);

	setThemeColor(ThemeColor::Button);
	fillRect(rect);

	setThemeColor((highlight || mods.selected) ? ThemeColor::BorderHighlight : ThemeColor::Border);
	strokeRect(rect);

	return {
		highlight && UI::mouse.justClicked(),
		highlight
	};
}

UI::ButtonResponse UI::TextButton(Rect rect, std::string text, TextButtonMods mods) {
	bool can = UI::canClick();
	bool highlight = can && rect.inside(UI::mouse);

	setThemeColor(mods.disabled ? ThemeColor::ButtonDisabled : ThemeColor::Button);
	fillRect(rect);

	if (mods.overrideTextColor) {
		Draw::color(mods.textColor);
	} else {
		setThemeColor(mods.disabled ? ThemeColor::TextDisabled : (mods.selected ? ThemeColor::TextHighlight : ThemeColor::Text));
	}
	Fonts::rainworld->writeCentered(text, rect.CenterX(), rect.CenterY(), 0.03, CENTER_XY);

	setThemeColor(mods.disabled ? ThemeColor::Border : ((highlight || mods.selected) ? ThemeColor::BorderHighlight : ThemeColor::Border));
	strokeRect(rect);

	return {
		highlight && UI::mouse.justClicked() && !mods.disabled,
		highlight
	};
}

UI::ButtonResponse UI::TextureButton(UVRect rect, TextureButtonMods mods) {
	bool can = canClick();
	bool highlight = can && rect.inside(UI::mouse);

	setThemeColor(mods.disabled ? ThemeColor::ButtonDisabled : ThemeColor::Button);
	fillRect(rect);

	glEnable(GL_BLEND);
	Draw::useTexture(mods.textureId);
	Draw::color(mods.textureColor);
	Draw::begin(Draw::QUADS);
	Draw::texCoord(rect.uv0.x, rect.uv0.y); Draw::vertex(MathUtils::lerp(rect.x1, rect.x0, 0.5 + mods.textureScale.x * 0.5), MathUtils::lerp(rect.y1, rect.y0, 0.5 + mods.textureScale.y * 0.5));
	Draw::texCoord(rect.uv1.x, rect.uv1.y); Draw::vertex(MathUtils::lerp(rect.x0, rect.x1, 0.5 + mods.textureScale.x * 0.5), MathUtils::lerp(rect.y1, rect.y0, 0.5 + mods.textureScale.y * 0.5));
	Draw::texCoord(rect.uv2.x, rect.uv2.y); Draw::vertex(MathUtils::lerp(rect.x0, rect.x1, 0.5 + mods.textureScale.x * 0.5), MathUtils::lerp(rect.y0, rect.y1, 0.5 + mods.textureScale.y * 0.5));
	Draw::texCoord(rect.uv3.x, rect.uv3.y); Draw::vertex(MathUtils::lerp(rect.x1, rect.x0, 0.5 + mods.textureScale.x * 0.5), MathUtils::lerp(rect.y0, rect.y1, 0.5 + mods.textureScale.y * 0.5));
	Draw::end();
	Draw::useTexture(0);
	glDisable(GL_BLEND);

	setThemeColor(mods.disabled ? ThemeColor::Border : ((highlight || mods.selected) ? ThemeColor::BorderHighlight : ThemeColor::Border));
	strokeRect(rect);

	return {
		highlight && UI::mouse.justClicked() && !mods.disabled,
		highlight
	};
}

UI::TextInputResponse UI::TextInput(Rect rect, UI::TextInputEditable &edit, TextInputMods mods) {
	bool selected = UI::currentEditable == &edit;
	bool can = canClick();
	bool highlight = can && rect.inside(UI::mouse) && !mods.disabled;
	bool submitted = edit.submitted;
	edit.submitted = false;

	setThemeColor(mods.disabled ? ThemeColor::ButtonDisabled : ThemeColor::Button);
	fillRect(rect);

	setThemeColor(mods.disabled ? ThemeColor::TextDisabled : (selected ? ThemeColor::TextHighlight : ThemeColor::Text));
	Fonts::rainworld->writeCentered(edit.value, rect.x0 + 0.01, rect.CenterY(), 0.03, CENTER_Y);
	if (selected && UI::selectTime < 30) {
		double width = Fonts::rainworld->getTextWidth(edit.value.substr(0, UI::selectIndex), 0.03);
		Draw::begin(Draw::LINES);
		Draw::vertex(rect.x0 + 0.012 + width, rect.CenterY() - 0.02);
		Draw::vertex(rect.x0 + 0.012 + width, rect.CenterY() + 0.02);
		Draw::end();
	}

	setThemeColor(mods.disabled ? ThemeColor::Border : ((highlight || selected) ? ThemeColor::BorderHighlight : ThemeColor::Border));
	strokeRect(rect);

	if (highlight && UI::mouse.justClicked() && !mods.disabled) {
		if (selected) {
			UI::updateTextInput(edit);
			UI::currentEditable = nullptr;
			submitted = true;
		} else {
			UI::selectTime = 0;
			UI::currentEditable = &edit;
			UI::selectIndex = edit.value.size();
		}
	}

	return {
		UI::currentEditable == &edit,
		highlight,
		submitted
	};
}

UI::CheckBoxResponse UI::CheckBox(Rect rect, bool &value) {
	UI::ButtonResponse response = UI::TextureButton(UVRect(rect.x0, rect.y0, rect.x1, rect.y1).uv(value ? 0.75 : 0.5, 0.25, value ? 1.0 : 0.75, 0.5), UI::TextureButtonMods().TextureId(UI::uiTexture->ID()));
	if (response.clicked) {
		value = !value;
		return { true, response.hovered, value };
	}

	return { false, response.hovered, value };
}

bool UI::canClick() {
	if (UI::mouse.disabled) return false;
	if (clipped) return clipRect.inside(UI::mouse);

	return true;
}

void UI::clip() {
	clipped = false;
}

void UI::clip(const Rect rect) {
	clipped = true;
	clipRect = rect;
}

void UI::Delete(const UI::Editable &editable) {
	if (UI::currentEditable == &editable) {
		UI::currentEditable = nullptr;
	}
}