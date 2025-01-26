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

Matrix4 operator*(const Matrix4 &a, const Matrix4 &b) {
    Matrix4 dest;
    
    // damn i forgot how matrix multiplication works
    // this is just from stack overflow
    // looks like they unrolled what would be two nested for loops.
    dest.v(0,0) = a.v(0,0) * b.v(0,0) + a.v(0,1) * b.v(1,0) + a.v(0,2) * b.v(2,0) + a.v(0,3) * b.v(3,0); 
    dest.v(0,1) = a.v(0,0) * b.v(0,1) + a.v(0,1) * b.v(1,1) + a.v(0,2) * b.v(2,1) + a.v(0,3) * b.v(3,1); 
    dest.v(0,2) = a.v(0,0) * b.v(0,2) + a.v(0,1) * b.v(1,2) + a.v(0,2) * b.v(2,2) + a.v(0,3) * b.v(3,2); 
    dest.v(0,3) = a.v(0,0) * b.v(0,3) + a.v(0,1) * b.v(1,3) + a.v(0,2) * b.v(2,3) + a.v(0,3) * b.v(3,3); 
    dest.v(1,0) = a.v(1,0) * b.v(0,0) + a.v(1,1) * b.v(1,0) + a.v(1,2) * b.v(2,0) + a.v(1,3) * b.v(3,0); 
    dest.v(1,1) = a.v(1,0) * b.v(0,1) + a.v(1,1) * b.v(1,1) + a.v(1,2) * b.v(2,1) + a.v(1,3) * b.v(3,1); 
    dest.v(1,2) = a.v(1,0) * b.v(0,2) + a.v(1,1) * b.v(1,2) + a.v(1,2) * b.v(2,2) + a.v(1,3) * b.v(3,2); 
    dest.v(1,3) = a.v(1,0) * b.v(0,3) + a.v(1,1) * b.v(1,3) + a.v(1,2) * b.v(2,3) + a.v(1,3) * b.v(3,3); 
    dest.v(2,0) = a.v(2,0) * b.v(0,0) + a.v(2,1) * b.v(1,0) + a.v(2,2) * b.v(2,0) + a.v(2,3) * b.v(3,0); 
    dest.v(2,1) = a.v(2,0) * b.v(0,1) + a.v(2,1) * b.v(1,1) + a.v(2,2) * b.v(2,1) + a.v(2,3) * b.v(3,1); 
    dest.v(2,2) = a.v(2,0) * b.v(0,2) + a.v(2,1) * b.v(1,2) + a.v(2,2) * b.v(2,2) + a.v(2,3) * b.v(3,2); 
    dest.v(2,3) = a.v(2,0) * b.v(0,3) + a.v(2,1) * b.v(1,3) + a.v(2,2) * b.v(2,3) + a.v(2,3) * b.v(3,3); 
    dest.v(3,0) = a.v(3,0) * b.v(0,0) + a.v(3,1) * b.v(1,0) + a.v(3,2) * b.v(2,0) + a.v(3,3) * b.v(3,0); 
    dest.v(3,1) = a.v(3,0) * b.v(0,1) + a.v(3,1) * b.v(1,1) + a.v(3,2) * b.v(2,1) + a.v(3,3) * b.v(3,1); 
    dest.v(3,2) = a.v(3,0) * b.v(0,2) + a.v(3,1) * b.v(1,2) + a.v(3,2) * b.v(2,2) + a.v(3,3) * b.v(3,2); 
    dest.v(3,3) = a.v(3,0) * b.v(0,3) + a.v(3,1) * b.v(1,3) + a.v(3,2) * b.v(2,3) + a.v(3,3) * b.v(3,3);

    return dest;
}

Matrix4 Matrix4::identity() {
    Matrix4 mat;
    mat.v(0, 0) = 1.0f;
    mat.v(1, 1) = 1.0f;
    mat.v(2,2) = 1.0f;
    mat.v(3, 3) = 1.0f;
    return mat;
}

Matrix4 Matrix4::rotation(Vector3f axis, float angle) {
    axis = axis.normalized();

    Matrix4 mat;
    float c = cosf(angle);
    float s = sinf(angle);
    float t = 1.0f - c;

    float tx = t * axis.x;
    float ty = t * axis.y;
    float tz = t * axis.z;

    mat.v(0, 0) = tx * axis.x + c;
    mat.v(0, 1) = tx * axis.y + axis.z * s;
    mat.v(0, 2) = tx * axis.z - axis.y * s;

    mat.v(1, 0) = tx * axis.y - axis.z * s;
    mat.v(1, 1) = ty * axis.y + c;
    mat.v(1, 2) = ty * axis.z + axis.x * s;

    mat.v(2, 0) = tx * axis.z + axis.y * s;
    mat.v(2, 1) = ty * axis.z - axis.x * s;
    mat.v(2, 2) = tz * axis.z + c;

    mat.v(3, 3) = 1.0f;

    return mat;
}

/*Matrix4 Matrix4::rotationZ(float angle) {
    Matrix4 rotMat;
    float cos = cosf(angle);
    float sin = sinf(angle);

    rotMat.v(0, 0) = cos;
    rotMat.v(1, 0) = -sin;
    rotMat.v(0, 1) = sin;
    rotMat.v(1, 1) = cos;
    rotMat.v(2, 2) = 1.0f;
    rotMat.v(3, 3) = 1.0f;

    return rotMat;
}*/

Matrix4 Matrix4::scaling(float x, float y, float z) {
    Matrix4 mat;
    mat.v(0, 0) = x;
    mat.v(1, 1) = y;
    mat.v(2, 2) = z;
    mat.v(3, 3) = 1.0f;

    return mat;
}

Matrix4 Matrix4::translation(float x, float y, float z) {
    Matrix4 mat;
    mat.v(0, 0) = 1.0f;
    mat.v(1, 1) = 1.0f;
    mat.v(2, 2) = 1.0f;
    mat.v(3, 3) = 1.0f;
    mat.v(0, 3) = x;
    mat.v(1, 3) = y;
    mat.v(2, 3) = z;

    return mat;
}