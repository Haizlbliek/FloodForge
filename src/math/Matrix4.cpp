#include "Matrix4.hpp"

Matrix4 ortho(float left, float right, float bottom, float top, float near, float far) {
    Matrix4 matrix = {
        1, 0, 0, 0,
        0, 1, 0, 0,
        0, 0, 1, 0,
        0, 0, 0, 1,
    };
    matrix.m[0] = 2.0f / (right - left);
    matrix.m[5] = 2.0f / (top - bottom);
    matrix.m[10] = -2.0f / (far - near);
    matrix.m[12] = -(right + left) / (right - left);
    matrix.m[13] = -(top + bottom) / (top - bottom);
    matrix.m[14] = -(far + near) / (far - near);
    matrix.m[15] = 1.0f;
    return matrix;
}

Matrix4 projectionMatrix(Vector2 position, Vector2 scale) {
    float left = -scale.x + position.x;
    float right = scale.x + position.x;
    float bottom = -scale.y + position.y;
    float top = scale.y + position.y;
    
    return ortho(left, right, bottom, top, 0.0, 1000.0);
}

Matrix4 modelMatrix(float x, float y) {
    return {
        1, 0, 0, 0,
        0, 1, 0, 0,
        0, 0, 1, 0,
        x, y, 0, 1
    };
}