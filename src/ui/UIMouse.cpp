#include "UIMouse.hpp"

UIMouse::UIMouse(double x, double y) {
	this->x = x;
	this->y = y;
	this->disabled = false;
}

bool UIMouse::justClicked() {
	return !disabled && leftMouse && !lastLeftMouse;
}