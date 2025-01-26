#include <iostream>
#include <cmath>
#include <string>

#ifndef VECTOR_HPP
#define VECTOR_HPP

template <typename T>
struct BasicVector2 {
	T x;
	T y;

	BasicVector2() : x(0.0), y(0.0) {}
	BasicVector2(const T x, const T y) : x(x), y(y) {}
	BasicVector2(const BasicVector2<T> &vector) : x(vector.x), y(vector.y) {}
	BasicVector2(const BasicVector2<T> *vector) : x(vector->x), y(vector->y) {}

	void add(const T x, const T y) {
		this->x += x;
		this->y += y;
	}

	void add(const BasicVector2<T> &other) {
		this->x += other.x;
		this->y += other.y;
	}

	BasicVector2 &round() {
		x = ::round(x);
		y = ::round(y);
		return *this;
	}

	BasicVector2 rounded() const {
		return BasicVector2(
			::round(x),
			::round(y)
		);
	}

	double distanceTo(const BasicVector2<T> &other) {
		return std::sqrt(pow(x - other.x, 2) + pow(y - other.y, 2));
	}

	BasicVector2<T> operator+(const BasicVector2<T> &other) const {
		return BasicVector2(this->x + other.x, this->y + other.y);
	}

	BasicVector2<T> operator-(const BasicVector2<T> &other) const {
		return BasicVector2(this->x - other.x, this->y - other.y);
	}

	static BasicVector2 min(const BasicVector2<T> &a, const BasicVector2<T> &b) {
		return BasicVector2<T>(
			(a.x < b.x) ? a.x : b.x,
			(a.y < b.y) ? a.y : b.y
		);
	}

	static BasicVector2 max(const BasicVector2<T> &a, const BasicVector2<T> &b) {
		return BasicVector2<T>(
			(a.x > b.x) ? a.x : b.x,
			(a.y > b.y) ? a.y : b.y
		);
	}
};

using Vector2 = BasicVector2<double>;
using Vector2f = BasicVector2<float>;
using Vector2i = BasicVector2<int>;

/*
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

		Vector2 rounded() const {
			return Vector2(
				::round(x),
				::round(y)
			);
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
*/


struct Vector3f {
    float x, y, z;

    inline constexpr Vector3f() noexcept : x(0.0f), y(0.0f), z(0.0f) {}
    inline constexpr Vector3f(float x, float y, float z) noexcept : x(x), y(y), z(z) {}

    inline constexpr float lengthSq() const { return x * x + y * y + z * z; }
    inline float length() const { return sqrtf(lengthSq()); }

    inline Vector3f normalized() const {
        float len = length();
        return Vector3f(x / len, y / len, z / len);
    }

    inline constexpr float dot(const Vector3f &other) const {
        return x * other.x + y * other.y + z * other.z;
    }

    inline constexpr Vector3f cross(const Vector3f &other) const {
        return Vector3f(
            y * other.z - z * other.y,
            z * other.x - x * other.z,
            x * other.y - y * other.x
        );
    }
};


// Vector2
std::ostream &operator<<(std::ostream &stream, Vector2 &obj);

bool operator==(const Vector2 &lhs, const Vector2 &rhs);

Vector2 &operator*=(Vector2 &lhs, const double &rhs);

Vector2 operator*(const Vector2 &lhs, const double &rhs);

Vector2 operator*(const double &lhs, const Vector2 &rhs);

Vector2 operator*(const Vector2 &lhs, const Vector2 &rhs);

Vector2 operator/(const Vector2 &lhs, const double &rhs);

Vector2 operator/(const Vector2 &lhs, const Vector2 &rhs);

std::string to_string(const Vector2 &vector);

// Vector2i
std::ostream &operator<<(std::ostream &stream, Vector2i &obj);

bool operator==(const Vector2i &lhs, const Vector2i &rhs);

void operator+=(Vector2i &lhs, const Vector2i &rhs);

std::string to_string(const Vector2i &vector);

// Vector3f
std::ostream &operator<<(std::ostream &stream, Vector3f &vec);

inline constexpr bool operator==(const Vector3f &a, const Vector3f &b) {
    return a.x == b.x && a.y == b.y && a.z == b.z;
}

inline constexpr Vector3f operator+(const Vector3f &a, const Vector3f &b) {
    return Vector3f(a.x + b.x, a.y + b.y, a.z + b.z);
}

inline constexpr Vector3f operator-(const Vector3f &a, const Vector3f &b) {
    return Vector3f(a.x - b.x, a.y - b.y, a.z - b.z);
}

template <typename T>
inline constexpr Vector3f operator*(const Vector3f &v, const T s) {
    return Vector3f(v.x * s, v.y * s, v.z * s);
}

template <typename T>
inline constexpr Vector3f operator*(const T s, const Vector3f &v) {
    return Vector3f(v.x * s, v.y * s, v.z * s);
}

template <typename T>
inline constexpr Vector3f operator/(const Vector3f &v, const T s) {
    return Vector3f(v.x / s, v.y / s, v.z / s);
}

template <typename T>
inline constexpr Vector3f operator/(const T s, const Vector3f &v) {
    return Vector3f(v.x / s, v.y / s, v.z / s);
}

#endif