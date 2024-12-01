#include "Draw.hpp"
#include <cstddef>
#include <cstdint>
#include <stack>
#include <string>
#include <iostream>
#include <cmath>
#include <glad/glad.h>

#define MAX_VERTICES 1024
#define MAX_INDICES 1024
#define MATRIX_COUNT 2

struct VertexData {
    float x, y, z, u, v, r, g, b, a;
};

const char *SHADER_VTX_SRC = "#version 330 core\n"
"layout(location=0) in vec3 aPos;\n"
"layout(location=1) in vec2 aTexCoord;\n"
"layout(location=2) in vec4 aColor;\n"
"out vec4 color;\n"
"out vec2 texCoord;\n"
"uniform mat4 uMvp;\n"
"void main() {\n"
"gl_Position = uMvp * vec4(aPos, 1.0);\n"
"color = aColor;\n"
"texCoord = aTexCoord;\n"
"}\n";

const char *SHADER_FRAG_SRC = "#version 330 core\n"
"in vec4 color;\n"
"in vec2 texCoord;\n"
"out vec4 fragColor;\n"
"uniform sampler2D uTexture;\n"
"void main() {\n"
"fragColor = texture(uTexture, texCoord) * color;\n"
"}\n";

struct {    
    VertexData batchVertices[MAX_VERTICES];
    uint32_t batchIndices[MAX_INDICES];

    size_t vertexCount;
    size_t indexCount;
    size_t currentIndex;

    GLuint vertexArray;
    GLuint vtxBuffer;
    GLuint idxBuffer;
    GLuint gpuProgram;
    GLuint mvpUniform;
    GLuint texUniform;

    GLuint placeholderTexture;
    GLuint activeTexture;

    Draw::PrimitiveType curPrim;
    GLuint curGlPrim;

    struct {
        std::stack<Draw::Matrix4f> stack;
        Draw::Matrix4f cur;
    } mats[MATRIX_COUNT];
    int matIdx;

    float u, v, r, g, b, a;

    VertexData verts[4];
    size_t idx;
} static drawState;

bool Draw::flushOnEnd = true;

void Draw::init() {
    drawState.vertexCount = 0;
    drawState.indexCount = 0;
    drawState.currentIndex = 0;

    glGenVertexArrays(1, &drawState.vertexArray);
    glBindVertexArray(drawState.vertexArray);
    glGenBuffers(1, &drawState.vtxBuffer);
    glGenBuffers(1, &drawState.idxBuffer);

    glBindBuffer(GL_ARRAY_BUFFER, drawState.vtxBuffer);
    glBufferData(GL_ARRAY_BUFFER, MAX_VERTICES * sizeof(VertexData), nullptr, GL_STREAM_DRAW);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, drawState.idxBuffer);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, MAX_INDICES * sizeof(uint32_t), nullptr, GL_STREAM_DRAW);

    // position attribute
    glVertexAttribPointer(0, 3, GL_FLOAT, false, sizeof(VertexData), 0);
    glEnableVertexAttribArray(0);

    // tex coord attribute
    glVertexAttribPointer(1, 2, GL_FLOAT, false, sizeof(VertexData), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);

    // color attribute
    glVertexAttribPointer(2, 4, GL_FLOAT, false, sizeof(VertexData), (void*)(5 * sizeof(float)));
    glEnableVertexAttribArray(2);

    glBindVertexArray(0);

    GLint status, strlen;
    std::string log;
    bool success = true;

    // create vertex shader
    GLuint vtxShader = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vtxShader, 1, &SHADER_VTX_SRC, 0);
    glCompileShader(vtxShader);

    glGetShaderiv(vtxShader, GL_COMPILE_STATUS, &status);
    if (!status) {
        success = false;
        glGetShaderiv(vtxShader, GL_INFO_LOG_LENGTH, &strlen);
        log.resize(strlen);
        glGetShaderInfoLog(vtxShader, log.size(), &strlen, log.data());
        std::cerr << "Shader compilation failed: " << log << "\n";
    }

    // create fragment shader
    GLuint fragShader = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fragShader, 1, &SHADER_FRAG_SRC, 0);
    glCompileShader(fragShader);

    glGetShaderiv(fragShader, GL_COMPILE_STATUS, &status);
    if (!status) {
        success = false;
        glGetShaderiv(fragShader, GL_INFO_LOG_LENGTH, &strlen);
        log.resize(strlen);
        glGetShaderInfoLog(fragShader, log.size(), &strlen, log.data());
        std::cerr << "Shader compilation failed: " << log << "\n";
    }

    if (success)
    {
        GLuint &program = drawState.gpuProgram;
        program = glCreateProgram();
        glAttachShader(program, vtxShader);
        glAttachShader(program, fragShader);
        glLinkProgram(program);

        glGetShaderiv(program, GL_LINK_STATUS, &status);
        if (!status) {
            glGetProgramiv(fragShader, GL_INFO_LOG_LENGTH, &strlen);
            log.resize(strlen);
            glGetProgramInfoLog(fragShader, log.size(), &strlen, log.data());
            std::cerr << "Shader linking failed: " << log << "\n";
        }

        drawState.mvpUniform = glGetUniformLocation(program, "uMvp");
        drawState.texUniform = glGetUniformLocation(program, "uTexture");
    }

    glDeleteShader(vtxShader);
    glDeleteShader(fragShader);

    // create placeholder texture
    uint8_t whitePixel[4] = { 255, 255, 255, 255 };
    glGenTextures(1, &drawState.placeholderTexture);
    glBindTexture(GL_TEXTURE_2D, drawState.placeholderTexture);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 1, 1, 0, GL_RGBA, GL_UNSIGNED_BYTE, whitePixel);

    drawState.activeTexture = drawState.placeholderTexture;

    for (int i = 0; i < MATRIX_COUNT; i++) {
        drawState.mats[i].cur = Draw::Matrix4f::identity();
    }

    drawState.matIdx = PROJECTION;
}

