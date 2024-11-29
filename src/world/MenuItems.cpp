#include "MenuItems.hpp"

std::vector<Button*> MenuItems::buttons;

Window *MenuItems::window = nullptr;

double MenuItems::currentButtonX = -0.99;

std::filesystem::path MenuItems::exportDirectory = "";
std::string MenuItems::worldAcronym = "";

std::string MenuItems::extraProperties = "";
std::string MenuItems::extraWorld = "";