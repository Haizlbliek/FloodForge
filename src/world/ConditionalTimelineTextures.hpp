#pragma once

#include <unordered_map>
#include <vector>
#include <string>
#include <filesystem>
#include "../gl.h"

#include "../Utils.hpp"

namespace ConditionalTimelineTextures {
	extern std::unordered_map<std::string, GLuint> textures;
	extern std::vector<std::string> timelines;
	extern GLuint UNKNOWN;

	GLuint getTexture(std::string type);

	void init();

	bool hasTimeline(std::string timeline);
};