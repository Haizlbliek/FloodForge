#include "Logger.hpp"

std::ofstream Logger::logFile("log.txt", std::ios::trunc);
std::mutex Logger::logMutex;
