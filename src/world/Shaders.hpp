#pragma once

#include "../gl.h"

namespace Shaders {
	extern GLuint roomShader;
	extern GLuint hueSliderShader;
	extern GLuint colorSquareShader;

	void init();
	void cleanup();
}