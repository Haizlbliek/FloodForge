#pragma once

#include "Vector.hpp"
#include "../ui/UIMouse.hpp"

class UVRect {
	public:
		double x0;
		double y0;
		double x1;
		double y1;
		Vector2 uv0;
		Vector2 uv1;
		Vector2 uv2;
		Vector2 uv3;

		UVRect() {
			this->x0 = 0.0;
			this->y0 = 0.0;
			this->x1 = 0.0;
			this->y1 = 0.0;
			this->uv0 = { 0.0, 1.0 };
			this->uv1 = { 1.0, 1.0 };
			this->uv2 = { 1.0, 0.0 };
			this->uv3 = { 0.0, 0.0 };
		}

		UVRect(double x0, double y0, double x1, double y1) {
			this->x0 = std::min(x0, x1);
			this->y0 = std::min(y0, y1);
			this->x1 = std::max(x0, x1);
			this->y1 = std::max(y0, y1);
			this->uv0 = { 0.0, 1.0 };
			this->uv1 = { 1.0, 1.0 };
			this->uv2 = { 1.0, 0.0 };
			this->uv3 = { 0.0, 0.0 };
		}

		UVRect(const Vector2 point0, const Vector2 point1) {
			this->x0 = std::min(point0.x, point1.x);
			this->y0 = std::min(point0.y, point1.y);
			this->x1 = std::max(point0.x, point1.x);
			this->y1 = std::max(point0.y, point1.y);
			this->uv0 = { 0.0, 1.0 };
			this->uv1 = { 1.0, 1.0 };
			this->uv2 = { 1.0, 0.0 };
			this->uv3 = { 0.0, 0.0 };
		}

		bool inside(const UIMouse &point) const {
			return point.x >= x0 && point.y >= y0 && point.x <= x1 && point.y <= y1;
		}

		bool inside(Vector2 point) const {
			return point.x >= x0 && point.y >= y0 && point.x <= x1 && point.y <= y1;
		}

		bool inside(double x, double y) const {
			return x >= x0 && y >= y0 && x <= x1 && y <= y1;
		}

		UVRect &offset(Vector2 offset) {
			x0 += offset.x;
			x1 += offset.x;
			y0 += offset.y;
			y1 += offset.y;
			
			return *this;
		}

		static UVRect fromSize(double x, double y, double width, double height) {
			return UVRect(x, y, x + width, y + height);
		}

		double CenterX() const {
			return (x0 + x1) * 0.5;
		}

		double CenterY() const {
			return (y0 + y1) * 0.5;
		}

		UVRect &uv(double u0, double v0, double u1, double v1) {
			this->uv0.x = u0;
			this->uv0.y = v0;
			this->uv1.x = u1;
			this->uv1.y = v0;
			this->uv2.x = u1;
			this->uv2.y = v1;
			this->uv3.x = u0;
			this->uv3.y = v1;

			return *this;
		}
};