#pragma once

#include <string>
#include <filesystem>

const std::string BASE_PATH = std::filesystem::path(__FILE__).parent_path().string() + "/../";