void Draw::cleanup() {
    glDeleteVertexArrays(1, &drawState.vertexArray);
    glDeleteBuffers(1, &drawState.vtxBuffer);
    glDeleteBuffers(1, &drawState.idxBuffer);
    glDeleteProgram(drawState.gpuProgram);
    glDeleteTextures(1, &drawState.placeholderTexture);

    drawState.vertexArray = 0;
    drawState.vtxBuffer = 0;
    drawState.idxBuffer = 0;
    drawState.gpuProgram = 0;
    drawState.placeholderTexture = 0;
}

void Draw::flush() {
    if (drawState.vertexCount == 0) return;

    glBindVertexArray(drawState.vertexArray);

    // update vtx buffer
    glBindBuffer(GL_ARRAY_BUFFER, drawState.vtxBuffer);
    glBufferSubData(GL_ARRAY_BUFFER, 0, drawState.vertexCount * sizeof(VertexData), drawState.batchVertices);

    // update index buffer
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, drawState.idxBuffer);
    glBufferSubData(GL_ELEMENT_ARRAY_BUFFER, 0, drawState.indexCount * sizeof(uint32_t), drawState.batchIndices);

    // update shader and bind texture
    glUseProgram(drawState.gpuProgram);
    glUniformMatrix4fv(drawState.mvpUniform, 1, GL_FALSE, Matrix4f::identity().m);

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, drawState.activeTexture);
    glUniform1i(drawState.texUniform, 0);

    // draw buffers
    glDrawElements(drawState.curGlPrim, drawState.indexCount, GL_UNSIGNED_INT, nullptr);
    drawState.vertexCount = 0;
    drawState.indexCount = 0;
    drawState.currentIndex = 0;
}

void Draw::useTexture(GLuint textureId) {
    if (textureId == 0) {
        drawState.activeTexture = drawState.placeholderTexture;
    } else {
        drawState.activeTexture = textureId;
    }
}

static void checkCapacity(size_t newVertices, size_t numIndices) {
    if (drawState.vertexCount + newVertices >= MAX_VERTICES || drawState.indexCount + numIndices >= MAX_INDICES) {
        Draw::flush();
    }
}

static void beginDraw(size_t requiredCapacity, size_t numIndices, GLuint newDrawMode) {
    checkCapacity(requiredCapacity, numIndices);

    // flush batch on texture/draw mode change
    if (drawState.curGlPrim != newDrawMode) {
        Draw::flush();
        drawState.curGlPrim = newDrawMode;
    }
}

