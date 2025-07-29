#pragma once

#include "../gl.h"

#include <iostream>
#include <string>
#include <vector>
#include <sstream>

#include "../Window.hpp"
#include "../Draw.hpp"
#include "../font/Fonts.hpp"
#include "../popup/MarkdownPopup.hpp"
#include "../popup/Popups.hpp"

#include "MenuItems.hpp"
#include "RecentFiles.hpp"

class SplashArtPopup : public Popup {
	public:
		SplashArtPopup(Window *window);

		~SplashArtPopup();
		
		const Rect Bounds() override;

		void draw(double mouseX, double mouseY, bool mouseInside, Vector2 screenBounds);

		void mouseClick(double mouseX, double mouseY);
		
		bool canStack(std::string popupName) override { return popupName == "MarkdownPopup" || popupName == "InfoPopup"; }
		std::string PopupName() override { return "SplashArtPopup"; }

	private:
		Texture *splashart;
};