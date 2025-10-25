#include "../gl.h"

#include <iostream>
#include <fstream>
#include <cmath>
#include <vector>
#include <algorithm>
#include <csignal>

#include "../Constants.hpp"
#include "../Window.hpp"
#include "../Utils.hpp"
#include "../Texture.hpp"
#include "../math/Vector.hpp"
#include "../math/Rect.hpp"
#include "../font/Fonts.hpp"
#include "../Theme.hpp"
#include "../Draw.hpp"
#include "../Settings.hpp"

#include "../ui/UI.hpp"

#include "../popup/Popups.hpp"
#include "../popup/ConfirmPopup.hpp"
#include "popup/SplashArtPopup.hpp"
#include "ConditionalTimelineTextures.hpp"

#include "Shaders.hpp"
#include "Globals.hpp"
#include "Room.hpp"
#include "OffscreenRoom.hpp"
#include "Connection.hpp"
#include "MenuItems.hpp"
#include "RecentFiles.hpp"

#include "droplet/DropletWindow.hpp"
#include "flood_forge/FloodForgeWindow.hpp"

void signalHandler(int signal) {
	Logger::error("Signal caught: ", signal);
	std::exit(1);
}

void updateGlobalInputs() {
	if (UI::window->justPressed(GLFW_KEY_F11)) {
		UI::window->toggleFullscreen();
	}

	if (UI::window->justPressed(GLFW_KEY_ESCAPE)) {
		if (Popups::popups.size() > 0) {
			Popups::popups[Popups::popups.size() - 1]->reject();
		} else {
			if (EditorState::dropletOpen) {
				Popups::addPopup((new ConfirmPopup("Exit Droplet?\nUnsaved changes will be lost"))->OnOkay([&]() {
					EditorState::dropletOpen = false;
					DropletWindow::resetChanges();
				}));
			} else {
				Popups::addPopup((new ConfirmPopup("Exit FloodForge?"))->OnOkay([&]() {
					UI::window->close();
				}));
			}
		}
	}

	if (UI::window->justPressed(GLFW_KEY_ENTER)) {
		if (Popups::popups.size() > 0) {
			Popups::popups[0]->accept();
		}
	}

	if (UI::window->modifierPressed(GLFW_MOD_ALT)) {
		if (UI::window->justPressed(GLFW_KEY_T)) {
			Popups::addPopup(new MarkdownPopup(BASE_PATH / "docs" / "controls.md"));
		}
		if (UI::window->justPressed(GLFW_KEY_S)) {
			Popups::addPopup(new SplashArtPopup());
		}
	}
}

int main() {
	Logger::init();
#ifdef NDEBUG
	std::signal(SIGSEGV, signalHandler); // Segmentation fault
	std::signal(SIGABRT, signalHandler); // Abort signal
	std::signal(SIGFPE, signalHandler);  // Floating-point error
	std::signal(SIGILL, signalHandler);  // Illegal instruction
	std::signal(SIGINT, signalHandler);  // Ctrl+C
	std::signal(SIGTERM, signalHandler); // Termination request
#endif
	std::ifstream versionFile(BASE_PATH / "assets" / "version.txt");
	std::string version;
	std::getline(versionFile, version);
	versionFile.close();
	Logger::info("FloodForge ", version);
	Logger::info();

	UI::window = new Window(1024, 1024);
	UI::window->setIcon(BASE_PATH / "assets" / "mainIcon.png");
	UI::window->setTitle("FloodForge World Editor");

	if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
		Logger::error("Failed to initialize GLAD!");
		return -1;
	}

	DropletWindow::init();
	UI::init(UI::window);
	Settings::init();
	Fonts::init();
	MenuItems::init();
	Popups::init();
	Shaders::init();
	Draw::init();
	CreatureTextures::init();
	ConditionalTimelineTextures::init();
	RecentFiles::init();
	RoomHelpers::loadColours();
	FloodForgeWindow::initUndoRedo();

	Popups::addPopup(new SplashArtPopup());
	if (std::filesystem::exists(BASE_PATH / "running.txt")) {
		Popups::addPopup(new MarkdownPopup(BASE_PATH / "docs" / "crash.md"));
	}

	std::ofstream running(BASE_PATH / "running.txt");
	running << "Temporary file to detect crashes!";
	running.close();

	while (UI::window->isOpen()) {
		UI::window->GetMouse()->updateLastPressed();
		glfwPollEvents();

		UI::window->ensureFullscreen();

		if (UI::window->Width() == 0 || UI::window->Height() == 0) continue;
		float size = std::min(UI::window->Width(), UI::window->Height());
		float offsetX = (UI::window->Width() * 0.5) - size * 0.5;
		float offsetY = (UI::window->Height() * 0.5) - size * 0.5;

		EditorState::globalMouse = Vector2(
			(UI::window->GetMouse()->X() - offsetX) / size * 1024,
			(UI::window->GetMouse()->Y() - offsetY) / size * 1024
		);
		UI::mouse.lastX = UI::mouse.x;
		UI::mouse.lastY = UI::mouse.y;
		UI::mouse.x = (EditorState::globalMouse.x / 1024.0) * 2.0 - 1.0;
		UI::mouse.y = (EditorState::globalMouse.y / 1024.0) * -2.0 + 1.0;
		UI::mouse.lastLeftMouse = UI::mouse.leftMouse;
		UI::mouse.leftMouse = UI::window->GetMouse()->Left();
		UI::mouse.lastMiddleMouse = UI::mouse.middleMouse;
		UI::mouse.middleMouse = UI::window->GetMouse()->Middle();
		UI::mouse.lastRightMouse = UI::mouse.rightMouse;
		UI::mouse.rightMouse = UI::window->GetMouse()->Right();

		EditorState::lineSize = 64.0 / EditorState::cameraScale;

		UI::screenBounds = Vector2(UI::window->Width(), UI::window->Height()) / size;

		glViewport(0, 0, UI::window->Width(), UI::window->Height());
	
		UI::window->clear();
		glDisable(GL_DEPTH_TEST);
	
		setThemeColour(ThemeColour::Background);
		fillRect(-UI::screenBounds.x, -UI::screenBounds.y, UI::screenBounds.x, UI::screenBounds.y);

		updateGlobalInputs();

		if (EditorState::dropletOpen) {
			MenuItems::setLayer(MENU_LAYER_DROPLET);
			DropletWindow::Draw();
		} else {
			MenuItems::setLayer(MENU_LAYER_FLOOD_FORGE);
			FloodForgeWindow::Draw();
		}

		/// Draw UI
		applyFrustumToOrthographic(Vector2(0.0f, 0.0f), 0.0f, UI::screenBounds);

		MenuItems::draw();

		Popups::draw(UI::screenBounds);

		UI::window->render();

		Popups::cleanup();
	}

	for (Room *room : EditorState::rooms)
		delete room;

	EditorState::rooms.clear();

	for (Connection *connection : EditorState::connections)
		delete connection;

	EditorState::connections.clear();

	DropletWindow::cleanup();
	Fonts::cleanup();
	MenuItems::cleanup();
	Shaders::cleanup();
	Draw::cleanup();
	Settings::cleanup();
	UI::cleanup();
	FloodForgeWindow::cleanupUndoRedo();

	std::filesystem::remove(BASE_PATH / "running.txt");

	return 0;
}