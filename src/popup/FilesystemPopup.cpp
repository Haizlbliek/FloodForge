#include "FilesystemPopup.hpp"

#include "../ui/UI.hpp"

// TODO

FilesystemPopup::FilesystemPopup(std::regex regex, std::string hint, std::function<void(std::set<std::filesystem::path>)> callback) : Popup(), regex(regex), hint(hint), callback(callback) {
	UI::window->addKeyCallback(this, keyCallback);
	UI::window->addScrollCallback(this, scrollCallback);
	called = false;
	showAll = false;
	mode = FilesystemMode::NORMAL;
	currentScroll = 0;
	targetScroll = 0;

	openType = FilesystemType::FILE;

#ifdef _WIN32
	currentDrive = 0;
	loadDrives();
#endif
	setDirectory();
	refresh();
}

FilesystemPopup::FilesystemPopup(FilesystemType type, std::string hint, std::function<void(std::set<std::filesystem::path>)> callback) : Popup(), hint(hint), callback(callback) {
	UI::window->addKeyCallback(this, keyCallback);
	UI::window->addScrollCallback(this, scrollCallback);
	called = false;
	showAll = false;
	mode = FilesystemMode::NORMAL;
	currentScroll = 0;
	targetScroll = 0;

	openType = type;

#ifdef _WIN32
	currentDrive = 0;
	loadDrives();
#endif
	setDirectory();
	refresh();
}

FilesystemPopup *FilesystemPopup::AllowMultiple() {
	allowMultiple = true;
	return this;
}

void FilesystemPopup::accept() {
	previousDirectory = currentDirectory;

	if (mode == FilesystemMode::NORMAL) {
		if (openType == FilesystemType::FOLDER) {
			called = true;
			std::set<std::filesystem::path> output { currentDirectory };
			callback(output);
		}

		if (openType == FilesystemType::FILE) {
			called = true;
			callback(selected);
		}

		close();
	}

	if (mode == FilesystemMode::NEW_DIRECTORY) {
		if (newDirectory.empty() || std::filesystem::exists(currentDirectory / newDirectory)) {
			mode = FilesystemMode::NORMAL;
			newDirectory = "";
			return;
		}

		std::filesystem::create_directory(currentDirectory / newDirectory);
		mode = FilesystemMode::NORMAL;
		newDirectory = "";
		refresh();
	}
}

void FilesystemPopup::reject() {
	if (mode == FilesystemMode::NORMAL) close();

	if (mode == FilesystemMode::NEW_DIRECTORY) {
		newDirectory = "";
		mode = FilesystemMode::NORMAL;
	}
}

void FilesystemPopup::close() {
	Popup::close();

	UI::window->removeKeyCallback(this, keyCallback);
	UI::window->removeScrollCallback(this, scrollCallback);

	if (!called) callback(std::set<std::filesystem::path>());
}

