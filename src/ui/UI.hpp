#pragma once

#include <string>
#include "../math/Rect.hpp"
#include "../math/UVRect.hpp"
#include "../math/Colour.hpp"

#include "UIMouse.hpp"

namespace UI {
	struct ButtonResponse {
		operator bool() const;

		bool clicked;
		bool hovered;
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
	};

	bool canClick();

	void clip();
	void clip(const Rect rect);

	void init();

	void cleanup();

	UI::ButtonResponse Button(Rect rect, ButtonMods mods = {});
	UI::ButtonResponse TextButton(Rect rect, std::string text, ButtonMods mods = {});
	UI::ButtonResponse TextureButton(UVRect rect, TextureButtonMods mods = {});

	extern UIMouse mouse;
	extern bool clipped;
	extern Rect clipRect;
}