#pragma once

#include "BodyChunk.hpp"

class BodyChunkConnection {
    public:
        enum Type {
            Normal,
            Pull,
            Push
        };

        BodyChunkConnection(BodyChunk *chunk1, BodyChunk *chunk2, double distance, BodyChunkConnection::Type type, double elasticity, double weightSymmetry) {
            this->chunk1 = chunk1;
            this->chunk2 = chunk2;
            this->distance = distance;
            this->type = type;
            this->elasticity = elasticity;
            this->weightSymmetry = weightSymmetry;
        }

        void update() {
			if (!active) return;

			float num = chunk1->position.distanceTo(chunk2->position);
			if (type == BodyChunkConnection::Type::Normal || (type == BodyChunkConnection::Type::Pull && num > distance) || (type == BodyChunkConnection::Type::Push && num < distance)) {
				Vector2 a = (chunk2->position - chunk1->position);
                double length = sqrt(a.x * a.x + a.y * a.y);
                a.x /= length;
                a.y /= length;
				chunk1->position -= (distance - num) * a * weightSymmetry * elasticity;
				chunk1->velocity -= (distance - num) * a * weightSymmetry * elasticity;
				chunk2->position += (distance - num) * a * (1 - weightSymmetry) * elasticity;
				chunk2->velocity += (distance - num) * a * (1 - weightSymmetry) * elasticity;
			}
        }
    

        BodyChunk *chunk1;
        BodyChunk *chunk2;

        double distance;

        BodyChunkConnection::Type type;

        double elasticity;
        double weightSymmetry;

        bool active = true;
};