#pragma once

struct UIMouse {
	UIMouse(double x, double y);

	double x;
	double y;
	bool leftMouse;
	bool lastLeftMouse;
	bool disabled;

	bool justClicked();
};