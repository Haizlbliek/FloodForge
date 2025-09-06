#pragma once

#include "../gl.h"

#include "../Window.hpp"
#include "../Theme.hpp"
#include "../font/Fonts.hpp"

#include "Popups.hpp"

class ConfirmPopup : public Popup {
	public:
		ConfirmPopup(Window *window, std::string question);
\
		ConfirmPopup *CancelText(std::string text);

		ConfirmPopup *OkayText(std::string text);

		void draw(double mouseX, double mouseY, bool mouseInside, Vector2 screenBounds);

		void accept();

		void reject();

		void mouseClick(double mouseX, double mouseY);

		ConfirmPopup *OnOkay(std::function<void()> listener);

		ConfirmPopup *OnCancel(std::function<void()> listener);

		bool canStack(std::string popupName) { return popupName == "InfoPopup" || popupName == "ConfirmPopup"; }
		std::string PopupName() { return "ConfirmPopup"; }

	private:
		std::string question;
		std::string buttonOkay = "Okay";
		std::string buttonCancel = "Cancel";

		std::vector<std::function<void()>> listenersOkay;
		std::vector<std::function<void()>> listenersCancel;
};