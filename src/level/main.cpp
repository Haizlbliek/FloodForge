#include "Main.hpp"

#include "../gl.h"
#include "../stb_image.h"
#include "../stb_image_write.h"

#include <iostream>
#include <cstdlib>
#include <ctime>
#include <cmath>

#include "../math/Colour.hpp"
#include "../math/Vector.hpp"

#include "../font/Font.hpp"
#include "../font/Fonts.hpp"

#include "../popup/Popups.hpp"
#include "../popup/QuitConfirmationPopup.hpp"

#include "Constants.hpp"
#include "../Window.hpp"
#include "../Grid.hpp"
#include "History.hpp"
#include "MenuItems.hpp"
#include "Tools.hpp"

#warning The FloodForge Level Editor is very WIP. Take caution.

Project *project = nullptr;
unsigned int currentLayer = 1;


#define clamp(x, a, b) x >= b ? b : (x <= a ? a : x)
#define min(a, b) (a < b) ? a : b
#define max(a, b) (a > b) ? a : b

void error_callback(int error, const char* description) {
	fputs(description, stderr);
}

void drawGrid(float spacing, unsigned int width, unsigned int height) {
	glBegin(GL_LINES);
	for (int x = 0; x <= width; x++) {
		glVertex2f(x * spacing - 1.0f, 1.0f);
		glVertex2f(x * spacing - 1.0f, -(height * spacing - 1.0f));
	}

	for (int y = 0; y <= height; y++) {
		glVertex2f(-1.0f, -(y * spacing - 1.0f));
		glVertex2f(width * spacing - 1.0f, -(y * spacing - 1.0f));
	}
	glEnd();
}

void drawSprite(GLuint texture, float x, float y, float width, float height) {
    glBindTexture(GL_TEXTURE_2D, texture);

    glEnable(GL_TEXTURE_2D);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	glBegin(GL_QUADS);
    
    glColor3f(1.0f, 1.0f, 1.0f);

	glTexCoord2f(0.0f, 1.0f);
	glVertex2f(x,         y);

	glTexCoord2f(1.0f, 1.0f);
	glVertex2f(x + width, y);

	glTexCoord2f(1.0f, 0.0f);
	glVertex2f(x + width, y + height);

	glTexCoord2f(0.0f, 0.0f);
	glVertex2f(x,         y + height);

	glEnd();

    glDisable(GL_TEXTURE_2D);
    glDisable(GL_BLEND);
}

void drawSprite(GLuint texture, float x, float y, float width, float height, unsigned int textureSubWidth, unsigned int textureSubHeight, unsigned int subId) {
	float uvX = float(subId % textureSubWidth) / float(textureSubWidth);
	float uvY = float(subId / textureSubWidth) / float(textureSubHeight);
	float uvWidth = 1.0 / textureSubWidth;
	float uvHeight = 1.0 / textureSubHeight;

    glBindTexture(GL_TEXTURE_2D, texture);

    glEnable(GL_TEXTURE_2D);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	glBegin(GL_QUADS);
    
    glColor3f(1.0f, 1.0f, 1.0f);

	glTexCoord2f(uvX,           uvY);            glVertex2f(x,         y);

	glTexCoord2f(uvX + uvWidth, uvY);            glVertex2f(x + width, y);

	glTexCoord2f(uvX + uvWidth, uvY + uvHeight); glVertex2f(x + width, y + height);

	glTexCoord2f(uvX,           uvY + uvHeight); glVertex2f(x,         y + height);

	glEnd();

    glDisable(GL_TEXTURE_2D);
    glDisable(GL_BLEND);
}

void applyFrustumToOrthographic(Vector2 position, float rotation, Vector2 scale, float left, float right, float bottom, float top, float nearVal, float farVal) {
    // Apply scaling
    left *= scale.x;
    right *= scale.x;
    bottom *= scale.y;
    top *= scale.y;

    // Apply position translation
    left += position.x;
    right += position.x;
    bottom += position.y;
    top += position.y;

    // Apply rotation
    float cosRot = std::cos(rotation);
    float sinRot = std::sin(rotation);

    // Create rotation matrix
    GLfloat rotationMatrix[16] = {
        cosRot,  sinRot, 0, 0,
        -sinRot, cosRot, 0, 0,
        0,       0,      1, 0,
        0,       0,      0, 1
    };

    // Apply the orthographic projection
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glOrtho(left, right, bottom, top, nearVal, farVal);

    // Apply rotation matrix
    glMultMatrixf(rotationMatrix);

    // Apply frustum based on modified projection
    // glFrustum(left, right, bottom, top, nearVal, farVal);
}

