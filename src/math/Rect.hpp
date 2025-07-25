#pragma once

#include "Vector.hpp"

class Rect {
	public:
		Rect() {
			this->x0 = 0.0;
			this->y0 = 0.0;
			this->x1 = 0.0;
			this->y1 = 0.0;
		}

		Rect(double x0, double y0, double x1, double y1) {
			this->x0 = std::min(x0, x1);
			this->y0 = std::min(y0, y1);
			this->x1 = std::max(x0, x1);
			this->y1 = std::max(y0, y1);
		}

		Rect(const Vector2 point0, const Vector2 point1) {
			x0 = std::min(point0.x, point1.x);
			y0 = std::min(point0.y, point1.y);
			x1 = std::max(point0.x, point1.x);
			y1 = std::max(point0.y, point1.y);
		}

		bool inside(Vector2 point) {
			return point.x >= x0 && point.y >= y0 && point.x <= x1 && point.y <= y1;
		}

		bool inside(double x, double y) {
			return x >= x0 && y >= y0 && x <= x1 && y <= y1;
		}

		Rect &offset(Vector2 offset) {
			x0 += offset.x;
			x1 += offset.x;
			y0 += offset.y;
			y1 += offset.y;
			
			return *this;
		}

		static Rect fromSize(double x, double y, double width, double height) {
			return Rect(x, y, x + width, y + height);
		}

		double x0;
		double y0;
		double x1;
		double y1;
};