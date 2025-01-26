#pragma once
#include <glad/glad.h>
#include "math/Matrix4.hpp"
#include "math/Colour.hpp"
#include "math/Vector.hpp"

namespace Draw {
    using vec2 = Vector2f;
    using vec3 = Vector3f;
    using mat4 = Matrix4;
    using col4 = Colour;

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

    void vertex(const Vector3f &pt);
    void vertex(const vec3 &pt);
    void texCoord(const vec2 &texCoord);
    void color(const col4 &color);

    inline void vertex(float x, float y, float z) { vertex({ x, y, z }); };
    inline void vertex(float x, float y) { vertex({ x, y, 0.0f }); };
    inline void texCoord(float u, float v) { texCoord({ u, v }); };
    inline void color(float r, float g, float b, float a) { color({ r, g, b, a }); };
    inline void color(float r, float g, float b) { color(r, g, b, 1.0f); };

    mat4 getMatrix(Draw::MatrixMode mode);
    void matrixMode(Draw::MatrixMode mode);
    void loadIdentity();
    void loadMatrix(const mat4 &mat);
    void multMatrix(const mat4 &mat);
    void popMatrix();
    void pushMatrix();
    void ortho(float left, float right, float bottom, float top, float near, float far);
    void perspective(float fov, float aspect, float near, float far);

    void translate(const vec3 &delta);
    void scale(const vec3 &factor);
    void rotate(float angle, const vec3 &axis);

    inline void translate(const vec2 &delta) { translate({ delta.x, delta.y, 0.0f }); };
    inline void scale(const vec2 &factor) { scale({ factor.x, factor.y, 1.0f }); };
    inline void translate(float x, float y) { translate({x, y}); };
    inline void scale(float x, float y) { scale({x, y}); };

    inline void translate(float x, float y, float z) { translate({x, y, z}); };
    inline void scale(float x, float y, float z) { scale({x, y, z}); };
    inline void rotate(float angle, float x, float y, float z) { rotate(angle, {x, y, z}); }

    /**
     * @brief Flush batched geometry to the GPU.
     * Draw commands geometry into a buffer stored in RAM. This command
     * sends the data to the GPU, draws it, and clears the buffer.
     *
     * This function should be called before calling any OpenGL Draw commands or issuing any state changes
     * outside of this library.
     *
     * Alternatively, you may set flushOnEnd to true (which is the default value), eliminating the need to call flush(), although this will increase the number
     * of Draw calls sent to the GPU and thus may incur a performance penalty.
     */
    void flush();

    /**
     * If set to true, flush() will automatically called on every subsequent call to end().
     * This is set to true by default. This can also be dynamically changed in-between begin/end calls.
     *
     * False:
     *   - Decreased Draw call count
     *   - Model-view transformation is done on the CPU.
     *   - No modifying OpenGL state externally while it is false.
     *
     * True:
     *   - Draw call for each begin/end,
     *   - Matrix transformation done on the GPU.
     *   - Can modify OpenGL state freely outside of begin/end calls.
     */
    extern bool flushOnEnd;
}