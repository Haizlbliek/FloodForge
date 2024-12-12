#include "Vector.hpp"

// Vector2
std::ostream &operator<<(std::ostream &stream, Vector2 &obj) {
	return stream << "(" << obj.x << ", " << obj.y << ")";
}

bool operator==(const Vector2 &lhs, const Vector2 &rhs) {
	return (lhs.x == rhs.x) && (lhs.y == rhs.y);
}

Vector2 operator*(const Vector2 &lhs, const double &rhs) {
	return Vector2(lhs.x * rhs, lhs.y * rhs);
}

Vector2 operator*(const double &lhs, const Vector2 &rhs) {
	return Vector2(lhs * rhs.x, lhs * rhs.y);
}

std::string to_string(const Vector2 &vector) {
	return "(" + std::to_string(vector.x) + ", " + std::to_string(vector.y) + ")";
}

// Vector2i
std::ostream &operator<<(std::ostream &stream, Vector2i &obj) {
	return stream << "(" << obj.x << ", " << obj.y << ")";
}

bool operator==(const Vector2i &lhs, const Vector2i &rhs) {
	return (lhs.x == rhs.x) && (lhs.y == rhs.y);
}

void operator+=(Vector2i &lhs, const Vector2i &rhs) {
	lhs.x += rhs.x;
	lhs.y += rhs.y;
}

std::string to_string(const Vector2i &vector) {
	return "(" + std::to_string(vector.x) + ", " + std::to_string(vector.y) + ")";
}