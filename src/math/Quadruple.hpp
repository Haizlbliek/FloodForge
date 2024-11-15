#ifndef QUADRUPLE_HPP
#define QUADRUPLE_HPP

template <typename T1, typename T2, typename T3, typename T4>
class Quadruple {
	public:
		T1 first;
		T2 second;
		T3 third;
		T4 fourth;

		Quadruple(const T1 &firstVal, const T2 &secondVal, const T3 &thirdVal, const T4 &fourthVal)
		: first(firstVal),
		  second(secondVal),
		  third(thirdVal),
		  fourth(fourthVal) {
		}
};

#endif