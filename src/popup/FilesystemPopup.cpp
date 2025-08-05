#include "FilesystemPopup.hpp"

FilesystemPopup::FilesystemPopup(Window *window, std::regex regex, std::string hint, std::function<void(std::set<std::filesystem::path>)> callback) : Popup(window), regex(regex), hint(hint), callback(callback) {
	window->addKeyCallback(this, keyCallback);
	window->addScrollCallback(this, scrollCallback);
	called = false;
	forceRegex = true;
	mode = MODE_NORMAL;
	currentScroll = 0;
	targetScroll = 0;

	openType = TYPE_FILE;

#ifdef _WIN32
	currentDrive = 0;
	loadDrives();
#endif
	setDirectory();
	refresh();
}

FilesystemPopup::FilesystemPopup(Window *window, int type, std::string hint, std::function<void(std::set<std::filesystem::path>)> callback) : Popup(window), hint(hint), callback(callback) {
	window->addKeyCallback(this, keyCallback);
	window->addScrollCallback(this, scrollCallback);
	called = false;
	forceRegex = true;
	mode = MODE_NORMAL;
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

	if (mode == MODE_NORMAL) {
		if (openType == TYPE_FOLDER) {
			called = true;
			std::set<std::filesystem::path> output { currentDirectory };
			callback(output);
		}
		
		if (openType == TYPE_FILE) {
			called = true;
			callback(selected);
		}

		close();
	}

	if (mode == MODE_NEW_DIRECTORY) {
		if (newDirectory.empty() || std::filesystem::exists(currentDirectory / newDirectory)) {
			mode = MODE_NORMAL;
			newDirectory = "";
			return;
		}

		std::filesystem::create_directory(currentDirectory / newDirectory);
		mode = MODE_NORMAL;
		newDirectory = "";
		refresh();
	}
}

void FilesystemPopup::reject() {
	if (mode == MODE_NORMAL) close();

	if (mode == MODE_NEW_DIRECTORY) {
		newDirectory = "";
		mode = MODE_NORMAL;
	}
}

void FilesystemPopup::close() {
	Popups::removePopup(this);

	window->removeKeyCallback(this, keyCallback);
	window->removeScrollCallback(this, scrollCallback);
	
	window = nullptr;
	
	if (!called) callback(std::set<std::filesystem::path>());
}

