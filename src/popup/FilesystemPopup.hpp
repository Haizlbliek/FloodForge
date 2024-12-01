#ifndef FILESYSTEM_POPUP_HPP
#define FILESYSTEM_POPUP_HPP

#include "../gl.h"

#include <iostream>
#include <string>
#include <vector>
#include <sstream>
#include <regex>

#include "../Window.hpp"
#include "../Theme.hpp"
#include "../font/Fonts.hpp"

// #include "MenuItems.hpp"
#include "Popups.hpp"

#define TYPE_FILE 0
#define TYPE_FOLDER 1

class FilesystemPopup : public Popup {
	public:
		FilesystemPopup(Window *window, std::regex regex, std::function<void(std::string)> callback)
		: Popup(window),
          regex(regex),
          callback(callback) {
			window->addKeyCallback(this, keyCallback);
			window->addScrollCallback(this, scrollCallback);
            setDirectory();
            called = false;
            forceRegex = true;
            mode = 0;
            scroll = 0;

            openType = TYPE_FILE;

            refresh();
		}

		FilesystemPopup(Window *window, int type, std::function<void(std::string)> callback)
		: Popup(window),
          callback(callback) {
			window->addKeyCallback(this, keyCallback);
			window->addScrollCallback(this, scrollCallback);
            called = false;
            forceRegex = true;
            mode = 0;
            scroll = 0;
            
            openType = type;

            setDirectory();
            refresh();
		}

