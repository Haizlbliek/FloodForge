#pragma once

#include <filesystem>

const std::filesystem::path BASE_PATH = std::filesystem::current_path();
const std::filesystem::path ASSETS_PATH = BASE_PATH / "assets";