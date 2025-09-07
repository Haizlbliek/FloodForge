#pragma once

#include "gl.h"
#include "stb_image.h"

#include <string>
#include <vector>
#include <iostream>
#include <utility>
#include <functional>
#include <algorithm>
#include <filesystem>
#include <set>

#include "Logger.hpp"
#include "math/Colour.hpp"
// #include "Utils.hpp"

#define CURSOR_DEFAULT 0
#define CURSOR_POINTER 1

class Mouse {
	public:
		Mouse(GLFWwindow *glfwWindow, double x, double y) : x(x), y(y), glfwWindow(glfwWindow) {
		}

		Mouse(GLFWwindow *glfwWindow) : x(0.0), y(0.0), glfwWindow(glfwWindow) {
		}

		double X() { return x; }
		double Y() { return y; }

		void update(GLFWwindow *glfwWindow, double x, double y);

		void updateLastPressed();

		bool Left() const;

		bool Middle() const;

		bool Right() const;

		bool JustLeft() const;

		bool JustMiddle() const;

		bool JustRight() const;

		bool Moved() const;

		void copyPressed(Mouse &otherMouse);

		void setCursor(unsigned int cursorMode);

	private:
		double x;
		double y;

		bool moved;
		bool lastFrameLeft;
		bool lastFrameMiddle;
		bool lastFrameRight;

		GLFWwindow *glfwWindow;
};

class Window {
	public:
		Window();

		Window(int width, int height);

		~Window();

		void close() const;

		void terminate() const;

		bool isOpen() const;

		void clear() const;

		void render() const;

		void setTitle(const std::string title);

		void setTitle(const char *title);

		Mouse *GetMouse() const;

		void setBackgroundColour(const Colour backgroundColour);

		void setBackgroundColour(const float r, const float g, const float b);

		bool keyPressed(uint16_t key);

		bool modifierPressed(uint16_t modifier);

		double getMouseScrollX();

		double getMouseScrollY();

		void setIcon(std::filesystem::path path);

		void setFullscreen(bool fullscreen);

		void toggleFullscreen();

		void ensureFullscreen();

		void addKeyCallback(void *object, std::function<void(void*, int, int)> callback);

		void removeKeyCallback(void *object, std::function<void(void*, int, int)> callback);

		void addScrollCallback(void *object, std::function<void(void*, double, double)> callback);

		void removeScrollCallback(void *object, std::function<void(void*, double, double)> callback);

		void clearCallbacks(void *object);

		std::string getClipboard();

		bool justPressed(int key);


		GLFWcursor *getCursor(unsigned int cursor);

		GLFWwindow *getGLFWWindow() const;

		int Width() const { return width; }
		int Height() const { return height; }

	private:
		static void mouseCallback(GLFWwindow *glfwWindow, double x, double y);

		static void scrollCallback(GLFWwindow *glfwWindow, double scrollX, double scrollY);

		static void keyCallback(GLFWwindow *glfwWindow, int key, int scancode, int action, int mods);

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

		bool capslock;

		std::set<int> previousKeys;
};