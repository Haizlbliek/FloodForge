#pragma once

struct UIMouse {
	UIMouse(double x, double y);

	double x;
	double y;
	bool leftMouse;
	bool lastLeftMouse;
	bool rightMouse;
	bool lastRightMouse;
	bool disabled;

	bool justClicked();
};