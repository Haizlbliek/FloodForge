#include "gl.h"
#include "stb_image.h"

#include <string>
#include <vector>
#include <iostream>
#include <utility>
#include <functional>
#include <algorithm>

#include "math/Colour.hpp"
// #include "Utils.hpp"

#ifndef WINDOW_HPP
#define WINDOW_HPP

#define CURSOR_DEFAULT 0
#define CURSOR_POINTER 1

class Mouse {
	public:
		Mouse(GLFWwindow *glfwWindow, double x, double y)
		 : x(x),
		   y(y),
		   glfwWindow(glfwWindow) {
		}

		Mouse(GLFWwindow *glfwWindow)
		 : x(0.0),
		   y(0.0),
		   glfwWindow(glfwWindow) {
		}

		double X() { return x; }
		double Y() { return y; }

		void update(GLFWwindow *glfwWindow, double x, double y) {
			if (this->glfwWindow != glfwWindow) return;

			this->x = x;
			this->y = y;
		}

		bool Left() {
			return GLFW_PRESS == glfwGetMouseButton(glfwWindow, GLFW_MOUSE_BUTTON_LEFT);
		}

		bool Right() {
			return GLFW_PRESS == glfwGetMouseButton(glfwWindow, GLFW_MOUSE_BUTTON_RIGHT);
		}

		bool Middle() {
			return GLFW_PRESS == glfwGetMouseButton(glfwWindow, GLFW_MOUSE_BUTTON_MIDDLE);
		}

		void setCursor(unsigned int cursorMode);

	private:
		double x;
		double y;

		GLFWwindow *glfwWindow;
};

class Window {
	public:
		Window() : Window(1024, 1024) {
		}

		Window(int width, int height)
		 : width(width),
		   height(height) {
			if (!glfwInit()) exit(EXIT_FAILURE);

			// glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);
		    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
		    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
			glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

		    glfwWindow = glfwCreateWindow(width, height, title.c_str(), NULL, NULL);
			if (!glfwWindow) {
				glfwTerminate();
				exit(EXIT_FAILURE);
			}
			glfwMakeContextCurrent(glfwWindow);
			glfwSwapInterval(1);
			// InitOpenGLExtensions();

			mouse = new Mouse(glfwWindow);

			// if (glfwRawMouseMotionSupported())
			// 	glfwSetInputMode(glfwWindow, GLFW_RAW_MOUSE_MOTION, GLFW_TRUE);

			glfwSetWindowUserPointer(glfwWindow, this);

		    glfwSetKeyCallback(glfwWindow, Window::keyCallback);
		    glfwSetCursorPosCallback(glfwWindow, Window::mouseCallback);

		    // glClearColor(0.0, 0.0, 0.0, 1.0);
	        // glViewport(0, 0, width, height);

	        backgroundColour = Colour(0.0, 0.0, 0.0, 1.0);

			glfwSetScrollCallback(glfwWindow, Window::scrollCallback);

			scrollXAccumulator = 0.0;
			scrollYAccumulator = 0.0;
		   	fullscreen = false;

		   	cursorDefault = glfwCreateStandardCursor(GLFW_ARROW_CURSOR);
		   	cursorPointer = glfwCreateStandardCursor(GLFW_HAND_CURSOR);
		}

		~Window() {
			glfwDestroyWindow(glfwWindow);
			
			delete mouse;
		}

		void close() const {
			glfwSetWindowShouldClose(glfwWindow, GLFW_TRUE);
		}

		void terminate() const {
			glfwDestroyWindow(glfwWindow);
		    glfwTerminate();
		}

		bool isOpen() const {
			return !glfwWindowShouldClose(glfwWindow);
		}

		void clear() const {
		    glClearColor(
		                 backgroundColour.R(),
		                 backgroundColour.G(),
		                 backgroundColour.B(),
		                 1.0
		                );

	        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		}

		void render() const {
			glfwMakeContextCurrent(glfwWindow);
	        glfwSwapBuffers(glfwWindow);
		}

		void setTitle(const std::string title) {
			setTitle(title.c_str());
		}

		void setTitle(const char *title) {
			this->title = title;
			glfwSetWindowTitle(glfwWindow, title);
		}

		Mouse *GetMouse() const {
			return mouse;
		}

		void setBackgroundColour(const Colour backgroundColour) {
			this->backgroundColour.copy(backgroundColour);
		}

		void setBackgroundColour(const float r, const float g, const float b) {
			backgroundColour.R(r);
			backgroundColour.G(g);
			backgroundColour.B(b);
		}

		bool keyPressed(uint16_t key) {
			return GLFW_PRESS == glfwGetKey(glfwWindow, key);
		}

		double getMouseScrollX() {
			double scrollX = scrollXAccumulator;

			scrollXAccumulator = 0.0;

			return scrollX;
		}

		double getMouseScrollY() {
			double scrollY = scrollYAccumulator;

			scrollYAccumulator = 0.0;

			return scrollY;
		}

