#pragma once

#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <mutex>

namespace Logger {
	extern std::ofstream logFile;
	extern std::mutex logMutex;

	inline void writeStream(std::ostream&) {}

	template<typename T>
	inline void writeStream(std::ostream &os, const T &value) {
		os << value;
	}
	
	template<typename T, typename... Args>
	inline void writeStream(std::ostream &os, const T &first, const Args&... rest) {
		os << first;
		writeStream(os, rest...);
	}
	
	template<typename... Args>
	inline void log(const Args&... args) {
		std::lock_guard<std::mutex> lock(logMutex);
	
		std::ostringstream oss;
		writeStream(oss, args...);
		std::string line = oss.str();
	
		std::cout << line << std::endl;
		if (logFile.is_open()) {
			logFile << line << std::endl;
			logFile.flush();
		}
	}
	
	template<typename... Args>
	inline void logError(const Args&... args) {
		std::lock_guard<std::mutex> lock(logMutex);
	
		std::ostringstream oss;
		writeStream(oss, args...);
		std::string line = oss.str();
	
		std::cerr << line << std::endl;
		if (logFile.is_open()) {
			logFile << line << std::endl;
			logFile.flush();
		}
	}
}