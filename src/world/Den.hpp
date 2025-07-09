#pragma once

#include <string>

class Den {
	public:
		Den(std::string type, int count, std::string tag, double data);
		
		std::string type;
		int count;
		std::string tag;
		double data;
};