void applyFrustumToOrthographic(Vector2 position, float rotation, Vector2 scale) {
	applyFrustumToOrthographic(position, rotation, scale, -1.0f, 1.0f, -1.0f, 1.0f, 0.000f, 100.0f);
}

// void windowCloseCallback(GLFWwindow *window) {
// 	if (!History::unsavedChanges) return;

// 	int result = verifyBox("You have unsaved changes.\nAre you sure you want to quit?");

// 	if (!result) {
// 		glfwSetWindowShouldClose(window, false);
// 	}
// }

float getLayerTransparency(unsigned int layer) {
	if (currentLayer == layer) return 1.0f;
	if (currentLayer == layer + 1 || currentLayer == layer - 1) return 0.5f;

	return 0.25f;
}

std::vector<Vector2i> line(int x0, int y0, int x1, int y1) {
	std::vector<Vector2i> points;

    int dx = abs(x1 - x0);
    int dy = abs(y1 - y0);
    int sx = (x0 < x1) ? 1 : -1;
    int sy = (y0 < y1) ? 1 : -1;
    int error = dx - dy;

    while (true) {
        points.push_back(Vector2i(x0, y0));

        if (x0 == x1 && y0 == y1) {
            break;
        }

        int e2 = 2 * error;
        if (e2 > -dy) {
            error -= dy;
            x0 += sx;
        }
        if (e2 < dx) {
            error += dx;
            y0 += sy;
        }
    }

    return points;
}

