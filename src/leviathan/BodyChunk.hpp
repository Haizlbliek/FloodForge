#pragma once

#include "math.h"

#include "../math/Vector.hpp"

#include "PhysicalObject.hpp"

class BodyChunk {
    public:
        BodyChunk(PhysicalObject *owner, Vector2 position, double radius, double mass) {
            this->owner = owner;
            this->position = position;
            this->radius = radius;
            this->mass = mass;
            this->velocity = Vector2(0, 0);
        }

        void update() {
            if (isnan(velocity.y)) {
                velocity.y = 0;
            }

            if (isnan(velocity.x)) {
                velocity.x = 0;
            }

            velocity.y = velocity.y - owner->gravity;

            velocity *= owner->airFriction;

            // this.lastLastPos = this.lastPos;
            // this.lastPos = this.pos;
            // if (this.setPos != null) {
            //     this.pos = this.setPos.Value;
            //     this.setPos = null;
            // }
            // else
            // {
            position += velocity;
            // }
            // this.onSlope = 0;
            // this.slopeRad = this.TerrainRad;

            // this.lastContactPoint = this.contactPoint;
            // if (this.collideWithTerrain)
            // {
            //     this.CheckVerticalCollision();
            //     if (this.collideWithSlopes)
            //     {
            //         this.checkAgainstSlopesVertically();
            //     }
            //     this.CheckHorizontalCollision();
            // }
            // else
            // {
            //     this.contactPoint.x = 0;
            //     this.contactPoint.y = 0;
            // }

            // if (this.pos.x < -1200) {
            //     this.vel.x = 0f;
            //     this.pos.x = -1200;
            // } else if (this.pos.x > this.owner.room.PixelWidth + 1200) {
            //     this.vel.x = 0f;
            //     this.pos.x = this.owner.room.PixelWidth + 1200;
            // }
            // if (this.pos.y < -1200) {
            //     this.vel.y = 0f;
            //     this.pos.y = -1200;
            // } else if (this.pos.y > this.owner.room.PixelHeight + 1200) {
            //     this.vel.y = 0f;
            //     this.pos.y = this.owner.room.PixelHeight + 1200;
            // }

            if (position.y - radius <= -10.0) {
                position.y = -10.0 + radius;
                velocity.y = 0;
                velocity.x *= std::clamp(owner->surfaceFriction * 2.0, 0.0, 1.0);
            }
        }
    
        Vector2 velocity;
        Vector2 position;
        double radius;
        double mass;

        PhysicalObject *owner;
};