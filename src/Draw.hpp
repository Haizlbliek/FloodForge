#pragma once
#include "math/Matrix4.hpp"
#include "math/Colour.hpp"
#include <glad/glad.h>
#include <cstring>

namespace Draw {
    enum PrimitiveType {
        POINTS,
        LINES,
        LINE_STRIP,
        LINE_LOOP,
        TRIANGLES,
        //TRIANGLE_STRIP,
        //TRIANGLE_FAN,
        QUADS,
    };

    enum MatrixMode {
        PROJECTION = 0,
        MODELVIEW = 1,
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
            r = col.r;
            g = col.g;
            b = col.b;
            a = col.a;
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

        Matrix4f(const float *values) noexcept {
            memcpy(m, values, sizeof(m));
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
            return m[y * 4 + x];
        }

        static Matrix4f identity();
        static Matrix4f rotationZ(float angle);
        static Matrix4f translation(float x, float y, float z);
        static Matrix4f scaling(float x, float y, float z);
    };

    /**
     * @brief Initialize the drawing library.
     */
    void init();

    /**
     * @brief Destroy all resources used by the drawing library.
     */
    void cleanup();

    /**
     * @brief Begin drawing geometry.
     * 
     * @param primType The primitive type to use.
     */
    void begin(PrimitiveType primType);

    /**
     * @brief End drawing geometry.
     * 
     */
    void end();

    /**
     * @brief Use a texture for the next begin() call.
     * 
     * @param textureId The OpenGL texture ID to use. 0 to use no texture.
     */
    void useTexture(GLuint textureId);

    void vertex(const Vec2f &pt);
    void texCoord(const Vec2f &texCoord);
    void color(const Color4f &color);

    inline void vertex(float x, float y) { vertex({ x, y }); };
    inline void texCoord(float u, float v) { texCoord({ u, v }); };
    inline void color(float r, float g, float b, float a) { color({ r, g, b, a }); };
    inline void color(float r, float g, float b) { color(r, g, b, 1.0f); };

    void matrixMode(Draw::MatrixMode mode);
    void loadIdentity();
    void loadMatrix(const Matrix4f &mat);
    void multMatrix(const Matrix4f &mat);
    void popMatrix();
    void pushMatrix();
    void ortho(float left, float right, float bottom, float top, float near, float far);
    void rotate(float angle);
    void scale(const Vec2f &factor);
    void translate(const Vec2f &delta);
    inline void scale(float x, float y) { scale({x, y}); };
    inline void translate(float x, float y) { translate({x, y}); };

    /**
     * @brief Flush batched geometry to the GPU.
     * Draw commands geometry into a buffer stored in RAM. This command
     * sends the data to the GPU, draws it, and clears the buffer.
     *
     * This function should be called before calling any OpenGL draw commands or issuing any state changes
     * outside of this library.
     *
     * Alternatively, you may set flushOnEnd to true (which is the default value), eliminating the need to call flush(), although this will increase the number
     * of draw calls sent to the GPU and thus may incur a performance penalty.
     */
    void flush();

    /**
     * If set to true, flush() will automatically called on every subsequent call to end(). This may have a performance penalty.
     * This is set to true by default.
     *
     * You may temporarily set this to false for blocks of draw commands without any raw OpenGL calls in-between.
     */
    extern bool flushOnEnd;
}

Draw::Matrix4f operator*(const Draw::Matrix4f &a, const Draw::Matrix4f &b);