static void pushVertex(VertexData vtxData) {
    const Draw::Matrix4f &mat = drawState.mats[0].cur * drawState.mats[1].cur;

    float x = vtxData.x;
    float y = vtxData.y;
    const float z = 0.0f;
    const float w = 1.0f;

    float x0 = x * mat.m[0] + y * mat.m[4] + z * mat.m[8] + w * mat.m[12];
    float y0 = x * mat.m[1] + y * mat.m[5] + z * mat.m[9] + w * mat.m[13];
    float z0 = x * mat.m[2] + y * mat.m[6] + z * mat.m[10] + w * mat.m[14];
    float w0 = x * mat.m[3] + y * mat.m[7] + z * mat.m[11] + w * mat.m[15];

    vtxData.x = x0 / w0;
    vtxData.y = y0 / w0;
    vtxData.z = z0;

    drawState.batchVertices[drawState.vertexCount++] = vtxData;
}

static void pushIndex(uint32_t idx) {
    drawState.batchIndices[drawState.indexCount++] = idx;
}

void Draw::begin(Draw::PrimitiveType primType) {
    drawState.idx = 0;
    drawState.curPrim = primType;
}

void Draw::texCoord(const Vec2f &uv) {
    drawState.u = uv.x;
    drawState.v = uv.y;
}

void Draw::color(const Color4f &col) {
    drawState.r = col.r;
    drawState.g = col.g;
    drawState.b = col.b;
    drawState.a = col.a;
}

void Draw::vertex(const Vec2f &pos) {
    VertexData &curVert = drawState.verts[drawState.idx++];
    curVert.x = pos.x;
    curVert.y = pos.y;
    curVert.z = 0.0f;
    curVert.u = drawState.u;
    curVert.v = drawState.v;
    curVert.r = drawState.r;
    curVert.g = drawState.g;
    curVert.b = drawState.b;
    curVert.a = drawState.a;

    switch (drawState.curPrim) {
        case POINTS:
            beginDraw(1, 1, GL_POINTS);
            pushVertex(drawState.verts[0]);
            drawState.idx = 0;
            break;
        
        case LINES:
        if (drawState.idx >= 2) {
            beginDraw(2, 2, GL_LINES);

            pushVertex(drawState.verts[0]);
            pushVertex(drawState.verts[1]);
            pushIndex(drawState.currentIndex++);
            pushIndex(drawState.currentIndex++);
            
            drawState.idx = 0;
            break;
        }

        case LINE_STRIP:
            beginDraw(1, 1, GL_LINE_STRIP);
            pushVertex(drawState.verts[0]);
            pushIndex(drawState.currentIndex++);
            drawState.idx = 0;
            break;

        case LINE_LOOP:
            beginDraw(1, 1, GL_LINE_LOOP);
            pushVertex(drawState.verts[0]);
            pushIndex(drawState.currentIndex++);
            drawState.idx = 0;
            break;
        
        case TRIANGLES:
        if (drawState.idx >= 3) {
            beginDraw(3, 3, GL_TRIANGLES);

            pushVertex(drawState.verts[0]);
            pushVertex(drawState.verts[1]);
            pushVertex(drawState.verts[2]);

            pushIndex(drawState.currentIndex++);
            pushIndex(drawState.currentIndex++);
            pushIndex(drawState.currentIndex++);

            drawState.idx = 0;
            break;
        }

        case TRIANGLE_STRIP:
            beginDraw(1, 1, GL_TRIANGLE_STRIP);
            pushVertex(drawState.verts[0]);
            pushIndex(drawState.currentIndex++);
            drawState.idx = 0;
            break;
        
        case TRIANGLE_FAN:
            beginDraw(1, 1, GL_TRIANGLE_FAN);
            pushVertex(drawState.verts[0]);
            pushIndex(drawState.currentIndex++);
            drawState.idx = 0;
            break;
        
        case QUADS:
        if (drawState.idx >= 4) {
            beginDraw(4, 6, GL_TRIANGLES);

            pushVertex(drawState.verts[0]);
            pushVertex(drawState.verts[1]);
            pushVertex(drawState.verts[2]);
            pushVertex(drawState.verts[3]);

            size_t &idx = drawState.currentIndex;
            pushIndex(idx + 0);
            pushIndex(idx + 1);
            pushIndex(idx + 2);
            pushIndex(idx + 2);
            pushIndex(idx + 3);
            pushIndex(idx + 0);
            idx += 4;
            
            drawState.idx = 0;
            break;
        }
    }
}

