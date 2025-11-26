#pragma once

#include <SFML/Graphics.hpp>

#include <FractalBase.hpp>

class Mandelbrot : public FractalBase {
public:
    Mandelbrot(sf::Image *image, Viewport *vp) : FractalBase(image, vp) {}

    void compute() override;

    double computePoint(double cr, double ci) const override;

private:
    sf::Color mapToPalette(double t);
};