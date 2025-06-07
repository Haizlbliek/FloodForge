#include "Logger.hpp"

std::ofstream Logger::logFile("log.txt", std::ios::app);
std::mutex Logger::logMutex;
