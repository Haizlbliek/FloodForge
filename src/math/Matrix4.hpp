#ifndef MATRIX4_HPP
#define MATRIX4_HPP

struct Matrix4 {
    float m[16];
};

Matrix4 ortho(float left, float right, float bottom, float top, float near, float far);

Matrix4 modelMatrix(float x, float y);

#endif