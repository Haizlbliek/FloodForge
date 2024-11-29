#ifndef MATH_UTILS_HPP
#define MATH_UTILS_HPP

class MathUtils {
	public:
		static double lerp(double a, double b, double t) {
			return (b - a) * t + a;
		}
};

#endif