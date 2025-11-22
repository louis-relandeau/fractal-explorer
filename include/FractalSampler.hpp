#pragma once

#include <SFML/Graphics.hpp>

#include <Viewport.hpp>
#include <cstdint>
#include <string>
#include <vector>

class FractalSampler {
public:
    FractalSampler(std::string const &fractalName);

    void compute(sf::Image &image, const Viewport &vp);

private:
    using FractalFn = std::uint32_t (*)(double cr, double ci);
    using FractalRowFn =
        void (*)(int width, double y, double x_start, double x_step, std::uint32_t *output);
    std::string fractalName;
    FractalFn fractalFunction;
    FractalRowFn fractalRowFunction;
};