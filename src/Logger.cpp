#include "Logger.hpp"

#include <filesystem>
#include "Constants.hpp"

std::ofstream Logger::logFile;
std::mutex Logger::logMutex;

void Logger::init() {
	std::filesystem::copy_file(BASE_PATH / "log.txt", BASE_PATH / "log.old.txt", std::filesystem::copy_options::overwrite_existing);
	logFile = std::ofstream(BASE_PATH / "log.txt", std::ios::trunc);
}