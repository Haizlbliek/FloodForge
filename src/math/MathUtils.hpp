#pragma once

class MathUtils {
	public:
		static double lerp(double a, double b, double t) {
			return (b - a) * t + a;
		}
};