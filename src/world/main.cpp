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

Vector2 lastMousePosition;


Vector2 worldMouse;

void signalHandler(int signal) {
	Logger::error("Signal caught: ", signal);
	std::exit(1);
}

void updateGlobalInputs() {
	if (EditorState::window->justPressed(GLFW_KEY_F11)) {
		EditorState::window->toggleFullscreen();
	}

	if (EditorState::window->justPressed(GLFW_KEY_ESCAPE)) {
		if (Popups::popups.size() > 0) {
			Popups::popups[Popups::popups.size() - 1]->reject();
		} else {
			Popups::addPopup((new ConfirmPopup(EditorState::window, "Exit FloodForge?"))->OnOkay([&]() {
				EditorState::window->close();
			}));
		}
	}

	if (EditorState::window->justPressed(GLFW_KEY_ENTER)) {
		if (Popups::popups.size() > 0) {
			Popups::popups[0]->accept();
		}
	}

	if (EditorState::window->modifierPressed(GLFW_MOD_ALT) && EditorState::window->justPressed(GLFW_KEY_T)) {
		Popups::addPopup(new MarkdownPopup(EditorState::window, BASE_PATH / "docs" / "controls.md"));
	}
}

int main() {
#ifdef NDEBUG
	std::signal(SIGSEGV, signalHandler); // Segmentation fault
	std::signal(SIGABRT, signalHandler); // Abort signal
	std::signal(SIGFPE, signalHandler);  // Floating-point error
	std::signal(SIGILL, signalHandler);  // Illegal instruction
	std::signal(SIGINT, signalHandler);  // Ctrl+C
	std::signal(SIGTERM, signalHandler); // Termination request
#endif

	EditorState::window = new Window(1024, 1024);
	Logger::info("Main icon path: ", BASE_PATH / "assets" / "mainIcon.png");
	EditorState::window->setIcon(BASE_PATH / "assets" / "mainIcon.png");
	EditorState::window->setTitle("FloodForge World Editor");
	EditorState::mouse = EditorState::window->GetMouse();

	if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
		Logger::error("Failed to initialize GLAD!");
		return -1;
	}

	UI::init(EditorState::window);
	Settings::init();
	Fonts::init();
	MenuItems::init(EditorState::window);
	Popups::init();
	Shaders::init();
	Draw::init();
	CreatureTextures::init();
	ConditionalTimelineTextures::init();
	RecentFiles::init();
	RoomHelpers::loadColours();

	Popups::addPopup(new SplashArtPopup(EditorState::window));

	while (EditorState::window->isOpen()) {
		EditorState::mouse->updateLastPressed();
		glfwPollEvents();

		EditorState::window->ensureFullscreen();

		glfwGetWindowSize(EditorState::window->getGLFWWindow(), &EditorState::windowSize.x, &EditorState::windowSize.y);
		if (EditorState::windowSize.x == 0 || EditorState::windowSize.y == 0) continue;
		float size = std::min(EditorState::windowSize.x, EditorState::windowSize.y);
		float offsetX = (EditorState::windowSize.x * 0.5) - size * 0.5;
		float offsetY = (EditorState::windowSize.y * 0.5) - size * 0.5;

		EditorState::globalMouse = Vector2(
			(EditorState::mouse->X() - offsetX) / size * 1024,
			(EditorState::mouse->Y() - offsetY) / size * 1024
		);
		UI::mouse.x = (EditorState::globalMouse.x / 1024.0) * 2.0 - 1.0;
		UI::mouse.y = (EditorState::globalMouse.y / 1024.0) * -2.0 + 1.0;
		UI::mouse.lastLeftMouse = UI::mouse.leftMouse;
		UI::mouse.leftMouse = EditorState::mouse->Left();

		EditorState::lineSize = 64.0 / EditorState::cameraScale;

		EditorState::screenBounds = Vector2(EditorState::windowSize.x, EditorState::windowSize.y) / size;


		glViewport(0, 0, EditorState::windowSize.x, EditorState::windowSize.y);
	
		EditorState::window->clear();
		glDisable(GL_DEPTH_TEST);
	
		setThemeColour(ThemeColour::Background);
		fillRect(-EditorState::screenBounds.x, -EditorState::screenBounds.y, EditorState::screenBounds.x, EditorState::screenBounds.y);

		updateGlobalInputs();

		if (EditorState::dropletOpen) {
			DropletWindow::Draw();
		} else {
			FloodForgeWindow::Draw();
		}

		/// Draw UI
		applyFrustumToOrthographic(Vector2(0.0f, 0.0f), 0.0f, EditorState::screenBounds);

		MenuItems::draw();

		Popups::draw(EditorState::screenBounds);

		EditorState::window->render();

		Popups::cleanup();

		lastMousePosition.x = EditorState::mouse->X();
		lastMousePosition.y = EditorState::mouse->Y();
	}

	for (Room *room : EditorState::rooms)
		delete room;

	EditorState::rooms.clear();

	for (Connection *connection : EditorState::connections)
		delete connection;

	EditorState::connections.clear();

	Fonts::cleanup();
	MenuItems::cleanup();
	Shaders::cleanup();
	Draw::cleanup();
	Settings::cleanup();
	UI::cleanup();

	return 0;
}