#pragma once

#include <string>

class ConfigLoader {
public:
    ConfigLoader(std::string const& filename);
    
    struct WindowParams {
        unsigned width;
        unsigned height;
    } windowParams;

    struct FractalParams {
        std::string name;
    } fractalParams;
};