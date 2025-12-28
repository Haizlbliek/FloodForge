#include "Fonts.hpp"

namespace Fonts {
	Font *rainworld;
	Font *rodondo;
}

void Fonts::init() {
	Logger::info("Loading fonts...");
	Fonts::rainworld = new Font("rainworld", FONT_SHARP);
	Fonts::rodondo = new Font("Rodondo", FONT_SMOOTH);
	Logger::info("Fonts loaded");
	Logger::info();
}

void Fonts::cleanup() {
	delete Fonts::rainworld;
	delete Fonts::rodondo;
}