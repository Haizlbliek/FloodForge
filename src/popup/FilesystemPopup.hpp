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

class FilesystemPopup : public Popup {
	public:
		enum class FilesystemType {
			FILE,
			FOLDER
		};

		FilesystemPopup(std::regex regex, std::string hint, std::function<void(std::set<std::filesystem::path>)> callback);

		FilesystemPopup(FilesystemType type, std::string hint, std::function<void(std::set<std::filesystem::path>)> callback);

		FilesystemPopup *AllowMultiple();

		void accept();

		void reject();

		void close();

		void draw();

		void drawBounds(Rect rect, double mouseX, double mouseY);

		static void scrollCallback(void *object, double deltaX, double deltaY);

		static char parseCharacter(char character, bool shiftPressed);

		static void keyCallback(void *object, int action, int key);

		bool canStack(std::string popupName) { return popupName == "InfoPopup" || popupName == "ConfirmPopup"; }
		std::string PopupName() { return "FilesystemPopup"; }

	private:
		enum class FilesystemMode {
			NORMAL,
			NEW_DIRECTORY
		};

		std::filesystem::path currentDirectory;
		static std::filesystem::path previousDirectory;

		std::vector<std::filesystem::path> directories;
		std::vector<std::filesystem::path> files;

		std::regex regex;
		bool allowMultiple;

		std::function<void(std::set<std::filesystem::path>)> callback;

		std::set<std::filesystem::path> selected;

		double currentScroll;
		double targetScroll;

		bool called;
		bool showAll;

		FilesystemMode mode;
		int frame = 0;

		FilesystemType openType;
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