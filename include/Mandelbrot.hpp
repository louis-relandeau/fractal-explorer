#pragma once

#include <cstdint>

#include <FractalBase.hpp>

class Mandelbrot : public FractalBase {
public:
    Mandelbrot(sf::Image *image, Viewport *vp) : FractalBase(image, vp) {}

    void compute() override;

    int computePoint(double cr, double ci) const override;
};