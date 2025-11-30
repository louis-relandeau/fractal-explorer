#pragma once

#include <SFML/Graphics.hpp>

#include <FractalBase.hpp>

class Mandelbrot : public FractalBase {
public:
    Mandelbrot(sf::Image *image, Viewport *vp);

    void compute() override;

    double computePoint(double cr, double ci) const override;

private:
    sf::Color mapToPalette(double t);

    void initPaletteCache();
    sf::Color getPaletteColor(double t) const;

    const int maxIterations;
    std::vector<sf::Color> colorPalette;
    std::vector<sf::Color> paletteCache;
    static constexpr int PALETTE_CACHE_SIZE = 4096;
};