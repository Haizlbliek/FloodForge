#pragma once

#include <string>

class Den {
    public:
        Den(std::string type, int count, std::string tag, double data) {
            this->type = type;
            this->count = count;
            this->tag = tag;
            this->data = data;
        }
        
        std::string type;
        int count;
        std::string tag;
        double data;
};