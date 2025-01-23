#include <iostream>
#include <cmath>
#include <string>

#ifndef VECTOR_HPP
#define VECTOR_HPP

class Vector2 {
	public:
		double x;
		double y;

		Vector2() : x(0.0), y(0.0) {}
		Vector2(double x, double y) : x(x), y(y) {}
		Vector2(const Vector2 &vector) : x(vector.x), y(vector.y) {}
		Vector2(const Vector2 *vector) : x(vector->x), y(vector->y) {}

		void add(const double x, const double y) {
			this->x += x;
			this->y += y;
		}

		void add(const Vector2 &other) {
			this->x += other.x;
			this->y += other.y;
		}

		Vector2 &round() {
			x = ::round(x);
			y = ::round(y);
			return *this;
		}

		double distanceTo(const Vector2 &other) {
			return std::sqrt(pow(x - other.x, 2) + pow(y - other.y, 2));
		}

		Vector2 operator+(const Vector2 &other) const {
			return Vector2(this->x + other.x, this->y + other.y);
		}

		Vector2 operator-(const Vector2 &other) const {
			return Vector2(this->x - other.x, this->y - other.y);
		}

		void X(const double x) {
			this->x = x;
		}

		void Y(const double y) {
			this->y = y;
		}

		static Vector2 min(const Vector2 &a, const Vector2 &b) {
			return Vector2(
				(a.x < b.x) ? a.x : b.x,
				(a.y < b.y) ? a.y : b.y
			);
		}

		static Vector2 max(const Vector2 &a, const Vector2 &b) {
			return Vector2(
				(a.x > b.x) ? a.x : b.x,
				(a.y > b.y) ? a.y : b.y
			);
		}

};

class Vector2i {
	public:
		int x;
		int y;

		Vector2i() {}

		Vector2i(int x, int y) : x(x), y(y) {}

		Vector2i(const Vector2i &vector) : x(vector.x), y(vector.y) {}
};


// Vector2
std::ostream &operator<<(std::ostream &stream, Vector2 &obj);

bool operator==(const Vector2 &lhs, const Vector2 &rhs);

Vector2 operator*(const Vector2 &lhs, const double &rhs);

Vector2 operator*(const double &lhs, const Vector2 &rhs);

std::string to_string(const Vector2 &vector);

// Vector2i
std::ostream &operator<<(std::ostream &stream, Vector2i &obj);

bool operator==(const Vector2i &lhs, const Vector2i &rhs);

void operator+=(Vector2i &lhs, const Vector2i &rhs);

std::string to_string(const Vector2i &vector);

#endif