		void setIcon(std::string path) {
			GLFWimage images[1]; 
			images[0].pixels = stbi_load(path.c_str(), &images[0].width, &images[0].height, 0, 4); //rgba channels 
			glfwSetWindowIcon(glfwWindow, 1, images); 
			stbi_image_free(images[0].pixels);
		}

		void setFullscreen(bool fullscreen) {
			this->fullscreen = fullscreen;

			GLFWmonitor *monitor = glfwGetPrimaryMonitor();
			const GLFWvidmode *mode = glfwGetVideoMode(monitor);

			if (fullscreen) {
				glfwSetWindowMonitor(glfwWindow, nullptr, 0, 0, mode->width, mode->height, 0);
				glfwSetWindowAttrib(glfwWindow, GLFW_DECORATED, GLFW_FALSE);
			} else {
				int xpos = (mode->width - 1024) / 2;
				int ypos = (mode->height - 1024) / 2;

				glfwSetWindowMonitor(glfwWindow, NULL, xpos, ypos, 1024, 1024, 60);
				glfwSetWindowAttrib(glfwWindow, GLFW_DECORATED, GLFW_TRUE);
			}
		}

		void toggleFullscreen() {
			setFullscreen(!fullscreen);
		}

		void ensureFullscreen() {
			if (fullscreen) setFullscreen(fullscreen);
		}

		void addKeyCallback(void *object, std::function<void(void*, int, int)> callback) {
			keyCallbacks.push_back(std::pair<void*, std::function<void(void*, int, int)>> { object, callback });
		}

		void removeKeyCallback(void *object, std::function<void(void*, int, int)> callback) {
			auto pair = std::pair<void*, std::function<void(void*, int, int)>> { object, callback };
	
			keyCallbacks.erase(
				std::remove_if(
					keyCallbacks.begin(),
					keyCallbacks.end(),
					[&pair](const std::pair<void*, std::function<void(void*, int, int)>>& p) {
						return p.first == pair.first && p.second.target<void(void*, int, int)>() == pair.second.target<void(void*, int, int)>();
					}
				),
				keyCallbacks.end()
			);
		}

		void addScrollCallback(void *object, std::function<void(void*, double, double)> callback) {
			scrollCallbacks.push_back(std::pair<void*, std::function<void(void*, double, double)>> { object, callback });
		}

		void removeScrollCallback(void *object, std::function<void(void*, double, double)> callback) {
			auto pair = std::pair<void*, std::function<void(void*, double, double)>> { object, callback };
	
			scrollCallbacks.erase(
				std::remove_if(
					scrollCallbacks.begin(),
					scrollCallbacks.end(),
					[&pair](const std::pair<void*, std::function<void(void*, double, double)>>& p) {
						return p.first == pair.first && p.second.target<void(void*, double, double)>() == pair.second.target<void(void*, double, double)>();
					}
				),
				scrollCallbacks.end()
			);
		}

		std::string getClipboard() {
			const char* clipboardText = glfwGetClipboardString(glfwWindow);

			return clipboardText ? std::string(clipboardText) : std::string();
		}


		GLFWcursor *getCursor(unsigned int cursor) {
			switch (cursor) {
				case CURSOR_DEFAULT: return cursorDefault;
				case CURSOR_POINTER: return cursorPointer;
			}

			return nullptr;
		}

		GLFWwindow *getGLFWWindow() const { return glfwWindow;}

		int Width() const { return width; }
		int Height() const { return height; }

	private:
		static void mouseCallback(GLFWwindow *glfwWindow, double x, double y) {
			// Retrieve the Window instance from the user pointer
			Window* window = static_cast<Window*>(glfwGetWindowUserPointer(glfwWindow));

			if (!window) return;

			window->mouse->update(glfwWindow, x, y);
		}

		static void scrollCallback(GLFWwindow *glfwWindow, double scrollX, double scrollY) {
			Window* window = static_cast<Window*>(glfwGetWindowUserPointer(glfwWindow));

			if (!window) return;

			window->scrollXAccumulator += scrollX;
			window->scrollYAccumulator += scrollY;

			for (std::pair<void*, std::function<void(void*, double, double)>> callback : window->scrollCallbacks) {
				callback.second(callback.first, scrollX, scrollY);
			}
		}

		static void keyCallback(GLFWwindow *glfwWindow, int key, int scancode, int action, int mods) {
			Window* window = static_cast<Window*>(glfwGetWindowUserPointer(glfwWindow));

			if (!window) return;

			for (std::pair<void*, std::function<void(void*, int, int)>> callback : window->keyCallbacks) {
				callback.second(callback.first, action, key);
			}
		}
		
		GLFWwindow *glfwWindow;

		std::string title;

		int width;
		int height;

		Mouse *mouse;

		Colour backgroundColour;

		double scrollXAccumulator;
		double scrollYAccumulator;

		bool fullscreen;

		GLFWcursor *cursorDefault;
		GLFWcursor *cursorPointer;

		std::vector<std::pair<void*, std::function<void(void*, int, int)>>> keyCallbacks;
		std::vector<std::pair<void*, std::function<void(void*, double, double)>>> scrollCallbacks;
};

#endif