int main() {
	std::cerr << "\033[38;5;13mThe FloodForge Level Editor is very WIP. Take caution.\033[0m" << std::endl;
	std::srand(std::time(0));

	glfwSetErrorCallback(error_callback);

	project = new Project("", 72, 43);

	Window *window = new Window(1024, 1024);
	window->setIcon(TEXTURE_PATH + "MainIcon.png");
	window->setTitle("FloodForge Level Editor - " + project->Name());
	window->setBackgroundColour(0.3f, 0.3f, 0.3f);
	
	if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
		std::cerr << "Failed to initialize GLAD!" << std::endl;
		return -1;
	}

	project->WindowC(window);
	// glfwSetWindowCloseCallback(window->getGLFWWindow(), windowCloseCallback);
	
	Fonts::init();
	Popups::init();

    GLuint textureSolids = loadTexture(TEXTURE_PATH + "solids.png");
    GLuint textureShortcuts = loadTexture(TEXTURE_PATH + "shortcuts.png");
    GLuint textureTools = loadTexture(TEXTURE_PATH + "tools.png");
    GLuint textureItems = loadTexture(TEXTURE_PATH + "items.png");


    bool popupVisible = false;
    double popupX;
    double popupY;

    unsigned int currentTool = 0;
    unsigned int newTool = 0;

    bool wasDrawing = false;
    unsigned int toolMode = 0;
    bool drawErasing = false;
    unsigned int drawToolX0 = 0;
    unsigned int drawToolY0 = 0;
    unsigned int rectToolMode = 0;

	Vector2 cameraOffset = Vector2(0.0f, 0.0f);
	Vector2 cameraScale = Vector2(4.0f, 4.0f);
	bool cameraPanning = false;
	Vector2 cameraPanStartMouse = Vector2(0.0f, 0.0f);
	Vector2 cameraPanStart = Vector2(0.0f, 0.0f);

	bool wasSaving = false;
	bool wasFullscreen = false;

	std::set<int> previousKeys;
	bool leftMouseDown = false;

	History::unsavedChanges = false;

	MenuItems::init(window);

	std::vector< Tool* > tools;
	tools.push_back(&toolDraw);
	tools.push_back(&toolBorder);
	tools.push_back(&toolSlope);
	tools.push_back(&toolOneWay);
	tools.push_back(&toolPoleHorizontal);
	tools.push_back(&toolPoleVertical);
	tools.push_back(&toolSpear);
	tools.push_back(&toolRock);
	tools.push_back(&toolShortcut);
	tools.push_back(&toolRoomExit);
	tools.push_back(&toolWackAMole);
	tools.push_back(&toolBatflyHive);
	tools.push_back(&toolWormgrass);
	tools.push_back(&toolGarbageWorm);
	tools.push_back(&toolLizardDen);
	tools.push_back(&toolWaterfall);

	while (window->isOpen()) {
		glfwPollEvents();

		window->GetMouse()->setCursor(CURSOR_DEFAULT);
		window->ensureFullscreen();

		int width;
		int height;
		glfwGetWindowSize(window->getGLFWWindow(), &width, &height);
		float size = min(width, height);
		float offsetX = (width * 0.5) - size * 0.5;
		float offsetY = (height * 0.5) - size * 0.5;

		Grid *grid = project->GetLayer(currentLayer);
		Mouse *mouse = window->GetMouse();

		double globalMouseX = (mouse->X() - offsetX) / size * 1024;
		double globalMouseY = (mouse->Y() - offsetY) / size * 1024;

		double screenMouseX = (globalMouseX / 1024.0) *  2.0 - 1.0;
		double screenMouseY = (globalMouseY / 1024.0) * -2.0 + 1.0;


		Mouse *customMouse = new Mouse(window->getGLFWWindow(), globalMouseX, globalMouseY);

		double mouseX = globalMouseX;
		double mouseY = globalMouseY;

		mouseX = (mouseX - 512.0) * cameraScale.x + 512.0;
		mouseY = (mouseY - 512.0) * cameraScale.y + 512.0;

		mouseX += cameraOffset.x *  512.0;
		mouseY += cameraOffset.y * -512.0;

		int tileX = int(floor(mouseX * 16 / 1024));
		int tileY = int(floor(mouseY * 16 / 1024));
		unsigned int clampedTileX = clamp(tileX, 0, -1 + (signed int) grid->Width());
		unsigned int clampedTileY = clamp(tileY, 0, -1 + (signed int) grid->Height());

		bool disableCursor = globalMouseY < window->Height() * 0.03;

		Popup *hoveringPopup = nullptr;
		for (Popup *popup : popups) {
			Rect bounds = popup->Bounds();

			if (bounds.inside(Vector2(screenMouseX, screenMouseY))) {
				hoveringPopup = popup;
				break;
			}
		}

		if (window->keyPressed(GLFW_KEY_ESCAPE)) {
			if (previousKeys.find(GLFW_KEY_ESCAPE) == previousKeys.end()) {
				if (popups.size() > 0)
					popups[0]->reject();
				else
					addPopup(new QuitConfirmationPopup(window));
			}

			previousKeys.insert(GLFW_KEY_ESCAPE);
		} else {
			previousKeys.erase(GLFW_KEY_ESCAPE);
		}

		if (window->keyPressed(GLFW_KEY_ENTER)) {
			if (previousKeys.find(GLFW_KEY_ENTER) == previousKeys.end()) {
				if (popups.size() > 0)
					popups[0]->accept();
			}

			previousKeys.insert(GLFW_KEY_ENTER);
		} else {
			previousKeys.erase(GLFW_KEY_ENTER);
		}

		if (hoveringPopup != nullptr) {
			if (mouse->Left()) {
				if (!leftMouseDown) {
					hoveringPopup->mouseClick(screenMouseX, screenMouseY);
				}

				leftMouseDown = true;
			} else {
				leftMouseDown = false;
			}
		} else {
			if (!mouse->Left()) leftMouseDown = false;
		}

		if (leftMouseDown || hoveringPopup != nullptr) {
			disableCursor = true;
			double __ = window->getMouseScrollY();
		}

		if (!disableCursor) {
			if (popupVisible) {
		        if (!window->keyPressed(GLFW_KEY_SPACE)) {
		        	popupVisible = false;
		        	currentTool = newTool;
		        }
		    } else {
		    	if (window->keyPressed(GLFW_KEY_SPACE)) {
		    		// rectToolDrawing = false;
		    		rectToolMode = 0;

		    		popupVisible = true;
		    		popupX = -1.0 + globalMouseX *  (1.0 / 512.0);
		    		popupY =  1.0 + globalMouseY * -(1.0 / 512.0);
		    		popupX += 0.25f - (currentTool % 4) * 0.125f;
		    		popupY += 0.25f - (3 - (currentTool / 4)) * 0.125f;
		    		popupX += -0.0625f;
		    		popupY += -0.0625f;
		    	}
		    }

			if (!popupVisible) {
				double scrollY = -window->getMouseScrollY();
				if (scrollY < -10.0) scrollY = -10.0;
				double zoom = std::pow(1.25, scrollY);
				
				double previousScreenMouseX = (globalMouseX / 1024.0) *  2.0 - 1.0;
				double previousScreenMouseY = (globalMouseY / 1024.0) * -2.0 + 1.0;

				Vector2 previousWorldMouse = Vector2(
					previousScreenMouseX * cameraScale.x + cameraOffset.x,
					previousScreenMouseY * cameraScale.y + cameraOffset.y
				);

				cameraScale.x *= zoom;
				cameraScale.y *= zoom;

				double globalMouseX2 = (mouse->X() - offsetX) / size * 1024;
				double globalMouseY2 = (mouse->Y() - offsetY) / size * 1024;
				// double screenMouseX = (globalMouseX / 1024.0) *  2.0 - 1.0;
				// double screenMouseY = (globalMouseY / 1024.0) * -2.0 + 1.0;

				Vector2 worldMouse = Vector2(
					screenMouseX * cameraScale.x + cameraOffset.x,
					screenMouseY * cameraScale.y + cameraOffset.y
				);

				cameraOffset.x += previousWorldMouse.x - worldMouse.x;
				cameraOffset.y += previousWorldMouse.y - worldMouse.y;


				if (mouse->Middle()) {
					if (!cameraPanning) {
						cameraPanStart.x = cameraOffset.x;
						cameraPanStart.y = cameraOffset.y;
						cameraPanStartMouse.x = globalMouseX;
						cameraPanStartMouse.y = globalMouseY;
					}

					cameraOffset.x = cameraPanStart.x + cameraScale.x * (cameraPanStartMouse.x - globalMouseX) / 512.0;
					cameraOffset.y = cameraPanStart.y + cameraScale.y * (cameraPanStartMouse.y - globalMouseY) / -512.0;

					cameraPanning = true;
				} else {
					cameraPanning = false;
				}

				if (currentTool < tools.size()) {
					// Tools
					Tool *tool = tools[currentTool];

					if (mouse->Left()) {
						if (!wasDrawing) {
							wasDrawing = true;
							drawErasing = false;
							toolMode = 0;
							if (window->keyPressed(GLFW_KEY_LEFT_CONTROL)) toolMode += 1;
							if (window->keyPressed(GLFW_KEY_LEFT_SHIFT)) toolMode += 2;

							drawToolX0 = clampedTileX;
							drawToolY0 = clampedTileY;
						}

						if (!drawErasing) {
							if (toolMode == 0) {
								tool->left(grid, tileX, tileY);
							}
						}
					} else {
						if (!drawErasing) {
							if (wasDrawing) {
								if (toolMode == 1) {
									int x0 = min(drawToolX0, clampedTileX);
									int y0 = min(drawToolY0, clampedTileY);
									int x1 = max(drawToolX0, clampedTileX);
									int y1 = max(drawToolY0, clampedTileY);

									for (int x = x0; x <= x1; x++) {
										for (int y = y0; y <= y1; y++) {
											tool->left(grid, x, y);
										}
									}
								} else if (toolMode == 2) {
									std::vector<Vector2i> points = line(drawToolX0, drawToolY0, clampedTileX, clampedTileY);

									for (Vector2i point : points) {
										tool->left(grid, point.x, point.y);
									}

									points.clear();
								}
							}

							wasDrawing = false;
						}
					}

					if (mouse->Right()) {
						if (!wasDrawing) {
							wasDrawing = true;
							drawErasing = true;
							toolMode = 0;
							if (window->keyPressed(GLFW_KEY_LEFT_CONTROL)) toolMode += 1;
							if (window->keyPressed(GLFW_KEY_LEFT_SHIFT)) toolMode += 2;

							drawToolX0 = clampedTileX;
							drawToolY0 = clampedTileY;
						}

						if (drawErasing) {
							if (toolMode == 0) {
								tool->right(grid, tileX, tileY);
							}
						}
					} else {
						if (drawErasing) {
							if (wasDrawing) {
								if (toolMode == 1) {
									int x0 = min(drawToolX0, clampedTileX);
									int y0 = min(drawToolY0, clampedTileY);
									int x1 = max(drawToolX0, clampedTileX);
									int y1 = max(drawToolY0, clampedTileY);
									for (int x = x0; x <= x1; x++) {
										for (int y = y0; y <= y1; y++) {
											tool->right(grid, x, y);
										}
									}
								} else if (toolMode == 2) {
									std::vector<Vector2i> points = line(drawToolX0, drawToolY0, clampedTileX, clampedTileY);

									for (Vector2i point : points) {
										tool->right(grid, point.x, point.y);
									}

									points.clear();
								}
							}

							wasDrawing = false;
						}
					}
				}
			}
		}

		if (window->keyPressed(GLFW_KEY_S)) {
			if (!wasSaving) {
				project->save();
				History::unsavedChanges = false;
				std::cout << "Saved!" << std::endl;
			}

			wasSaving = true;
		} else {
			wasSaving = false;
		}

		if (window->keyPressed(GLFW_KEY_F11)) {
			if (!wasFullscreen) {
				window->toggleFullscreen();
			}

			wasFullscreen = true;
		} else {
			wasFullscreen = false;
		}

		// Draw

		glViewport(0, 0, width, height);

		window->clear();
		glDisable(GL_DEPTH_TEST);

		glColor3f(0.0f, 0.0f, 0.0f);
		fillRect(-1.0, -1.0, 1.0, 1.0);

		glViewport(offsetX, offsetY, size, size);

		glColor3f(0.3f, 0.3f, 0.3f);
		fillRect(-1.0, -1.0, 1.0, 1.0);

		applyFrustumToOrthographic(cameraOffset, 0.0f, cameraScale);

		float tileSize = 0.125f;

		glColor3f(0.5f, 0.5f, 0.5f);
		drawGrid(tileSize, grid->Width(), grid->Height());

		glEnable(GL_BLEND);
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
		drawTexturedGrid(project->GetLayer(3), tileSize, textureSolids, textureShortcuts, textureItems, getLayerTransparency(3));
		drawTexturedGrid(project->GetLayer(2), tileSize, textureSolids, textureShortcuts, textureItems, getLayerTransparency(2));
		drawTexturedGrid(project->GetLayer(1), tileSize, textureSolids, textureShortcuts, textureItems, getLayerTransparency(1));

		glColor3f(0.75f, 0.75f, 0.75f);
		strokeRect(
			tileSize * 12.0f - 1.0f,
			-(tileSize * 3.0f - 1.0f),
			tileSize * (grid->Width() - 12.0f) - 1.0f,
			-(tileSize * (grid->Height() - 5.0f) - 1.0f)
		);

        if (!popupVisible && !disableCursor) { // Selection
	        glColor3f(0.0f, 1.0f, 0.0f);

	        if (wasDrawing && toolMode == 1) {
	        	int x0 = min(tileX, drawToolX0);
				int y0 = min(tileY, drawToolY0);
				int x1 = max(tileX, drawToolX0);
				int y1 = max(tileY, drawToolY0);

				x0 = clamp(x0, 0, -1 + (signed int) grid->Width());
				y0 = clamp(y0, 0, -1 + (signed int) grid->Height());
				x1 = clamp(x1, 0, -1 + (signed int) grid->Width());
				y1 = clamp(y1, 0, -1 + (signed int) grid->Height());


		        strokeRect(
					x0 * tileSize - 1.0f,
					-(y0 * tileSize - 1.0f),
					(x1 + 1) * tileSize - 1.0f,
					-((y1 + 1) * tileSize - 1.0f)
				);
		    } else if (wasDrawing && toolMode == 2) {
				std::vector<Vector2i> points = line(drawToolX0, drawToolY0, clampedTileX, clampedTileY);

				for (Vector2i point : points) {
			        strokeRect(
						point.x * tileSize - 1.0f,
						-(point.y * tileSize - 1.0f),
						(point.x + 1) * tileSize - 1.0f,
						-((point.y + 1) * tileSize - 1.0f)
					);
				}
	        } else {
		        strokeRect(
					tileX * tileSize - 1.0f,
					-(tileY * tileSize - 1.0f),
					(tileX + 1) * tileSize - 1.0f,
					-((tileY + 1) * tileSize - 1.0f)
				);
		    }
	    }

	    // UI
		applyFrustumToOrthographic(Vector2(0.0f, 0.0f), 0.0f, Vector2(1.0f, 1.0f));

        if (popupVisible && !disableCursor) {
	        glColor3f(0.0f, 0.0f, 0.0f);
	        fillRect(-0.25f + popupX, -0.25f + popupY, 0.25f + popupX, 0.25f + popupY);

	        glColor3f(1.0f, 1.0f, 1.0f);
	        strokeRect(-0.25f + popupX, -0.25f + popupY, 0.25f + popupX, 0.25f + popupY);

	        // Draw tools
	        for (int i = 0; i < 16; i++) {
	        	double offsetX = (i % 4) * 0.125;
	        	double offsetY = (i / 4) * 0.125;
	        	drawSprite(textureTools, -0.25f + popupX + offsetX, 0.25f + popupY - offsetY, 0.125f, -0.125f, 4, 6, tools[i]->iconId);
	        }

	        // Draw selection
	        glLineWidth(3);

	        glColor3f(1.0f, 0.0f, 0.0f);
	        strokeRect(
				-0.25f + popupX + (currentTool % 4) * 0.125f,
				-0.25f + popupY + (3 - currentTool / 4) * 0.125f,
				-0.125f + popupX + (currentTool % 4) * 0.125f,
				-0.125f + popupY + (3 - currentTool / 4) * 0.125f
			);

	        float cx = -1.0 + globalMouseX *  (1.0 / 512.0);
	        float cy =  1.0 + globalMouseY * -(1.0 / 512.0);
	        int popupSelectionX = round((cx - popupX) * 8.0f + 1.5f);
	        int popupSelectionY = round((cy - popupY) * 8.0f + 1.5f);
	        popupSelectionX = clamp(popupSelectionX, 0, 3);
	        popupSelectionY = clamp(popupSelectionY, 0, 3);
	        newTool = popupSelectionX + (3 - popupSelectionY) * 4;
	        glColor3f(0.0f, 1.0f, 0.0f);
	        strokeRect(-0.25f + popupX + popupSelectionX * 0.125f, -0.25f + popupY + popupSelectionY * 0.125f, -0.125f + popupX + popupSelectionX * 0.125f, -0.125f + popupY + popupSelectionY * 0.125f);

	        glLineWidth(1);
		}

		MenuItems::draw(customMouse);
		
		for (Popup *popup : popups) {
			Rect bounds = popup->Bounds();
			bool mouseInside = bounds.inside(screenMouseX, screenMouseY);

			popup->draw(screenMouseX, screenMouseY, mouseInside);
		}

		// glPushAttrib(GL_ENABLE_BIT); 

		// glLineStipple(1, 0xF000);
		// glEnable(GL_LINE_STIPPLE);
		// glBegin(GL_LINES);
		// glVertex3f(-.5,.5,-.5);
		// glVertex3f(.5,.5,-.5);
		// glEnd();

		// glPopAttrib();

		window->render();

		delete customMouse;
		
		Popups::cleanup();
	}

	MenuItems::terminate();

	delete project;
	delete window;

	Fonts::cleanup();

	glfwTerminate();

	return 0;
}