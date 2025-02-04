#pragma once

#include "../math/Vector.hpp"

#include "BodyChunk.hpp"
#include "BodyChunkConnection.hpp"

extern Vector2 cameraOffset;
extern double cameraScale;

extern std::vector<BodyChunk*> bodyChunks;
extern std::vector<BodyChunkConnection*> bodyChunkConnections;