void FilesystemPopup::draw(double mouseX, double mouseY, bool mouseInside, Vector2 screenBounds) {
	Popup::draw(mouseX, mouseY, mouseInside, screenBounds);
	
	if (minimized) return;

	currentScroll += (targetScroll - currentScroll) * Settings::getSetting<double>(Settings::Setting::PopupScrollSpeed);

	frame++;

	setThemeColour(ThemeColour::Text);
	drawIcon(1, bounds.x0 + 0.02, bounds.y1 - 0.07);
	drawIcon(2, bounds.x0 + 0.09, bounds.y1 - 0.07);
	drawIcon(5, bounds.x1 - 0.09, bounds.y1 - 0.07);

	if (openType == TYPE_FILE) {
		if (forceRegex) {
			drawIcon(6, bounds.x0 + 0.02, bounds.y0 + 0.09);
		} else {
			drawIcon(7, bounds.x0 + 0.02, bounds.y0 + 0.09);
		}

		Fonts::rainworld->write("Show all", bounds.x0 + 0.09, bounds.y0 + 0.09, 0.04);
		
		setThemeColour(ThemeColour::TextDisabled);
		Fonts::rainworld->write(hint, bounds.x0 + 0.35, bounds.y0 + 0.09, 0.04);
	} else {
		setThemeColour(ThemeColour::TextDisabled);
		Fonts::rainworld->write(hint, bounds.x0 + 0.02, bounds.y0 + 0.09, 0.04);
	}

	if (selected.empty() && openType == TYPE_FILE) {
		setThemeColour(ThemeColour::TextDisabled);
	} else {
		setThemeColour(ThemeColour::Text);
	}
	Fonts::rainworld->write("Open", bounds.x1 - 0.17, bounds.y0 + 0.09, 0.04);
	drawBounds(Rect(bounds.x1 - 0.17, bounds.y0 + 0.09, bounds.x1 - 0.05, bounds.y0 + 0.04), mouseX, mouseY);

	setThemeColour(ThemeColour::Text);
	drawBounds(Rect(bounds.x0 + 0.02, bounds.y1 - 0.12, bounds.x0 + 0.07, bounds.y1 - 0.07), mouseX, mouseY);
	drawBounds(Rect(bounds.x0 + 0.09, bounds.y1 - 0.12, bounds.x0 + 0.14, bounds.y1 - 0.07), mouseX, mouseY);
	drawBounds(Rect(bounds.x1 - 0.09, bounds.y1 - 0.12, bounds.x1 - 0.04, bounds.y1 - 0.07), mouseX, mouseY);

	std::string croppedPath = currentDirectory.generic_u8string();
	if (croppedPath.size() > 23) croppedPath = croppedPath.substr(croppedPath.size() - 23);

	setThemeColour(ThemeColour::Text);
	Fonts::rainworld->write(croppedPath, bounds.x0 + 0.23, bounds.y1 - 0.07, 0.04);

#ifdef _WIN32
	Fonts::rainworld->write(std::string(1, drives[currentDrive]) + ":", bounds.x0 + 0.16, bounds.y1 - 0.07, 0.05);
	drawBounds(Rect(bounds.x0 + 0.16, bounds.y1 - 0.12, bounds.x0 + 0.21, bounds.y1 - 0.07), mouseX, mouseY);
#endif

	double offsetY = (bounds.y1 + bounds.y0) * 0.5;
	double y = 0.35 - currentScroll + offsetY;
	bool hasExtras = false;

	// New Directory
	if (mode == MODE_NEW_DIRECTORY) {
		if (y > -0.35 + offsetY) {
			if (y > 0.375 + offsetY) {
				y -= 0.06;
			} else {
				setThemeColour(ThemeColour::TextDisabled);
				fillRect(bounds.x0 + 0.1, y, bounds.x1 - 0.1, y - 0.05);
				setThemeColour(ThemeColour::TextHighlight);

				Fonts::rainworld->write(newDirectory, bounds.x0 + 0.1, y, 0.04);

				// Cursor
				if (frame % 60 < 30) {
					setThemeColour(ThemeColour::Text);
					double cursorX = bounds.x0 + 0.1 + Fonts::rainworld->getTextWidth(newDirectory, 0.04);
					fillRect(cursorX, y + 0.01, cursorX + 0.005, y - 0.06);
				}

				setThemeColour(ThemeColour::TextDisabled);
				drawIcon(5, y);
				y -= 0.06;
			}
		}
	}

	// Directories
	for (std::filesystem::path path : directories) {
		if (y <= -0.30 + offsetY) { hasExtras = true; break; }
		if (y > 0.375 + offsetY) {
			y -= 0.06;
			continue;
		}

		if (mouseX >= bounds.x0 + 0.1 && mouseX <= bounds.x1 - 0.1 && mouseY <= y && mouseY >= y - 0.06)
			setThemeColour(ThemeColour::TextHighlight);
		else
			setThemeColour(ThemeColour::Text);

		Fonts::rainworld->write(path.filename().generic_u8string() + "/", bounds.x0 + 0.1, y, 0.04);
		setThemeColour(ThemeColour::TextDisabled);
		drawIcon(5, y);
		y -= 0.06;
	}

	// Files
	for (std::filesystem::path path : files) {
		if (y <= -0.30 + offsetY) { hasExtras = true; break; }

		if (y > 0.375 + offsetY) {
			y -= 0.06;
			continue;
		}

		if (mouseX >= bounds.x0 + 0.1 && mouseX <= bounds.x1 - 0.1 && mouseY <= y && mouseY >= y - 0.06)
			setThemeColour(ThemeColour::TextHighlight);
		else
			setThemeColour(ThemeColour::Text);

		if (selected.find(path.generic_u8string()) != selected.end()) {
			strokeRect(bounds.x0 + 0.09, y + 0.01, bounds.x1 - 0.09, y - 0.05);
		}

		Fonts::rainworld->write(path.filename().generic_u8string(), bounds.x0 + 0.1, y, 0.04);
		setThemeColour(ThemeColour::TextDisabled);
		drawIcon(4, y);
		y -= 0.06;
	}
	
	// ...
	if (hasExtras) {
		setThemeColour(ThemeColour::TextDisabled);
		Fonts::rainworld->write("...", bounds.x0 + 0.1, ceil(y / 0.06) * 0.06, 0.04);
	}
}

void FilesystemPopup::drawBounds(Rect rect, double mouseX, double mouseY) {
	if (!rect.inside(mouseX, mouseY)) return;

	setThemeColour(ThemeColour::BorderHighlight);
	strokeRect(rect.x0, rect.y0, rect.x1, rect.y1);
}

