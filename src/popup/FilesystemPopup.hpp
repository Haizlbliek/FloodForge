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

// #include "MenuItems.hpp"
#include "Popups.hpp"

#define TYPE_FILE 0
#define TYPE_FOLDER 1

class FilesystemPopup : public Popup {
	public:
		FilesystemPopup(Window *window, std::regex regex, std::string hint, std::function<void(std::set<std::string>)> callback)
		: Popup(window),
		  regex(regex),
		  hint(hint),
		  callback(callback) {
			window->addKeyCallback(this, keyCallback);
			window->addScrollCallback(this, scrollCallback);
			setDirectory();
			called = false;
			forceRegex = true;
			mode = 0;
			currentScroll = 0;
			targetScroll = 0;

			openType = TYPE_FILE;

			refresh();
		}

		FilesystemPopup(Window *window, int type, std::string hint, std::function<void(std::set<std::string>)> callback)
		: Popup(window),
		  hint(hint),
		  callback(callback) {
			window->addKeyCallback(this, keyCallback);
			window->addScrollCallback(this, scrollCallback);
			called = false;
			forceRegex = true;
			mode = 0;
			currentScroll = 0;
			
			openType = type;

			setDirectory();
			refresh();
		}

		FilesystemPopup *AllowMultiple() {
			allowMultiple = true;
			return this;
		}

		void accept() {
			previousDirectory = currentDirectory;

			if (mode == 0) {
				if (openType == TYPE_FOLDER) {
					called = true;
					std::set<std::string> output { currentDirectory.string() };
					callback(output);
				}
				
				if (openType == TYPE_FILE) {
					called = true;
					callback(selected);
				}

				close();
			}

			if (mode == 1) {
				if (newDirectory.empty() || std::filesystem::exists(currentDirectory / newDirectory)) {
					mode = 0;
					newDirectory = "";
					return;
				}

				std::filesystem::create_directory(currentDirectory / newDirectory);
				mode = 0;
				newDirectory = "";
				refresh();
			}
		}

		void reject() {
			if (mode == 0) close();

			if (mode == 1) {
				newDirectory = "";
				mode = 0;
			}
		}

		void close() {
			Popups::removePopup(this);

			window->removeKeyCallback(this, keyCallback);
			window->removeScrollCallback(this, scrollCallback);
			
			window = nullptr;
			
			if (!called) callback(std::set<std::string>());
		}

		void draw(double mouseX, double mouseY, bool mouseInside, Vector2 screenBounds) {
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

			std::string croppedPath = currentDirectory.string();
			if (croppedPath.size() > 25) croppedPath = croppedPath.substr(croppedPath.size() - 25);

			setThemeColour(ThemeColour::Text);
			Fonts::rainworld->write(croppedPath, bounds.x0 + 0.19, bounds.y1 - 0.07, 0.04);

			double offsetY = (bounds.y1 + bounds.y0) * 0.5;
			double y = 0.35 - currentScroll + offsetY;
			bool hasExtras = false;

			// New Directory
			if (mode == 1) {
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

				Fonts::rainworld->write(path.filename().string() + "/", bounds.x0 + 0.1, y, 0.04);
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

				if (selected.find(path.string()) != selected.end()) {
					strokeRect(bounds.x0 + 0.09, y + 0.01, bounds.x1 - 0.09, y - 0.05);
				}

				Fonts::rainworld->write(path.filename().string(), bounds.x0 + 0.1, y, 0.04);
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

		void drawBounds(Rect rect, double mouseX, double mouseY) {
			if (!rect.inside(mouseX, mouseY)) return;

			setThemeColour(ThemeColour::BorderHighlight);
			strokeRect(rect.x0, rect.y0, rect.x1, rect.y1);
		}

		void mouseClick(double mouseX, double mouseY) {
			Popup::mouseClick(mouseX, mouseY);

			if (mode == 0) {
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
					mode = 1;
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
							if (allowMultiple && (window->keyPressed(GLFW_KEY_LEFT_SHIFT) || window->keyPressed(GLFW_KEY_RIGHT_SHIFT))) {
								if (selected.find(files[id].string()) == selected.end()) {
									selected.insert(files[id].string());
								} else {
									selected.erase(files[id].string());
								}
							} else {
								selected.clear();
								selected.insert(files[id].string());
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
			} else if (mode == 1) {
				accept();
			}
		}
		
		static void scrollCallback(void *object, double deltaX, double deltaY) {
			FilesystemPopup *popup = static_cast<FilesystemPopup*>(object);

			popup->targetScroll += deltaY * 0.06;
			
			popup->clampScroll();
		}

		static char parseCharacter(char character, bool shiftPressed) {
			if (!shiftPressed) return std::tolower(character);

			return std::toupper(character);
		}

		static void keyCallback(void *object, int action, int key) {
			FilesystemPopup *popup = static_cast<FilesystemPopup*>(object);

			if (!popup) {
				Logger::logError("Error: popup is nullptr.");
				return;
			}

			if (!popup->window) {
				Logger::logError("Error: popup->window is nullptr.");
				return;
			}

			if (popup->mode == 0) return;

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

		void setDirectory() {
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
			std::vector<char> drives(256);
			DWORD size = GetLogicalDriveStringsA(drives.size(), drives.data());

			if (size == 0) {
				Logger::logError("Failed to get drives");
				return;
			}

			for (char* drive = drives.data(); *drive; drive += std::strlen(drive) + 1) {
				potentialPath = std::filesystem::path(drive) / "Program Files (x86)\\Steam\\steamapps\\common\\Rain World\\RainWorld_Data\\StreamingAssets";

				if (std::filesystem::exists(potentialPath)) {
					currentDirectory = potentialPath.string();
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

		void refresh() {
			directories.clear();
			files.clear();
			selected.clear();

			try {
				for (const auto &entry : std::filesystem::directory_iterator(currentDirectory)) {
					if (entry.is_directory()) {
						directories.push_back(entry.path());
					} else {
						if (!forceRegex || std::regex_match(entry.path().filename().string(), regex))
							files.push_back(entry.path());
					}
				}
			} catch (...) {}
		}

		void drawIcon(int type, double y) {
			drawIcon(type, bounds.x0 + 0.02, y);
		}

		void drawIcon(int type, double x, double y) {
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

		void clampScroll() {
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
};