void Draw::end() {
    if (Draw::flushOnEnd) Draw::flush();
}

///////////////////////
// MATRIX OPERATIONS //
///////////////////////

#define O(y,x) (y + (x<<2))
Draw::Matrix4f operator*(const Draw::Matrix4f &a, const Draw::Matrix4f &b) {
    Draw::Matrix4f dest;
    
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

Draw::Matrix4f Draw::Matrix4f::identity() {
    Matrix4f mat;
    mat.v(0, 0) = 1.0f;
    mat.v(1, 1) = 1.0f;
    mat.v(2,2) = 1.0f;
    mat.v(3, 3) = 1.0f;
    return mat;
}

Draw::Matrix4f Draw::Matrix4f::rotationZ(float angle) {
    Draw::Matrix4f rotMat;
    float cos = cosf(angle);
    float sin = sinf(angle);

    rotMat.v(0, 0) = cos;
    rotMat.v(1, 0) = -sin;
    rotMat.v(0, 1) = sin;
    rotMat.v(1, 1) = cos;
    rotMat.v(2, 2) = 1.0f;
    rotMat.v(3, 3) = 1.0f;

    return rotMat;
}

Draw::Matrix4f Draw::Matrix4f::scaling(float x, float y, float z) {
    Draw::Matrix4f mat;
    mat.v(0, 0) = x;
    mat.v(1, 1) = y;
    mat.v(2, 2) = z;
    mat.v(3, 3) = 1.0f;

    return mat;
}

Draw::Matrix4f Draw::Matrix4f::translation(float x, float y, float z) {
    Draw::Matrix4f mat;
    mat.v(0, 0) = 1.0f;
    mat.v(1, 1) = 1.0f;
    mat.v(2, 2) = 1.0f;
    mat.v(3, 3) = 1.0f;
    mat.v(0, 3) = x;
    mat.v(1, 3) = y;
    mat.v(2, 3) = z;

    return mat;
}

void Draw::matrixMode(Draw::MatrixMode mode) {
    drawState.matIdx = mode;
}

void Draw::loadIdentity() {
    drawState.mats[drawState.matIdx].cur = Draw::Matrix4f::identity();
}

void Draw::loadMatrix(const Matrix4f &mat) {
    drawState.mats[drawState.matIdx].cur = mat;
}

void Draw::popMatrix() {
    drawState.mats[drawState.matIdx].cur = drawState.mats[drawState.matIdx].stack.top();
    drawState.mats[drawState.matIdx].stack.pop();
}

void Draw::pushMatrix() {
    drawState.mats[drawState.matIdx].stack.push(drawState.mats[drawState.matIdx].cur);
}

void Draw::rotate(float angle) {
    Draw::Matrix4f &m = drawState.mats[drawState.matIdx].cur;
    m = Draw::Matrix4f::rotationZ(angle) * m;
}

void Draw::translate(const Vec2f &vec) {
    Draw::Matrix4f &m = drawState.mats[drawState.matIdx].cur;
    m = Draw::Matrix4f::translation(vec.x, vec.y, 0.0f) * m;
}

void Draw::scale(const Vec2f &vec) {
    Draw::Matrix4f &m = drawState.mats[drawState.matIdx].cur;
    m = Draw::Matrix4f::scaling(vec.x, vec.y, 1.0f) * m;
}

void Draw::ortho(float left, float right, float bottom, float top, float near, float far) {
    Draw::Matrix4f mat;
    mat.v(0, 0) = 2.0f / (right - left);
    mat.v(1, 1) = 2.0f / (top - bottom);
    mat.v(2, 2) = -2.0f / (far - near);
    mat.v(3, 3) = 1.0f;

    mat.v(0, 3) = -(right + left) / (right - left);
    mat.v(1, 3) = -(top + bottom) / (top - bottom);
    mat.v(2, 3) = -(far + near) / (far - near);

    Draw::Matrix4f &m = drawState.mats[drawState.matIdx].cur;
    m = mat * m;
}