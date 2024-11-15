#include <string>
#include <filesystem>

#ifndef BASE_CONSTANTS_HPP
#define BASE_CONSTANTS_HPP

const std::string BASE_PATH = std::filesystem::path(__FILE__).parent_path().string() + "/../";

#endif