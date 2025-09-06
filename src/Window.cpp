#include "Window.hpp"

void Mouse::setCursor(unsigned int cursorMode) {
	Window *window = static_cast<Window*>(glfwGetWindowUserPointer(glfwWindow));

	if (window == nullptr) return;

	glfwSetCursor(glfwWindow, window->getCursor(cursorMode));
}

void Mouse::update(GLFWwindow *glfwWindow, double x, double y) {
	if (this->glfwWindow != glfwWindow) return;

	this->x = x;
	this->y = y;
}

void Mouse::updateLastPressed() {
	lastFrameLeft = Left();
	lastFrameMiddle = Middle();
	lastFrameRight = Right();
}

bool Mouse::Left() const {
	return GLFW_PRESS == glfwGetMouseButton(glfwWindow, GLFW_MOUSE_BUTTON_LEFT);
}

bool Mouse::Middle() const {
	return GLFW_PRESS == glfwGetMouseButton(glfwWindow, GLFW_MOUSE_BUTTON_MIDDLE);
}

bool Mouse::Right() const {
	return GLFW_PRESS == glfwGetMouseButton(glfwWindow, GLFW_MOUSE_BUTTON_RIGHT);
}

bool Mouse::JustLeft() const {
	return (!lastFrameLeft) && (GLFW_PRESS == glfwGetMouseButton(glfwWindow, GLFW_MOUSE_BUTTON_LEFT));
}

bool Mouse::JustMiddle() const {
	return (!lastFrameMiddle) && (GLFW_PRESS == glfwGetMouseButton(glfwWindow, GLFW_MOUSE_BUTTON_MIDDLE));
}

bool Mouse::JustRight() const {
	return (!lastFrameRight) && (GLFW_PRESS == glfwGetMouseButton(glfwWindow, GLFW_MOUSE_BUTTON_RIGHT));
}

void Mouse::copyPressed(Mouse &otherMouse) {
	this->lastFrameLeft = otherMouse.lastFrameLeft;
	this->lastFrameMiddle = otherMouse.lastFrameMiddle;
	this->lastFrameRight = otherMouse.lastFrameRight;
}



Window::Window() : Window(1024, 1024) {}

Window::Window(int width, int height) : width(width), height(height) {
	if (!glfwInit()) exit(EXIT_FAILURE);

	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

	glfwWindow = glfwCreateWindow(width, height, title.c_str(), nullptr, nullptr);
	if (!glfwWindow) {
		glfwTerminate();
		exit(EXIT_FAILURE);
	}
	glfwMakeContextCurrent(glfwWindow);
	glfwSwapInterval(1);

	mouse = new Mouse(glfwWindow);

	glfwSetWindowUserPointer(glfwWindow, this);

	glfwSetKeyCallback(glfwWindow, Window::keyCallback);
	glfwSetCursorPosCallback(glfwWindow, Window::mouseCallback);
	glfwSetInputMode(glfwWindow, GLFW_LOCK_KEY_MODS, GLFW_TRUE);
	glfwSetScrollCallback(glfwWindow, Window::scrollCallback);

	backgroundColour = Colour(0.0, 0.0, 0.0, 1.0);

	scrollXAccumulator = 0.0;
	scrollYAccumulator = 0.0;
	fullscreen = false;

	cursorDefault = glfwCreateStandardCursor(GLFW_ARROW_CURSOR);
	cursorPointer = glfwCreateStandardCursor(GLFW_HAND_CURSOR);
}

Window::~Window() {
	glfwDestroyWindow(glfwWindow);
	
	delete mouse;
}

void Window::close() const {
	glfwSetWindowShouldClose(glfwWindow, GLFW_TRUE);
}

void Window::terminate() const {
	glfwDestroyWindow(glfwWindow);
	glfwTerminate();
}

bool Window::isOpen() const {
	return !glfwWindowShouldClose(glfwWindow);
}

void Window::clear() const {
	glClearColor(
		backgroundColour.r,
		backgroundColour.g,
		backgroundColour.b,
		1.0
	);

	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
}

void Window::render() const {
	glfwMakeContextCurrent(glfwWindow);
	glfwSwapBuffers(glfwWindow);
}

void Window::setTitle(const std::string title) {
	setTitle(title.c_str());
}

void Window::setTitle(const char *title) {
	this->title = title;
	glfwSetWindowTitle(glfwWindow, title);
}

Mouse *Window::GetMouse() const {
	return mouse;
}

void Window::setBackgroundColour(const Colour backgroundColour) {
	this->backgroundColour.copy(backgroundColour);
}

void Window::setBackgroundColour(const float r, const float g, const float b) {
	backgroundColour.r = r;
	backgroundColour.g = g;
	backgroundColour.b = b;
}

bool Window::keyPressed(uint16_t key) {
	return GLFW_PRESS == glfwGetKey(glfwWindow, key);
}

bool Window::modifierPressed(uint16_t modifier) {
	switch (modifier) {
		case GLFW_MOD_CONTROL:
			return keyPressed(GLFW_KEY_LEFT_CONTROL) || keyPressed(GLFW_KEY_RIGHT_CONTROL) || keyPressed(GLFW_KEY_LEFT_SUPER) || keyPressed(GLFW_KEY_RIGHT_SUPER);

		case GLFW_MOD_SHIFT:
			return (keyPressed(GLFW_KEY_LEFT_SHIFT) || keyPressed(GLFW_KEY_RIGHT_SHIFT)) ^ capslock;

		case GLFW_MOD_ALT:
			return keyPressed(GLFW_KEY_LEFT_ALT) || keyPressed(GLFW_KEY_RIGHT_ALT);
	}

	return false;
}

