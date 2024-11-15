#ifndef RECT_HPP
#define RECT_HPP

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

		static Rect fromSize(double x, double y, double width, double height) {
			return Rect(x, y, x + width, y + height);
		}

		void X0(const double x0) { this->x0 = x0; }
		void Y0(const double y0) { this->y0 = y0; }
		void X1(const double x1) { this->x1 = x1; }
		void Y1(const double y1) { this->y1 = y1; }

		const double X0() const { return x0; }
		const double Y0() const { return y0; }
		const double X1() const { return x1; }
		const double Y1() const { return y1; }

	private:
		double x0;
		double y0;
		double x1;
		double y1;
};

#endif