void FilesystemPopup::mouseClick(double mouseX, double mouseY) {
	Popup::mouseClick(mouseX, mouseY);

	if (mode == MODE_NORMAL) {
		if (Rect(bounds.x0 + 0.02, bounds.y1 - 0.12, bounds.x0 + 0.07, bounds.y1 - 0.07).inside(mouseX, mouseY)) {
			currentDirectory = std::filesystem::canonical(currentDirectory / "..");
			currentScroll = 0.0;
			targetScroll = 0.0;
			refresh();
			clampScroll();
		}

		if (Rect(bounds.x0 + 0.09, bounds.y1 - 0.12, bounds.x0 + 0.14, bounds.y1 - 0.07).inside(mouseX, mouseY)) {
			refresh();
			clampScroll();
		}

		if (Rect(bounds.x1 - 0.09, bounds.y1 - 0.12, bounds.x1 - 0.04, bounds.y1 - 0.07).inside(mouseX, mouseY)) {
			mode = MODE_NEW_DIRECTORY;
			currentScroll = 0.0;
			targetScroll = 0.0;
			newDirectory = "";
		}
		
		if (mouseX >= bounds.x0 + 0.1 && mouseX <= bounds.x1 - 0.1 && mouseY >= bounds.y0 + 0.2 && mouseY <= bounds.y1 - 0.15) {
			int id = (-mouseY + (bounds.y1 - 0.15) - currentScroll) / 0.06;
			
			if (id < directories.size()) {
				currentDirectory = std::filesystem::canonical(currentDirectory / directories[id].filename());
				currentScroll = 0.0;
				targetScroll = 0.0;
				refresh();
			} else {
				id -= directories.size();

				if (id < files.size()) {
					// called = true;
					if (allowMultiple && (window->modifierPressed(GLFW_MOD_SHIFT) || window->modifierPressed(GLFW_MOD_CONTROL))) {
						if (selected.find(files[id].generic_u8string()) == selected.end()) {
							selected.insert(files[id].generic_u8string());
						} else {
							selected.erase(files[id].generic_u8string());
						}
					} else {
						selected.clear();
						selected.insert(files[id].generic_u8string());
					}
					// close();
				}
			}
		}

		if (openType == TYPE_FILE) {
			if (Rect(bounds.x0 + 0.02, bounds.y0 + 0.09, bounds.x0 + 0.07, bounds.y0 + 0.04).inside(mouseX, mouseY)) {
				forceRegex = !forceRegex;
				refresh();
				clampScroll();
			}
		}
		
		if (Rect(bounds.x1 - 0.17, bounds.y0 + 0.09, bounds.x1 - 0.05, bounds.y0 + 0.04).inside(mouseX, mouseY)) {
			accept();
		}

#ifdef _WIN32
		if (Rect(bounds.x0 + 0.16, bounds.y1 - 0.12, bounds.x0 + 0.21, bounds.y1 - 0.07).inside(mouseX, mouseY)) {
			currentDrive = (currentDrive + 1) % drives.size();
			currentDirectory = std::filesystem::path(std::string(1, drives[currentDrive]) + ":\\");
			refresh();
		}
#endif
	} else if (mode == MODE_NEW_DIRECTORY) {
		accept();
	}
}

void FilesystemPopup::scrollCallback(void *object, double deltaX, double deltaY) {
	FilesystemPopup *popup = static_cast<FilesystemPopup*>(object);

	popup->targetScroll += deltaY * 0.06;
	
	popup->clampScroll();
}

char FilesystemPopup::parseCharacter(char character, bool shiftPressed) {
	if (!shiftPressed) return std::tolower(character);

	return std::toupper(character);
}

void FilesystemPopup::keyCallback(void *object, int action, int key) {
	FilesystemPopup *popup = static_cast<FilesystemPopup*>(object);

	if (!popup) {
		Logger::logError("Error: popup is nullptr.");
		return;
	}

	if (!popup->window) {
		Logger::logError("Error: popup->window is nullptr.");
		return;
	}

	if (popup->mode == MODE_NORMAL) return;

	if (action == GLFW_PRESS) {
		if (key >= GLFW_KEY_A && key <= GLFW_KEY_Z) {
			char character = parseCharacter(key, popup->window->keyPressed(GLFW_KEY_LEFT_SHIFT) || popup->window->keyPressed(GLFW_KEY_RIGHT_SHIFT));

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

		if (key == GLFW_KEY_BACKSPACE) {
			if (!popup->newDirectory.empty()) popup->newDirectory.pop_back();
			
			popup->frame = 0;
		}
	}
}

void FilesystemPopup::setDirectory() {
	// TODO: Make this work
	// if (!MenuItems::ExportDirectory().empty()) {
	//     currentDirectory = MenuItems::ExportDirectory();
	//     return;
	// }
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
				if (!forceRegex || std::regex_match(entry.path().filename().generic_u8string(), regex))
					files.push_back(entry.path());
			}
		}
	} catch (...) {}
}

void FilesystemPopup::drawIcon(int type, double y) {
	drawIcon(type, bounds.x0 + 0.02, y);
}

void FilesystemPopup::drawIcon(int type, double x, double y) {
	Draw::useTexture(Popups::textureUI);
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
		Logger::logError("Failed to get drive data");
		return;
	}
	
	for (char* drive = driveData.data(); *drive; drive += std::strlen(drive) + 1) {
		drives.push_back(drive[0]);
	}
}
#endif