double Window::getMouseScrollX() {
	double scrollX = scrollXAccumulator;

	scrollXAccumulator = 0.0;

	return scrollX;
}

double Window::getMouseScrollY() {
	double scrollY = scrollYAccumulator;

	scrollYAccumulator = 0.0;

	return scrollY;
}

void Window::setIcon(std::filesystem::path path) {
	GLFWimage images[1]; 
	images[0].pixels = stbi_load(path.generic_u8string().c_str(), &images[0].width, &images[0].height, 0, 4); //rgba channels 
	if (!images[0].pixels) {
		Logger::error("Failed to load icon: ", path);
		return;
	}

	glfwSetWindowIcon(glfwWindow, 1, images); 
	stbi_image_free(images[0].pixels);
}

void Window::setFullscreen(bool fullscreen) {
	this->fullscreen = fullscreen;

	GLFWmonitor *monitor = glfwGetPrimaryMonitor();
	const GLFWvidmode *mode = glfwGetVideoMode(monitor);

	if (fullscreen) {
		glfwSetWindowMonitor(glfwWindow, nullptr, 0, 0, mode->width, mode->height, 0);
		glfwSetWindowAttrib(glfwWindow, GLFW_DECORATED, GLFW_FALSE);
	} else {
		int xpos = (mode->width - 1024) / 2;
		int ypos = (mode->height - 1024) / 2;

		glfwSetWindowMonitor(glfwWindow, nullptr, xpos, ypos, 1024, 1024, 60);
		glfwSetWindowAttrib(glfwWindow, GLFW_DECORATED, GLFW_TRUE);
	}
}

void Window::toggleFullscreen() {
	setFullscreen(!fullscreen);
}

void Window::ensureFullscreen() {
	if (fullscreen) setFullscreen(fullscreen);
}

void Window::addKeyCallback(void *object, std::function<void(void*, int, int)> callback) {
	keyCallbacks.push_back(std::pair<void*, std::function<void(void*, int, int)>> { object, callback });
}

void Window::removeKeyCallback(void *object, std::function<void(void*, int, int)> callback) {
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

void Window::addScrollCallback(void *object, std::function<void(void*, double, double)> callback) {
	scrollCallbacks.push_back(std::pair<void*, std::function<void(void*, double, double)>> { object, callback });
}

void Window::removeScrollCallback(void *object, std::function<void(void*, double, double)> callback) {
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

void Window::clearCallbacks(void *object) {
	keyCallbacks.erase(
		std::remove_if(
			keyCallbacks.begin(),
			keyCallbacks.end(),
			[&object](const std::pair<void*, std::function<void(void*, int, int)>>& p) {
				return p.first == object;
			}
		),
		keyCallbacks.end()
	);
	
	scrollCallbacks.erase(
		std::remove_if(
			scrollCallbacks.begin(),
			scrollCallbacks.end(),
			[&object](const std::pair<void*, std::function<void(void*, double, double)>>& p) {
				return p.first == object;
			}
		),
		scrollCallbacks.end()
	);
}

std::string Window::getClipboard() {
	const char* clipboardText = glfwGetClipboardString(glfwWindow);

	return clipboardText ? std::string(clipboardText) : std::string();
}

GLFWcursor *Window::getCursor(unsigned int cursor) {
	switch (cursor) {
		case CURSOR_DEFAULT: return cursorDefault;
		case CURSOR_POINTER: return cursorPointer;
	}

	return nullptr;
}

GLFWwindow *Window::getGLFWWindow() const { return glfwWindow;}

void Window::mouseCallback(GLFWwindow *glfwWindow, double x, double y) {
	// Retrieve the Window instance from the user pointer
	Window* window = static_cast<Window*>(glfwGetWindowUserPointer(glfwWindow));

	if (!window) return;

	window->mouse->update(glfwWindow, x, y);
}

void Window::scrollCallback(GLFWwindow *glfwWindow, double scrollX, double scrollY) {
	Window* window = static_cast<Window*>(glfwGetWindowUserPointer(glfwWindow));

	if (!window) return;

	window->scrollXAccumulator += scrollX;
	window->scrollYAccumulator += scrollY;

	for (std::pair<void*, std::function<void(void*, double, double)>> callback : window->scrollCallbacks) {
		callback.second(callback.first, scrollX, scrollY);
	}
}

void Window::keyCallback(GLFWwindow *glfwWindow, int key, int scancode, int action, int mods) {
	Window* window = static_cast<Window*>(glfwGetWindowUserPointer(glfwWindow));

	if (!window) return;
	
	if (action == GLFW_PRESS || action == GLFW_REPEAT) {
		window->capslock = (mods & GLFW_MOD_CAPS_LOCK) > 0;
	}

	for (std::pair<void*, std::function<void(void*, int, int)>> callback : window->keyCallbacks) {
		if (!callback.second) {
			Logger::info("INVALID CALLBACK");
			continue;
		}

		callback.second(callback.first, action, key);
	}
}
