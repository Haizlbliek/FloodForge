#include "Window.hpp"

void Mouse::setCursor(unsigned int cursorMode) {
	Window *window = static_cast<Window*>(glfwGetWindowUserPointer(glfwWindow));

	if (window == nullptr) return;

	glfwSetCursor(glfwWindow, window->getCursor(cursorMode));
}