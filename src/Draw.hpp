#include "math/Matrix4.hpp"
#include "math/Colour.hpp"
#include <glad/glad.h>
#include <cstring>

namespace Draw {
    enum PrimitiveType {
        DRAW_POINTS,
        DRAW_LINES,
        DRAW_LINE_STRIP,
        DRAW_LINE_LOOP,
        DRAW_TRIANGLES,
        DRAW_TRIANGLE_STRIP,
        DRAW_TRIANGLE_FAN,
        DRAW_QUADS,
    };

    struct Vec2f {
        float x; float y;

        Vec2f() : x(0.0f), y(0.0f) {}
        Vec2f(float x, float y) noexcept : x(x), y(y) {}
        Vec2f(const Vector2 &vec) noexcept {
            x = vec.x;
            y = vec.y;
        }

        operator Vector2() const {
            return { x, y };
        }
    };

    struct Color4f {
        float r, g, b, a;

        Color4f() : r(0.0f), g(0.0), b(0.0), a(0.0) {}
        Color4f(float r, float g, float b, float a) noexcept : r(r), g(g), b(b), a(a) {}
        Color4f(const Colour &col) noexcept {
            r = col.R();
            g = col.G();
            b = col.B();
            a = col.A();
        }

        operator Colour() const {
            return { r, g, b, a };
        }
    };

    struct Matrix4f {
        float m[16];

        Matrix4f() noexcept {
            memset(m, 0, sizeof(m));
        }

        Matrix4f(const Matrix4 &mat) noexcept {
            memcpy(m, mat.m, sizeof(float) * 16);
        }

        operator Matrix4() const {
            Matrix4 mat;
            memcpy(mat.m, m, sizeof(m));
            return mat;
        }

        inline float& v(size_t x, size_t y) {
            return m[y * 4 + x];
        }

        inline const float& v(size_t x, size_t y) const {
            return v(x, y);
        }

        static Matrix4f identity();
        static Matrix4f rotationZ(float angle);
        static Matrix4f translation(float x, float y, float z);
        static Matrix4f scaling(float x, float y, float z);

        Matrix4 operator*(const Matrix4f &m);
    };

    void init();
    void cleanup();

    void begin(PrimitiveType primType);
    void end();

    void useTexture(GLuint textureId);
    void viewport(int x, int y, int w, int h);

    void vertex(const Vec2f &pt);
    void texCoord(const Vec2f &texCoord);
    void color(const Color4f &color);

    inline void vertex(float x, float y) { vertex({ x, y }); };
    inline void texCoord(float u, float v) { texCoord({ u, v }); };
    inline void color(float r, float g, float b, float a) { color({ r, g, b, a }); };
    inline void color(float r, float g, float b) { color(r, g, b, 1.0f); };

    void loadIdentity();
    void loadMatrix(const Matrix4f &mat);
    void popMatrix();
    void pushMatrix();
    void rotate(float angle);
    void scale(const Vec2f &factor);
    void translate(const Vec2f &delta);
    inline void scale(float x, float y) { scale({x, y}); };
    inline void translate(float x, float y) { translate({x, y}); };

    void flush();
}

Draw::Matrix4f operator*(const Draw::Matrix4f &a, const Draw::Matrix4f &b);