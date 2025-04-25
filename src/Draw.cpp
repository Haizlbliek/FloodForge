#include "Draw.hpp"
#include <cstddef>
#include <cstdint>
#include <stack>
#include <string>
#include <iostream>
#include <cmath>
#include <glad/glad.h>

using Draw::vec2;
using Draw::vec3;
using Draw::mat4;
using Draw::col4;

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

	GLsizei vertexCount;
	GLsizei indexCount;
	GLsizei currentIndex;

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
	bool dirty;
	bool drawActive;

	struct {
		std::stack<mat4> stack;
		mat4 cur;
	} mats[MATRIX_COUNT];
	int matIdx;

	float u, v, r, g, b, a;

	VertexData verts[4];
	size_t active_idx0;
	size_t active_idx1;
} static drawState;

bool Draw::flushOnEnd = true;

void Draw::init() {
	drawState.vertexCount = 0;
	drawState.indexCount = 0;
	drawState.currentIndex = 0;
	drawState.dirty = false;
	drawState.drawActive = false;

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
		glGetShaderInfoLog(vtxShader, (GLsizei)log.size(), &strlen, &log.front());
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
		glGetShaderInfoLog(fragShader, (GLsizei)log.size(), &strlen, &log.front());
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
			glGetProgramInfoLog(fragShader, (GLsizei)log.size(), &strlen, &log.front());
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
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 1, 1, 0, GL_RGBA, GL_UNSIGNED_BYTE, whitePixel);

	drawState.activeTexture = drawState.placeholderTexture;

	for (int i = 0; i < MATRIX_COUNT; i++) {
		drawState.mats[i].cur = mat4::identity();
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
	drawState.dirty = false;
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

	if (Draw::flushOnEnd) {
		mat4 &modelView = drawState.mats[Draw::MODELVIEW].cur;
		mat4 &proj = drawState.mats[Draw::PROJECTION].cur;
		mat4 mvp = proj * modelView;
		glUniformMatrix4fv(drawState.mvpUniform, 1, GL_FALSE, mvp.m);
	} else {
		glUniformMatrix4fv(drawState.mvpUniform, 1, GL_FALSE, drawState.mats[Draw::PROJECTION].cur.m);
	}

	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, drawState.activeTexture);
	glUniform1i(drawState.texUniform, 0);

	// Draw buffers
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

	// flush batch on texture/Draw mode change
	if (drawState.curGlPrim != newDrawMode) {
		Draw::flush();
		drawState.curGlPrim = newDrawMode;
	}
}

template<bool cpuTransform>
static void pushVertex(VertexData vtxData);