        void accept() {
            if (mode == 0) {
                if (openType == TYPE_FOLDER) {
                    called = true;
                    callback(currentDirectory.string());
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
			removePopup(this);

			window->removeKeyCallback(this, keyCallback);
			window->removeScrollCallback(this, scrollCallback);
            
            if (!called) callback("");
		}

		void draw(double mouseX, double mouseY, bool mouseInside) {
			Popup::draw(mouseX, mouseY, mouseInside);

            frame++;

            setThemeColour(THEME_TEXT_COLOUR);
            drawIcon(1, bounds.X0() + 0.02, bounds.Y1() - 0.07);
            drawIcon(2, bounds.X0() + 0.09, bounds.Y1() - 0.07);
            drawIcon(5, bounds.X1() - 0.09, bounds.Y1() - 0.07);

            if (openType == TYPE_FILE) {
                if (forceRegex) {
                    drawIcon(6, bounds.X0() + 0.02, bounds.Y0() + 0.09);
                } else {
                    drawIcon(7, bounds.X0() + 0.02, bounds.Y0() + 0.09);
                }

                Fonts::rainworld->write("Show all", bounds.X0() + 0.09, bounds.Y0() + 0.09, 0.04);
            } else if (openType == TYPE_FOLDER) {
                Fonts::rainworld->write("Open", bounds.X1() - 0.17, bounds.Y0() + 0.09, 0.04);

                drawBounds(Rect(bounds.X1() - 0.17, bounds.Y0() + 0.09, bounds.X1() - 0.05, bounds.Y0() + 0.04), mouseX, mouseY);
            }

            drawBounds(Rect(bounds.X0() + 0.02, bounds.Y1() - 0.12, bounds.X0() + 0.07, bounds.Y1() - 0.07), mouseX, mouseY);
            drawBounds(Rect(bounds.X0() + 0.09, bounds.Y1() - 0.12, bounds.X0() + 0.14, bounds.Y1() - 0.07), mouseX, mouseY);
            drawBounds(Rect(bounds.X1() - 0.09, bounds.Y1() - 0.12, bounds.X1() - 0.04, bounds.Y1() - 0.07), mouseX, mouseY);

            std::string croppedPath = currentDirectory.string();
            if (croppedPath.size() > 25) croppedPath = croppedPath.substr(croppedPath.size() - 25);

            setThemeColour(THEME_TEXT_COLOUR);
            Fonts::rainworld->write(croppedPath, bounds.X0() + 0.19, bounds.Y1() - 0.07, 0.04);

            double offsetY = (bounds.Y1() + bounds.Y0()) * 0.5;
            double y = 0.35 - scroll + offsetY;
            bool hasExtras = false;

            // New Directory
            if (mode == 1) {
                if (y > -0.35 + offsetY) {
                    if (y > 0.375 + offsetY) {
                        y -= 0.06;
                    } else {
                        setThemeColour(THEME_TEXT_DISABLED_COLOUR);
                        fillRect(bounds.X0() + 0.1, y, bounds.X1() - 0.1, y - 0.05);
                        setThemeColour(THEME_TEXT_HIGHLIGHT_COLOUR);

                        Fonts::rainworld->write(newDirectory, bounds.X0() + 0.1, y, 0.04);

                        // Cursor
                        if (frame % 60 < 30) {
                            setThemeColour(THEME_TEXT_COLOUR);
                            double cursorX = bounds.X0() + 0.1 + Fonts::rainworld->getTextWidth(newDirectory, 0.04);
                            fillRect(cursorX, y + 0.01, cursorX + 0.005, y - 0.06);
                        }

                        setThemeColour(THEME_TEXT_DISABLED_COLOUR);
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

                if (mouseX >= bounds.X0() + 0.1 && mouseX <= bounds.X1() - 0.1 && mouseY <= y && mouseY >= y - 0.06)
                    setThemeColour(THEME_TEXT_HIGHLIGHT_COLOUR);
                else
                    setThemeColour(THEME_TEXT_COLOUR);

                Fonts::rainworld->write(path.filename().string() + "/", bounds.X0() + 0.1, y, 0.04);
                setThemeColour(THEME_TEXT_DISABLED_COLOUR);
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

                if (mouseX >= bounds.X0() + 0.1 && mouseX <= bounds.X1() - 0.1 && mouseY <= y && mouseY >= y - 0.06)
                    setThemeColour(THEME_TEXT_HIGHLIGHT_COLOUR);
                else
                    setThemeColour(THEME_TEXT_COLOUR);

                Fonts::rainworld->write(path.filename().string(), bounds.X0() + 0.1, y, 0.04);
                setThemeColour(THEME_TEXT_DISABLED_COLOUR);
                drawIcon(4, y);
                y -= 0.06;
            }
            
            // ...
            if (hasExtras) {
                setThemeColour(THEME_TEXT_DISABLED_COLOUR);
                Fonts::rainworld->write("...", bounds.X0() + 0.1, y, 0.04);
            }
		}

        void drawBounds(Rect rect, double mouseX, double mouseY) {
            if (!rect.inside(mouseX, mouseY)) return;

            setThemeColour(THEME_BORDER_HIGHLIGHT_COLOUR);
            strokeRect(rect.X0(), rect.Y0(), rect.X1(), rect.Y1());
        }

        void mouseClick(double mouseX, double mouseY) {
            Popup::mouseClick(mouseX, mouseY);

            if (mode == 0) {
                if (Rect(bounds.X0() + 0.02, bounds.Y1() - 0.12, bounds.X0() + 0.07, bounds.Y1() - 0.07).inside(mouseX, mouseY)) {
                    currentDirectory = std::filesystem::canonical(currentDirectory / "..");
                    scroll = 0.0;
                    refresh();
                    clampScroll();
                }

                if (Rect(bounds.X0() + 0.09, bounds.Y1() - 0.12, bounds.X0() + 0.14, bounds.Y1() - 0.07).inside(mouseX, mouseY)) {
                    refresh();
                    clampScroll();
                }

                if (Rect(bounds.X1() - 0.09, bounds.Y1() - 0.12, bounds.X1() - 0.04, bounds.Y1() - 0.07).inside(mouseX, mouseY)) {
                    mode = 1;
                    scroll = 0.0;
                    newDirectory = "";
                }
                
                if (mouseX >= bounds.X0() + 0.1 && mouseX <= bounds.X1() - 0.1 && mouseY >= bounds.Y0() + 0.2 && mouseY <= bounds.Y1() - 0.15) {
                    int id = (-mouseY + (bounds.Y1() - 0.15) - scroll) / 0.06;
                    
                    if (id < directories.size()) {
                        currentDirectory = std::filesystem::canonical(currentDirectory / directories[id].filename());
                        scroll = 0.0;
                        refresh();
                    } else {
                        id -= directories.size();

                        if (id < files.size()) {
                            called = true;
                            callback(files[id].string());
                            close();
                        }
                    }
                }

                if (openType == TYPE_FILE) {
                    if (Rect(bounds.X0() + 0.02, bounds.Y0() + 0.09, bounds.X0() + 0.07, bounds.Y0() + 0.04).inside(mouseX, mouseY)) {
                        forceRegex = !forceRegex;
                        refresh();
                        clampScroll();
                    }
                } else if (openType == TYPE_FOLDER) {
                    if (Rect(bounds.X1() - 0.17, bounds.Y0() + 0.09, bounds.X1() - 0.05, bounds.Y0() + 0.04).inside(mouseX, mouseY)) {
                        accept();
                    }
                }
            } else if (mode == 1) {
                accept();
            }
        }
        
		static void scrollCallback(void *object, double deltaX, double deltaY) {
            FilesystemPopup *popup = static_cast<FilesystemPopup*>(object);

            popup->scroll += deltaY * 0.06;
            
            popup->clampScroll();
        }

		static char parseCharacter(char character, bool shiftPressed) {
			if (!shiftPressed) return std::tolower(character);

			return std::toupper(character);
		}

		static void keyCallback(void *object, int action, int key) {
			FilesystemPopup *popup = static_cast<FilesystemPopup*>(object);

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

		bool canStack(std::string popupName) { return popupName == "WarningPopup"; }
		std::string PopupName() { return "FilesystemPopup"; }
    
    private:
        std::filesystem::path currentDirectory;

        std::vector<std::filesystem::path> directories;
        std::vector<std::filesystem::path> files;

        std::regex regex;
        std::function<void(std::string)> callback;

        double scroll;

        bool called;
        bool forceRegex;

        int mode;
        int frame = 0;

        int openType;

        std::string newDirectory;

        void setDirectory() {
            // TODO: Make this work
            // if (!MenuItems::ExportDirectory().empty()) {
            //     currentDirectory = MenuItems::ExportDirectory();
            //     return;
            // }

            if (std::filesystem::exists("C:\\Program Files (x86)\\Steam\\steamapps\\common\\Rain World\\RainWorld_Data\\StreamingAssets")) {
                currentDirectory = "C:\\Program Files (x86)\\Steam\\steamapps\\common\\Rain World\\RainWorld_Data\\StreamingAssets";
                return;
            }

            std::filesystem::path dir = std::filesystem::path(std::getenv("HOME")) / ".steam/steam/steamapps/common/Rain World/RainWorld_Data/StreamingAssets";
            if (std::filesystem::exists(dir)) {
                currentDirectory = dir;
                return;
            }

            currentDirectory = std::filesystem::canonical(BASE_PATH);
        }

        void refresh() {
            directories.clear();
            files.clear();

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
            drawIcon(type, bounds.X0() + 0.02, y);
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

            if (scroll < -size * 0.06 + 0.06) scroll = -size * 0.06 + 0.06;
            if (scroll > 0) scroll = 0;
        }
};

#endif