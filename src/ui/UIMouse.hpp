#pragma once

struct UIMouse {
	UIMouse(double x, double y);

	double lastX;
	double lastY;
	double x;
	double y;
	bool leftMouse;
	bool lastLeftMouse;
	bool rightMouse;
	bool lastRightMouse;
	bool middleMouse;
	bool lastMiddleMouse;
	bool disabled;

	bool moved();

	bool justClicked();
};