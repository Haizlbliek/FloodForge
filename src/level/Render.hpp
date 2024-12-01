#ifndef RENDER_HPP
#define RENDER_HPP

#include "../gl.h"

#include "Project.hpp"
#include "../Grid.hpp"
#include "../Window.hpp"
#include "../Utils.hpp"

class Render {
	public:
		static void render(Project *project, const char *path) {
			Grid *grid = project->GetLayer(1);

			unsigned int width = 70;//grid->Width();
			unsigned int height = 40;//grid->Height();

			Window *window = new Window(width * 20, height * 20);
			window->setTitle("Rendering");
			window->setBackgroundColour(1.0f, 1.0f, 1.0f);

			window->clear();

			GLuint solids = loadTexture(TEXTURE_PATH + "renderSolids.png");
			GLuint shortcuts = loadTexture(TEXTURE_PATH + "shortcuts.png");
			glColor3f(0.4745f, 0.0f, 0.0f);
        	drawTexturedGrid(
				grid,
				2.0 / (double) width,
				2.0 / (double) height,
				solids,
				shortcuts,

				0, 0,
				70, 40
			);

        	window->render();

			while (window->isOpen()) {
				glfwPollEvents();
				// window->close();
			}

        	window->render();

			saveImage(window->getGLFWWindow(), path);

			delete window;
		}
};

#endif