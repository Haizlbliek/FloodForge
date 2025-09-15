#pragma once

#include <string>
#include "../math/Rect.hpp"
#include "../math/UVRect.hpp"
#include "../math/Colour.hpp"
#include "../Window.hpp"
#include "../Texture.hpp"

#include "UIMouse.hpp"

namespace UI {
	struct ButtonResponse {
		operator bool() const;

		bool clicked;
		bool hovered;
	};

	struct TextInputResponse {
		bool focused;
		bool hovered;
		bool submitted;
	};

	struct CheckBoxResponse {
		bool clicked;
		bool checked;
	};

	struct Editable {
		bool submitted;

		~Editable();
	};

	enum TextInputEditableType {
		Text,
		UnsignedFloat,
		SignedFloat,
		UnsignedInteger,
		SignedInteger
	};

	struct TextInputEditable : Editable {
		TextInputEditable(TextInputEditableType type, std::string text, int floatDecimalCount = 1) {
			this->type = type;
			this->value = text;
			this->floatDecimalCount = floatDecimalCount;
		}

		TextInputEditableType type = TextInputEditableType::Text;
		int floatDecimalCount = 1;

		std::string value;
	};

	class ButtonMods {
		public:
			bool selected = false;
			bool disabled = false;

			ButtonMods &Selected(bool v = true) {
				selected = v;
				return *this;
			}

			ButtonMods &Disabled(bool v = true) {
				disabled = v;
				return *this;
			}
	};

	class TextureButtonMods : public ButtonMods {
		public:
			unsigned int textureId;
			Vector2 textureScale = Vector2(1.0, 1.0);
			Color textureColor = Color(1.0, 1.0, 1.0);

			TextureButtonMods &Selected(bool v = true) {
				selected = v;
				return *this;
			}

			TextureButtonMods &Disabled(bool v = true) {
				disabled = v;
				return *this;
			}

			TextureButtonMods &TextureId(unsigned int v) {
				textureId = v;
				return *this;
			}

			TextureButtonMods &TextureColor(Color v) {
				textureColor = v;
				return *this;
			}

			TextureButtonMods &TextureScale(float v) {
				textureScale = Vector2(v, v);
				return *this;
			}

			TextureButtonMods &TextureScale(Vector2 v) {
				textureScale = v;
				return *this;
			}
	};

	class TextButtonMods : public ButtonMods {
		public:
			bool overrideTextColor = false;
			Color textColor = Color(1.0, 1.0, 1.0);

			TextButtonMods &Selected(bool v = true) {
				selected = v;
				return *this;
			}

			TextButtonMods &Disabled(bool v = true) {
				disabled = v;
				return *this;
			}

			TextButtonMods &TextColor() {
				overrideTextColor = false;
				return *this;
			}

			TextButtonMods &TextColor(Color v) {
				textColor = v;
				overrideTextColor = true;
				return *this;
			}
	};

	class TextInputMods {
		public:
			bool disabled = false;

			TextInputMods &Disabled(bool v = true) {
				disabled = v;
				return *this;
			}
	};

	bool canClick();

	void clip();
	void clip(const Rect rect);

	void init(Window *window);
	void update();
	void cleanup();

	void updateTextInput(TextInputEditable &edit);

	UI::ButtonResponse Button(Rect rect, ButtonMods mods = {});
	UI::ButtonResponse TextButton(Rect rect, std::string text, TextButtonMods mods = {});
	UI::ButtonResponse TextureButton(UVRect rect, TextureButtonMods mods = {});
	UI::TextInputResponse TextInput(Rect rect, TextInputEditable &edit, TextInputMods mods = {});
	UI::CheckBoxResponse CheckBox(Rect rect, bool &value);

	void Delete(const Editable &editable);
	static void _keyCallback(void *object, int action, int key);
	extern Window *window;
	extern UIMouse mouse;
	extern bool clipped;
	extern Rect clipRect;
	extern Editable *currentEditable;
	extern int selectTime;
	extern int selectIndex;
	extern Texture *uiTexture;
}