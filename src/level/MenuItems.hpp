#ifndef MENU_ITEMS_HPP
#define MENU_ITEMS_HPP

#include <vector>
#include <functional>
#include <string>

#include "../font/Fonts.hpp"

// #include "Utils.hpp"
#include "../Window.hpp"
#include "Main.hpp"

class Button {
	public:
		Button(std::string text, double x, double y, double width, double height, Font *font)
		 : x(x),
		   y(y),
		   width(width),
		   height(height),
		   text(text),
		   font(font) {
			// Temp
		}

		Button *OnPress(std::function<void(Button*)> listener) {
			listeners.push_back(listener);

			return this;
		}

		bool isHovered(Mouse *mouse) {
			double mouseX = mouse->X() / 512.0 - 1.0;
			double mouseY = -(mouse->Y() / 512.0 - 1.0);

			return mouseX >= x && mouseX <= x + width && mouseY <= y && mouseY >= y - height;
		}

		void update(Mouse *mouse) {
			if (isHovered(mouse) && mouse->Left()) {
				if (!lastPressed) {
					lastPressed = true;
					press();
				}
			} else {
				lastPressed = false;
			}
		}

		void draw(Mouse *mouse) {
			glColor3f(1.0f, 1.0f, 1.0f);
			font->write(text, x, y, height);

			if (isHovered(mouse)) {
				strokerect(x - 0.005, y + 0.005, x + 0.005 + width, y - 0.005 - height);
			}
		}

		void setText(std::string text) {
			this->text = text;
		}

	private:
		void press() {
			for (const auto &listener : listeners) {
				listener(this);
			}
		}

		std::vector<std::function<void(Button*)>> listeners;

		double x;
		double y;
		double width;
		double height;

		std::string text;
		Font *font;


		bool lastPressed = false;
};

class MenuItems {
	public:
		static void init(Window *window) {
			MenuItems::window = window;
			buttons.push_back(
				(new Button("New", -0.99, 0.99, 0.1, 0.04, Fonts::rainworld))->OnPress(
				[](Button *button) {
					if (History::unsavedChanges) {
						bool result = verifyBox("You have unsaved changes.\nAre you sure you want to create a new project?");

						if (!result) return;
					}

					delete project;

					project = new Project("Untitled", 72, 43);
				}
			));
			buttons.push_back(
				(new Button("Open", -0.84, 0.99, 0.12, 0.04, Fonts::rainworld))->OnPress(
				[](Button *button) {
					if (History::unsavedChanges) {
						bool result = verifyBox("You have unsaved changes.\nAre you sure you want to load project?");
						
						if (!result) return;
					}

					std::string path = OpenFileDialog();

					if (path == "") return;

					std::cout << "Opening " << path << std::endl;

					delete project;
					project = Project::loadFromPath(path);

					History::unsavedChanges = false;
				}
			));
			buttons.push_back(
				(new Button("Export", -0.67, 0.99, 0.15, 0.04, Fonts::rainworld))->OnPress(
				[](Button *button) {
					if (!project->validPath()) project->save();
					if (project->validPath()) project->exportDrizzle();
				}
			));
			buttons.push_back(
				(new Button("Size", -0.46, 0.99, 0.09, 0.04, Fonts::rainworld))->OnPress(
				[](Button *button) {
					int screenWidth = 0;
					int screenHeight = 0;
					std::cout << "Enter room width (screens) ";
					std::cin >> screenWidth;
					std::cout << "Enter room height (screens) ";
					std::cin >> screenHeight;
					std::cout << screenWidth << " " << screenHeight << std::endl;
					project->resize(52 * screenWidth + 20, 40 * screenHeight + 3);
				}
			));
			buttons.push_back(
				(new Button("Layer 1", -0.31, 0.99, 0.15, 0.04, Fonts::rainworld))->OnPress(
				[](Button *button) {
					currentLayer++;

					if (currentLayer > 3) currentLayer = 1;

					button->setText("Layer " + std::to_string(currentLayer));
				}
			));
			buttons.push_back(
				(new Button("Render", -0.10, 0.99, 0.16, 0.04, Fonts::rainworld))->OnPress(
				[](Button *button) {
					if (!project->validPath()) project->save();
					if (project->validPath()) project->render();
				}
			));
		}

		static void terminate() {
			for (Button *button : buttons) {
				delete button;
			}

			buttons.clear();
		}

		static void draw(Mouse *mouse) {
			glColor3f(0.0f, 0.0f, 0.0f);
			fillrect(-1.0f, 1.0f, 1.0f, 1.0f - 0.06f);

			for (Button *button : buttons) {
				button->update(mouse);
				button->draw(mouse);
			}

			// glColor3f(1.0f, 1.0f, 1.0f);
			// fontRainworld->write("File", -0.99f, 0.99f, 0.04f);
			// fontRainworld->write("Edit", -0.84f, 0.99f, 0.04f);

			// bool hovering = mouse->X() < 1024 * 0.045 && mouse->Y() < 1024 * 0.03;

			// if (hovering) {
			// 	strokerect(-0.995f, 0.995f, -0.91f, 0.945f);
			// }
		}

	private:
		static std::vector<Button*> buttons;

		static Window *window;
};

#endif