template <>
void pushVertex<true>(VertexData vtxData) {
	const mat4 &mat = drawState.mats[Draw::MODELVIEW].cur;

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

template <>
inline void pushVertex<false>(VertexData vtxData) {
	drawState.batchVertices[drawState.vertexCount++] = vtxData;
}

static void pushIndex(uint32_t idx) {
	drawState.batchIndices[drawState.indexCount++] = idx;
}

void Draw::begin(Draw::PrimitiveType primType) {
	if (drawState.drawActive) {
		std::cerr << "ERROR: Draw::begin called when Draw operation was already active.\n";
		return;
	}

	if (drawState.dirty)
		Draw::flush();

	drawState.drawActive = true;
	drawState.active_idx0 = 0;
	drawState.curPrim = primType;
}

void Draw::texCoord(const vec2 &uv) {
	drawState.u = uv.x;
	drawState.v = uv.y;
}

void Draw::color(const col4 &col) {
	drawState.r = col.r;
	drawState.g = col.g;
	drawState.b = col.b;
	drawState.a = col.a;
}

template <bool flushOnEnd>
static void processVertex(const vec3 &pos) {
	if (!drawState.drawActive) {
		std::cerr << "ERROR: Draw::vertex called before Draw::begin.\n";
		return;
	}
	
	constexpr bool cpuTransform = !flushOnEnd;

	VertexData &curVert = drawState.verts[drawState.active_idx0++];
	curVert.x = pos.x;
	curVert.y = pos.y;
	curVert.z = pos.z;
	curVert.u = drawState.u;
	curVert.v = drawState.v;
	curVert.r = drawState.r;
	curVert.g = drawState.g;
	curVert.b = drawState.b;
	curVert.a = drawState.a;

	switch (drawState.curPrim) {
		case Draw::POINTS:
			beginDraw(1, 1, GL_POINTS);
			pushVertex<cpuTransform>(drawState.verts[0]);
			pushIndex(drawState.currentIndex++);
			drawState.active_idx0 = 0;
			break;
		
		case Draw::LINES:
			if (drawState.active_idx0 >= 2) {
				beginDraw(2, 2, GL_LINES);

				pushVertex<cpuTransform>(drawState.verts[0]);
				pushVertex<cpuTransform>(drawState.verts[1]);
				pushIndex(drawState.currentIndex++);
				pushIndex(drawState.currentIndex++);
				
				drawState.active_idx0 = 0;
			}
			break;

		case Draw::LINE_STRIP:
		case Draw::LINE_LOOP:
			// first vertex
			if (drawState.active_idx0 == 1) {
				beginDraw(1, 0, GL_LINES);

				drawState.active_idx1 = drawState.currentIndex;
				pushVertex<cpuTransform>(drawState.verts[0]);
			
			// following vertices
			} else {
				beginDraw(1, 2, GL_LINES);

				pushVertex<cpuTransform>(drawState.verts[1]);
				pushIndex(drawState.currentIndex++);
				pushIndex(drawState.currentIndex);
				drawState.active_idx0--;
			}

			break;
		
		case Draw::TRIANGLES:
			if (drawState.active_idx0 >= 3) {
				beginDraw(3, 3, GL_TRIANGLES);

				pushVertex<cpuTransform>(drawState.verts[0]);
				pushVertex<cpuTransform>(drawState.verts[1]);
				pushVertex<cpuTransform>(drawState.verts[2]);

				pushIndex(drawState.currentIndex++);
				pushIndex(drawState.currentIndex++);
				pushIndex(drawState.currentIndex++);

				drawState.active_idx0 = 0;
			}
			break;

		/*case TRIANGLE_STRIP:
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
			break;*/
		
		case Draw::QUADS:
			if (drawState.active_idx0 >= 4) {
				beginDraw(4, 6, GL_TRIANGLES);

				pushVertex<cpuTransform>(drawState.verts[0]);
				pushVertex<cpuTransform>(drawState.verts[1]);
				pushVertex<cpuTransform>(drawState.verts[2]);
				pushVertex<cpuTransform>(drawState.verts[3]);

				GLsizei &idx = drawState.currentIndex;
				pushIndex(idx + 0);
				pushIndex(idx + 1);
				pushIndex(idx + 2);
				pushIndex(idx + 2);
				pushIndex(idx + 3);
				pushIndex(idx + 0);
				idx += 4;
				
				drawState.active_idx0 = 0;
			}
			break;
	}
}

void Draw::vertex(const vec3 &pos) {
	if (Draw::flushOnEnd) {
		processVertex<true>(pos);
	} else {
		processVertex<false>(pos);
	}
}

void Draw::end() {
	if (!drawState.drawActive) {
		std::cerr << "ERROR: Draw::end called without an active Draw operation.\n";
		return;
	}

	switch (drawState.curPrim) {
		case LINE_LOOP:
			beginDraw(1, 1, GL_LINES);
			pushIndex(drawState.currentIndex++);
			pushIndex(drawState.active_idx1);
			break;
		
		default: break;
	}

	if (Draw::flushOnEnd || drawState.dirty) Draw::flush();
	drawState.drawActive = false;
}

///////////////////////
// MATRIX OPERATIONS //
///////////////////////

static void matrixChange() {
	if (drawState.matIdx == Draw::PROJECTION)
		drawState.dirty = true;
}

#define VALIDATE_MATRIX_OPERATION() \
	if (drawState.drawActive) { \
		std::cerr << "ERROR: Attempt to modify render matrix with a Draw operation already active.\n"; \
		return; \
	}

mat4 Draw::getMatrix(Draw::MatrixMode mode) {
	return drawState.mats[mode].cur;
}

void Draw::matrixMode(Draw::MatrixMode mode) {
	VALIDATE_MATRIX_OPERATION();
	drawState.matIdx = mode;

	matrixChange();
}

void Draw::loadIdentity() {
	VALIDATE_MATRIX_OPERATION();
	drawState.mats[drawState.matIdx].cur = mat4::identity();

	matrixChange();
}

void Draw::loadMatrix(const mat4 &mat) {
	VALIDATE_MATRIX_OPERATION();
	drawState.mats[drawState.matIdx].cur = mat;

	matrixChange();
}

void Draw::multMatrix(const mat4 &mat) {
	VALIDATE_MATRIX_OPERATION();
	drawState.mats[drawState.matIdx].cur = mat * drawState.mats[drawState.matIdx].cur;

	matrixChange();
}

void Draw::popMatrix() {
	VALIDATE_MATRIX_OPERATION();
	drawState.mats[drawState.matIdx].cur = drawState.mats[drawState.matIdx].stack.top();
	drawState.mats[drawState.matIdx].stack.pop();

	matrixChange();
}

void Draw::pushMatrix() {
	VALIDATE_MATRIX_OPERATION();
	drawState.mats[drawState.matIdx].stack.push(drawState.mats[drawState.matIdx].cur);

	matrixChange();
}

void Draw::rotate(float angle, const vec3 &axis) {
	VALIDATE_MATRIX_OPERATION();
	mat4 &m = drawState.mats[drawState.matIdx].cur;
	//m = mat4::rotationZ(angle) * m;
	m = mat4::rotation(axis, angle) * m;

	matrixChange();
}

void Draw::translate(const vec3 &vec) {
	VALIDATE_MATRIX_OPERATION();
	mat4 &m = drawState.mats[drawState.matIdx].cur;
	m = mat4::translation(vec.x, vec.y, vec.z) * m;

	matrixChange();
}

void Draw::scale(const vec3 &vec) {
	VALIDATE_MATRIX_OPERATION();
	mat4 &m = drawState.mats[drawState.matIdx].cur;
	m = mat4::scaling(vec.x, vec.y, vec.z) * m;

	matrixChange();
}

void Draw::ortho(float left, float right, float bottom, float top, float near, float far) {
	VALIDATE_MATRIX_OPERATION();
	mat4 mat;
	mat.v(0, 0) = 2.0f / (right - left);
	mat.v(1, 1) = 2.0f / (top - bottom);
	mat.v(2, 2) = -2.0f / (far - near);
	mat.v(3, 3) = 1.0f;

	mat.v(0, 3) = -(right + left) / (right - left);
	mat.v(1, 3) = -(top + bottom) / (top - bottom);
	mat.v(2, 3) = -(far + near) / (far - near);

	mat4 &m = drawState.mats[drawState.matIdx].cur;
	m = mat * m;

	matrixChange();
}

void Draw::perspective(float fov, float aspect, float near, float far) {
	VALIDATE_MATRIX_OPERATION();
	mat4 mat;
	float tan = tanf(fov / 2.0f);

	mat.v(0, 0) = 1.0f / (aspect * tan);
	mat.v(1, 1) = 1.0f / tan;
	mat.v(2, 2) = -(far + near) / (far - near);
	mat.v(3, 2) = -1.0f;
	mat.v(2, 3) = -(2.0f * far * near) / (far - near);

	mat4 &m = drawState.mats[drawState.matIdx].cur;
	m = mat * m;

	matrixChange();
}