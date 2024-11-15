#ifndef FONTS_HPP
#define FONTS_HPP

#include "Font.hpp"

namespace Fonts {
	extern Font *rainworld;
	extern Font *ancient;
	extern Font *rodondo;

	void init();
	void cleanup();
}

#endif