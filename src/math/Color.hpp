#pragma once

#include "MathUtils.hpp"

class Color {
	public:
		Color()
		 : r(0.0),
		   g(0.0),
		   b(0.0),
		   a(1.0) {
		}

		Color(float v)
		: r(v),
		  g(v),
		  b(v),
		  a(1.0) {
		}

		Color(float r, float g, float b)
		 : r(r),
		   g(g),
		   b(b),
		   a(1.0) {
		}

		Color(float r, float g, float b, float a)
		 : r(r),
		   g(g),
		   b(b),
		   a(a) {}

		void copy(const Color color) {
			r = color.r;
			g = color.g;
			b = color.b;
			a = color.a;
		}

		Color mix(Color other, double amount) {
			return Color(
				MathUtils::lerp(r, r * other.r, amount),
				MathUtils::lerp(g, g * other.g, amount),
				MathUtils::lerp(b, b * other.b, amount),
				MathUtils::lerp(a, a * other.a, amount)
			);
		}

		Vector3f HSV() {
			float cmax = std::max(r, std::max(g, b));
			float cmin = std::min(r, std::min(g, b));
			float delta = cmax - cmin;

			float h = 0.0f, s = 0.0f, v = cmax;

			if (delta > 0.0f) {
				if (cmax == r) {
					h = 60.0f * std::fmod(((g - b) / delta), 6.0f);
				} else if (cmax == g) {
					h = 60.0f * (((b - r) / delta) + 2.0f);
				} else {
					h = 60.0f * (((r - g) / delta) + 4.0f);
				}
				if (h < 0.0f) h += 360.0f;
			}

			if (cmax > 0.0f) {
				s = delta / cmax;
			}

			return { h, s, v };
		}

		void HSV(Vector3f from) {
			float h = from.x;
			float s = from.y;
			float v = from.z;

			float c = v * s;
			float x = c * (1.0f - std::fabs(std::fmod(h / 60.0f, 2.0f) - 1.0f));
			float m = v - c;

			float r1 = 0, g1 = 0, b1 = 0;
			if (h < 60) {
				r1 = c; g1 = x; b1 = 0;
			} else if (h < 120) {
				r1 = x; g1 = c; b1 = 0;
			} else if (h < 180) {
				r1 = 0; g1 = c; b1 = x;
			} else if (h < 240) {
				r1 = 0; g1 = x; b1 = c;
			} else if (h < 300) {
				r1 = x; g1 = 0; b1 = c;
			} else {
				r1 = c; g1 = 0; b1 = x;
			}

			r = r1 + m;
			g = g1 + m;
			b = b1 + m;
		}

		float r;
		float g;
		float b;
		float a;
};