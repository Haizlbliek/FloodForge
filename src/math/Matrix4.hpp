#pragma once

#include "Vector.hpp"
#include <cstring>

struct Matrix4 {
	float m[16];

	inline Matrix4() noexcept {
		memset(m, 0, sizeof(m));
	}

	inline Matrix4(const float values[16]) noexcept {
		memcpy(m, values, sizeof(m));
	}

	inline Matrix4(
		float m00, float m10, float m20, float m30,
		float m01, float m11, float m21, float m31,
		float m02, float m12, float m22, float m32,
		float m03, float m13, float m23, float m33
	) noexcept {
		m[0] = m00; m[1] = m10; m[2] = m20; m[3] = m30;
		m[4] = m01; m[5] = m11; m[6] = m21; m[7] = m31;
		m[8] = m02; m[9] = m12; m[10] =m22; m[11] =m32;
		m[12] =m03; m[13] =m13; m[14] =m23; m[15] =m33;
	}

	inline float& v(int x, int y) {
		return m[y * 4 + x];
	}

	inline const float& v(int x, int y) const {
		return m[y * 4 + x];
	}

	static Matrix4 identity();
	static Matrix4 rotation(const Vector3f axis, float angle);
	//static Matrix4 rotationZ(float angle);
	static Matrix4 translation(float x, float y, float z);
	static Matrix4 scaling(float x, float y, float z);

	inline static Matrix4 translation(const Vector3f &vec) { return translation(vec.x, vec.y, vec.z); }
	inline static Matrix4 scaling(const Vector3f &vec) { return scaling(vec.x, vec.y, vec.z); }
};

Matrix4 operator*(const Matrix4 &a, const Matrix4 &b);

Matrix4 ortho(float left, float right, float bottom, float top, float near, float far);


Matrix4 projectionMatrix(Vector2 position, Vector2 scale);

Matrix4 modelMatrix(float x, float y);