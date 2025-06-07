#include "../gl.h"

#include <iostream>

#include "../Constants.hpp"
#include "../Window.hpp"
#include "../Utils.hpp"
#include "../Texture.hpp"
#include "../Theme.hpp"
#include "../Draw.hpp"
#include "../Settings.hpp"
#include "../font/Fonts.hpp"
#include "../popup/Popups.hpp"

#include "BodyChunk.hpp"
#include "Globals.hpp"

#define TEXTURE_PATH (BASE_PATH + "assets/")

#define clamp(x, a, b) x >= b ? b : (x <= a ? a : x)
#define min(a, b) (a < b) ? a : b
#define max(a, b) (a > b) ? a : b

Vector2 cameraOffset = Vector2(0.0, 0.0);
double cameraScale = 96.0;

std::vector<BodyChunk*> bodyChunks;
std::vector<BodyChunkConnection*> bodyChunkConnections;

BodyChunk *grabbedChunk = nullptr;

void drawCircle(float centreX, float centreY, float radius, int segmentCount) {
	Draw::begin(Draw::LINE_LOOP);

	for (int index = 0; index < segmentCount; index++)   {
		double theta = 2.0 * 3.1415926 * double(index) / double(segmentCount);
		float x = radius * cos(theta);
		float y = radius * sin(theta);
		Draw::vertex(x + centreX, y + centreY);
	}

	Draw::end();
}

void mainUpdate() {
	for (BodyChunk *bodyChunk : bodyChunks) {
		bodyChunk->update();
	}
	
	for (BodyChunkConnection *bodyChunkConnection : bodyChunkConnections) {
		bodyChunkConnection->update();
	}
}

void mainDraw() {
	Draw::begin(Draw::LINES);
	for (BodyChunkConnection *bodyChunkConnection : bodyChunkConnections) {
		if (bodyChunkConnection->type == BodyChunkConnection::Type::Normal) {
			Draw::color(0.0, 1.0, 0.0);
		} else if (bodyChunkConnection->type == BodyChunkConnection::Type::Pull) {
			Draw::color(0.0, 1.0, 1.0);
		} else if (bodyChunkConnection->type == BodyChunkConnection::Type::Push) {
			Draw::color(1.0, 1.0, 0.0);
		}

		Draw::vertex(bodyChunkConnection->chunk1->position.x, bodyChunkConnection->chunk1->position.y);
		Draw::vertex(bodyChunkConnection->chunk2->position.x, bodyChunkConnection->chunk2->position.y);
	}
	Draw::end();

	Draw::color(1.0, 0.0, 0.0);
	for (BodyChunk *bodyChunk : bodyChunks) {
		drawCircle(bodyChunk->position.x, bodyChunk->position.y, bodyChunk->radius, 12);
	}
}

int main() {
	Window *window = new Window(1024, 1024);
	window->setIcon(TEXTURE_PATH + "MainIcon.png");
	window->setTitle("Leviathan");

	if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
		Logger::logError("Failed to initialize GLAD!");
		return -1;
	}

	Fonts::init();
	Settings::init();
	Draw::init();
	Popups::init();

	int frame = 0;

	PhysicalObject *physicalObject = new PhysicalObject();

	bodyChunks.push_back(new BodyChunk(physicalObject, new Vector2(0,       0), 8.0 * 0.95, 0));
	bodyChunks.push_back(new BodyChunk(physicalObject, new Vector2(16.575,  0), 8.0 * 0.95, 0));
	bodyChunks.push_back(new BodyChunk(physicalObject, new Vector2(28.1775, 0), 8.0 * 0.95, 0));
	
	bodyChunkConnections.push_back(new BodyChunkConnection(bodyChunks[0], bodyChunks[1], 17 * 1.95 / 2,       BodyChunkConnection::Type::Normal, 0.95,                               0.5));
	bodyChunkConnections.push_back(new BodyChunkConnection(bodyChunks[1], bodyChunks[2], 17 * 1.95 / 2,       BodyChunkConnection::Type::Normal, 0.95,                               0.5));
	bodyChunkConnections.push_back(new BodyChunkConnection(bodyChunks[0], bodyChunks[2], 17 * 1.95 / 2 * 1.7, BodyChunkConnection::Type::Push,   1 - MathUtils::lerp(0.9, 0.5, 0.7), 0.5));
	
	while (window->isOpen()) {
		window->ensureFullscreen();

		int width;
		int height;
		glfwGetWindowSize(window->getGLFWWindow(), &width, &height);
		float size = min(width, height);

		Mouse *mouse = window->GetMouse();

		Vector2 globalMouse(
			(mouse->X() - ((width * 0.5) - size * 0.5)) / size * 1024,
			(mouse->Y() - ((height * 0.5) - size * 0.5)) / size * 1024
		);
		Vector2 screenMouse(
			(globalMouse.x / 1024.0) *  2.0 - 1.0,
			(globalMouse.y / 1024.0) * -2.0 + 1.0
		);

		Vector2 worldMouse = Vector2(
			screenMouse.x * cameraScale + cameraOffset.x,
			screenMouse.y * cameraScale + cameraOffset.y
		);

		// Update

		frame++;
		if (frame % 3 == 0) {
			window->GetMouse()->updateLastPressed();
			glfwPollEvents();

			mainUpdate();

			if (mouse->JustLeft()) {
				for (BodyChunk *bodyChunk : bodyChunks) {
					if (bodyChunk->position.distanceTo(worldMouse) <= bodyChunk->radius) {
						grabbedChunk = bodyChunk;
					}
				}
			}

			if (!mouse->Left()) {
				grabbedChunk = nullptr;
			}

			if (grabbedChunk != nullptr) {
				grabbedChunk->velocity.x += (worldMouse.x - grabbedChunk->position.x) * 0.3;
				grabbedChunk->velocity.y += (worldMouse.y - grabbedChunk->position.y) * 0.3;
			}
		}

		// Draw
		
		window->clear();
		glDisable(GL_DEPTH_TEST);

		setThemeColour(ThemeColour::Background);
		Vector2 screenBounds = Vector2(width, height) / size;
		fillRect(-screenBounds.x, -screenBounds.y, screenBounds.x, screenBounds.y);
		
		applyFrustumToOrthographic(cameraOffset, 0.0f, cameraScale * screenBounds);
		
		mainDraw();

		/// Draw UI
		applyFrustumToOrthographic(Vector2(0.0f, 0.0f), 0.0f, screenBounds);

		Popups::draw(screenMouse, screenBounds);
		
		window->render();
		
		Popups::cleanup();
	}

	Fonts::cleanup();
	Settings::cleanup();
	Draw::cleanup();

	return 0;
}