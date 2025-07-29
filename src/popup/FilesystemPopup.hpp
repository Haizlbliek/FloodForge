#pragma once

#include "../gl.h"

#include <iostream>
#include <string>
#include <vector>
#include <sstream>
#include <regex>
#include <set>
#ifdef _WIN32
#include <windows.h>
#endif

#include "../Window.hpp"
#include "../Theme.hpp"
#include "../font/Fonts.hpp"
#include "../Settings.hpp"

#include "Popups.hpp"

#define TYPE_FILE 0
#define TYPE_FOLDER 1

#define MODE_NORMAL 2
#define MODE_NEW_DIRECTORY 3

class FilesystemPopup : public Popup {
	public:
		FilesystemPopup(Window *window, std::regex regex, std::string hint, std::function<void(std::set<std::string>)> callback);

		FilesystemPopup(Window *window, int type, std::string hint, std::function<void(std::set<std::string>)> callback);

		FilesystemPopup *AllowMultiple();

		void accept();

		void reject();

		void close();

		void draw(double mouseX, double mouseY, bool mouseInside, Vector2 screenBounds);

		void drawBounds(Rect rect, double mouseX, double mouseY);

		void mouseClick(double mouseX, double mouseY);
		
		static void scrollCallback(void *object, double deltaX, double deltaY);

		static char parseCharacter(char character, bool shiftPressed);

		static void keyCallback(void *object, int action, int key);

		bool canStack(std::string popupName) { return popupName == "InfoPopup" || popupName == "ConfirmPopup"; }
		std::string PopupName() { return "FilesystemPopup"; }
	
	private:
		std::filesystem::path currentDirectory;
		static std::filesystem::path previousDirectory;

		std::vector<std::filesystem::path> directories;
		std::vector<std::filesystem::path> files;

		std::regex regex;
		bool allowMultiple;

		std::function<void(std::set<std::string>)> callback;

		std::set<std::string> selected;

		double currentScroll;
		double targetScroll;

		bool called;
		bool forceRegex;

		int mode;
		int frame = 0;

		int openType;
		std::string hint;

		std::string newDirectory;

#ifdef _WIN32
		void loadDrives();

		std::vector<char> drives;
		int currentDrive;
#endif

		void setDirectory();

		void refresh();

		void drawIcon(int type, double y);

		void drawIcon(int type, double x, double y);

		void clampScroll();
};