void FilesystemPopup::draw() {
	Popup::draw();

	if (minimized) return;

	currentScroll += (targetScroll - currentScroll) * Settings::getSetting<double>(Settings::Setting::PopupScrollSpeed);

	frame++;

	// Up Directory
	if (UI::TextureButton(UVRect::fromSize(bounds.x0 + 0.02, bounds.y1 - 0.12, 0.05, 0.05).uv(0.25, 0.25, 0.5, 0.0))) {
		currentDirectory = std::filesystem::canonical(currentDirectory / "..");
		currentScroll = 0.0;
		targetScroll = 0.0;
		refresh();
		clampScroll();
	}

	// Refresh
	if (UI::TextureButton(UVRect::fromSize(bounds.x0 + 0.09, bounds.y1 - 0.12, 0.05, 0.05).uv(0.5, 0.25, 0.75, 0.0))) {
		refresh();
		clampScroll();
	}

	// New Directory
	if (UI::TextureButton(UVRect::fromSize(bounds.x1 - 0.09, bounds.y1 - 0.12, 0.05, 0.05).uv(0.25, 0.5, 0.5, 0.25))) {
		mode = FilesystemMode::NEW_DIRECTORY;
		currentScroll = 0.0;
		targetScroll = 0.0;
		newDirectory = "";
	}

	if (openType == FilesystemType::FILE) {
		if (UI::CheckBox(Rect::fromSize(bounds.x0 + 0.02, bounds.y0 + 0.04, 0.05, 0.05), showAll)) {
			refresh();
			clampScroll();
		}

		setThemeColor(ThemeColor::Text);
		Fonts::rainworld->write("Show all", bounds.x0 + 0.09, bounds.y0 + 0.09, 0.04);

		setThemeColor(ThemeColor::TextDisabled);
		Fonts::rainworld->write(hint, bounds.x0 + 0.35, bounds.y0 + 0.09, 0.04);
	} else {
		setThemeColor(ThemeColor::TextDisabled);
		Fonts::rainworld->write(hint, bounds.x0 + 0.02, bounds.y0 + 0.09, 0.04);
	}

	if (UI::TextButton(Rect(bounds.x1 - 0.16, bounds.y0 + 0.09, bounds.x1 - 0.05, bounds.y0 + 0.04), "Open", UI::TextButtonMods().Disabled(selected.empty() && openType == FilesystemType::FILE))) {
		accept();
	}

	std::string croppedPath = currentDirectory.generic_u8string();
	if (croppedPath.size() > 23) croppedPath = croppedPath.substr(croppedPath.size() - 23);

	setThemeColor(ThemeColor::Text);
	Fonts::rainworld->write(croppedPath, bounds.x0 + 0.23, bounds.y1 - 0.07, 0.04);

#ifdef _WIN32
	if (UI::TextButton(Rect(bounds.x0 + 0.16, bounds.y1 - 0.12, bounds.x0 + 0.21, bounds.y1 - 0.07), std::string(1, drives[currentDrive]) + ":")) {
		currentDrive = (currentDrive + 1) % drives.size();
		currentDirectory = std::filesystem::path(std::string(1, drives[currentDrive]) + ":\\");
		refresh();
	}
#endif

	double offsetY = (bounds.y1 + bounds.y0) * 0.5;
	double y = 0.35 - currentScroll + offsetY;
	bool hasExtras = false;

	// New Directory
	if (mode == FilesystemMode::NEW_DIRECTORY) {
		if (y > -0.35 + offsetY) {
			if (y > 0.375 + offsetY) {
				y -= 0.06;
			} else {
				setThemeColor(ThemeColor::TextDisabled);
				fillRect(bounds.x0 + 0.1, y, bounds.x1 - 0.1, y - 0.05);
				setThemeColor(ThemeColor::TextHighlight);

				Fonts::rainworld->write(newDirectory, bounds.x0 + 0.1, y, 0.04);

				// Cursor
				if (frame % 60 < 30) {
					setThemeColor(ThemeColor::Text);
					double cursorX = bounds.x0 + 0.1 + Fonts::rainworld->getTextWidth(newDirectory, 0.04);
					fillRect(cursorX, y + 0.01, cursorX + 0.005, y - 0.06);
				}

				setThemeColor(ThemeColor::TextDisabled);
				drawIcon(5, y);
				y -= 0.06;
			}
		}
	}

	bool refreshing = false;

	// Directories
	for (std::filesystem::path path : directories) {
		if (y <= -0.30 + offsetY) { hasExtras = true; break; }
		if (y > 0.375 + offsetY) {
			y -= 0.06;
			continue;
		}

		Rect rect = Rect(bounds.x0 + 0.1, y, bounds.x1 - 0.1, y - 0.06);
		setThemeColor(rect.inside(UI::mouse) ? ThemeColor::TextHighlight : ThemeColor::Text);

		Fonts::rainworld->write(path.filename().generic_u8string() + "/", bounds.x0 + 0.1, y, 0.04);
		setThemeColor(ThemeColor::TextDisabled);
		drawIcon(5, y);

		if (rect.inside(UI::mouse) && UI::mouse.justClicked()) {
			currentDirectory = std::filesystem::canonical(currentDirectory / path.filename());
			currentScroll = 0.0;
			targetScroll = 0.0;
			refresh();
			refreshing = true;
			break;
		}

		y -= 0.06;
	}

	// Files
	for (std::filesystem::path path : files) {
		if (refreshing) break;
		if (y <= -0.30 + offsetY) { hasExtras = true; break; }

		if (y > 0.375 + offsetY) {
			y -= 0.06;
			continue;
		}

		Rect rect = Rect(bounds.x0 + 0.1, y, bounds.x1 - 0.1, y - 0.06);
		if (rect.inside(UI::mouse) && UI::mouse.justClicked()) {
			if (allowMultiple && (UI::window->modifierPressed(GLFW_MOD_SHIFT) || UI::window->modifierPressed(GLFW_MOD_CONTROL))) {
				if (selected.find(path) == selected.end()) {
					selected.insert(path);
				} else {
					selected.erase(path);
				}
			} else {
				selected.clear();
				selected.insert(path);
			}
		}

		setThemeColor(rect.inside(UI::mouse) ? ThemeColor::TextHighlight : ThemeColor::Text);
		if (selected.find(path.generic_u8string()) != selected.end()) {
			strokeRect(bounds.x0 + 0.09, y + 0.01, bounds.x1 - 0.09, y - 0.05);
		}

		Fonts::rainworld->write(path.filename().generic_u8string(), bounds.x0 + 0.1, y, 0.04);
		setThemeColor(ThemeColor::TextDisabled);
		drawIcon(4, y);

		y -= 0.06;
	}

	// ...
	if (hasExtras && !refreshing) {
		setThemeColor(ThemeColor::TextDisabled);
		Fonts::rainworld->write("...", bounds.x0 + 0.1, ceil(y / 0.06) * 0.06, 0.04);
	}
}

