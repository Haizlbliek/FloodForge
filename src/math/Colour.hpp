#pragma once

#include "MathUtils.hpp"

#define Color Colour

class Colour {
	public:
		Colour()
		 : r(0.0),
		   g(0.0),
		   b(0.0),
		   a(1.0) {
		}

		Colour(float v)
		: r(v),
		  g(v),
		  b(v),
		  a(1.0) {
		}

		Colour(float r, float g, float b)
		 : r(r),
		   g(g),
		   b(b),
		   a(1.0) {
		}

		Colour(float r, float g, float b, float a)
		 : r(r),
		   g(g),
		   b(b),
		   a(a) {}

		void copy(const Colour colour) {
			r = colour.r;
			g = colour.g;
			b = colour.b;
			a = colour.a;
		}

		// Colour mix(Colour other, double amount) {
		// 	return Colour(
		// 		MathUtils::lerp(r, other.r, amount),
		// 		MathUtils::lerp(g, other.g, amount),
		// 		MathUtils::lerp(b, other.b, amount),
		// 		MathUtils::lerp(a, other.a, amount)
		// 	);
		// }

		Colour mix(Colour other, double amount) {
			return Colour(
				MathUtils::lerp(r, r * other.r, amount),
				MathUtils::lerp(g, g * other.g, amount),
				MathUtils::lerp(b, b * other.b, amount),
				MathUtils::lerp(a, a * other.a, amount)
			);
		}
		
		float r;
		float g;
		float b;
		float a;
};