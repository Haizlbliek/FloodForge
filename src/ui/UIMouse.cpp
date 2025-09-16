#include "UIMouse.hpp"

UIMouse::UIMouse(double x, double y) {
	this->x = x;
	this->y = y;
	this->disabled = false;
}

bool UIMouse::moved() {
	return this->lastX != this->x && this->lastY != this->y;
}

bool UIMouse::justClicked() {
	return !disabled && leftMouse && !lastLeftMouse;
}