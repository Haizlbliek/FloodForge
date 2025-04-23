#pragma once

#include "Direction.hpp"
#include "Vector.hpp"

class MathUtils {
	public:
		static double lerp(double a, double b, double t) {
			return (b - a) * t + a;
		}
		
		static Vector2 directionToVector(Direction direction) {
			switch (direction) {
				case Direction::RIGHT: return Vector2( 1,  0);
				case Direction::UP:    return Vector2( 0,  1);
				case Direction::LEFT:  return Vector2(-1,  0);
				case Direction::DOWN:  return Vector2( 0, -1);
			}
			
			return Vector2(0, 0);
		}
};