void FilesystemPopup::drawBounds(Rect rect, double mouseX, double mouseY) {
	if (!rect.inside(mouseX, mouseY)) return;

	setThemeColor(ThemeColor::BorderHighlight);
	strokeRect(rect.x0, rect.y0, rect.x1, rect.y1);
}

void FilesystemPopup::scrollCallback(void *object, double deltaX, double deltaY) {
	FilesystemPopup *popup = static_cast<FilesystemPopup*>(object);

	if (!popup->hovered) return;

	popup->targetScroll += deltaY * 0.06;

	popup->clampScroll();
}

void FilesystemPopup::keyCallback(void *object, int action, int key) {
	FilesystemPopup *popup = static_cast<FilesystemPopup*>(object);

	if (!popup) {
		Logger::error("Error: popup is nullptr.");
		return;
	}

	if (popup->mode == FilesystemMode::NORMAL) return;

	if (action == GLFW_PRESS || action == GLFW_REPEAT) {
		if (key == GLFW_KEY_V && UI::window->modifierPressed(GLFW_MOD_CONTROL)) {
			for (char x : UI::window->getClipboard()) {
				if (x == 0 || x == '\\' || x == '/' || x == '.') continue;

				popup->newDirectory += x;
			}
			return;
		}

		if (key >= GLFW_KEY_A && key <= GLFW_KEY_Z) {
			char character = parseCharacter(key, UI::window->modifierPressed(GLFW_MOD_SHIFT), UI::window->modifierPressed(GLFW_MOD_CAPS_LOCK));

			popup->newDirectory += character;
			popup->frame = 0;
		}

		if (key >= GLFW_KEY_0 && key <= GLFW_KEY_9) {
			popup->newDirectory += key;
			popup->frame = 0;
		}

		if (key == GLFW_KEY_SPACE) {
			if (!popup->newDirectory.empty())
				popup->newDirectory += " ";

			popup->frame = 0;
		}

		if (key == GLFW_KEY_ENTER) {
			popup->accept();
		}

		if (key == GLFW_KEY_BACKSPACE) {
			if (!popup->newDirectory.empty()) popup->newDirectory.pop_back();

			popup->frame = 0;
		}
	}
}

