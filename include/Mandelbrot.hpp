#pragma once

#include <cstdint>

#include <FractalBase.hpp>

class Mandelbrot : public FractalBase {
public:
    Mandelbrot(sf::Image *image, Viewport *vp) : FractalBase(image, vp) {}

    void compute() override;

    std::uint32_t computePoint(double cr, double ci) const override;
};