void FilesystemPopup::setDirectory() {
	selected.clear();

	if (!previousDirectory.empty() && std::filesystem::exists(previousDirectory)) {
		currentDirectory = previousDirectory;
		return;
	}

	std::filesystem::path potentialPath;
	potentialPath = Settings::getSetting<std::string>(Settings::Setting::DefaultFilePath);
	if (std::filesystem::exists(potentialPath)) {
		currentDirectory = potentialPath;
		return;
	}

#ifdef _WIN32
	for (char drive : drives) {
		potentialPath = std::filesystem::path(std::string(1, drive) + ":\\") / "Program Files (x86)\\Steam\\steamapps\\common\\Rain World\\RainWorld_Data\\StreamingAssets";

		if (std::filesystem::exists(potentialPath)) {
			currentDirectory = potentialPath.generic_u8string();
			return;
		}
	}
#endif

	if (std::getenv("HOME") != nullptr) {
		potentialPath = std::filesystem::path(std::getenv("HOME")) / ".steam/steam/steamapps/common/Rain World/RainWorld_Data/StreamingAssets";
		if (std::filesystem::exists(potentialPath)) {
			currentDirectory = potentialPath;
			return;
		}
	}

	currentDirectory = std::filesystem::canonical(BASE_PATH);
}

void FilesystemPopup::refresh() {
	directories.clear();
	files.clear();
	selected.clear();

	try {
		for (const auto &entry : std::filesystem::directory_iterator(currentDirectory)) {
			if (entry.is_directory()) {
				directories.push_back(entry.path());
			} else {
				if (showAll || std::regex_match(entry.path().filename().generic_u8string(), regex))
					files.push_back(entry.path());
			}
		}
	} catch (...) {}
}

void FilesystemPopup::drawIcon(int type, double y) {
	drawIcon(type, bounds.x0 + 0.02, y);
}

void FilesystemPopup::drawIcon(int type, double x, double y) {
	Draw::useTexture(UI::uiTexture->ID());
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	Draw::begin(Draw::QUADS);

	float offsetUVx = (type % 4) * 0.25f;
	float offsetUVy = (type / 4) * 0.25f;

	Draw::texCoord(0.00f + offsetUVx, 0.00f + offsetUVy); Draw::vertex(x + 0.00, y);
	Draw::texCoord(0.25f + offsetUVx, 0.00f + offsetUVy); Draw::vertex(x + 0.05, y);
	Draw::texCoord(0.25f + offsetUVx, 0.25f + offsetUVy); Draw::vertex(x + 0.05, y - 0.05);
	Draw::texCoord(0.00f + offsetUVx, 0.25f + offsetUVy); Draw::vertex(x + 0.00, y - 0.05);

	Draw::end();
	Draw::useTexture(0);
	glDisable(GL_BLEND);
}

void FilesystemPopup::clampScroll() {
	int size = directories.size() + files.size();

	if (targetScroll < -size * 0.06 + 0.06) {
		targetScroll = -size * 0.06 + 0.06;
		if (currentScroll <= -size * 0.06 + 0.12) {
			currentScroll = -size * 0.06 + 0.03;
		}
	}
	if (targetScroll > 0) {
		targetScroll = 0;
		if (currentScroll >= -0.06) {
			currentScroll = 0.03;
		}
	}
}

#ifdef _WIN32
void FilesystemPopup::loadDrives() {
	std::vector<char> driveData(256);
	DWORD size = GetLogicalDriveStringsA(driveData.size(), driveData.data());

	if (size == 0) {
		Logger::error("Failed to get drive data");
		return;
	}

	for (char* drive = driveData.data(); *drive; drive += std::strlen(drive) + 1) {
		drives.push_back(drive[0]);
	}
}
#endif