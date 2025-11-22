#include "FractalSampler.hpp"

#include <iostream>

#include <SFML/Graphics.hpp>

#include "Mandelbrot.hpp"

FractalSampler::FractalSampler(std::string const &fractalName)
    : fractalName(fractalName), fractalFunction(nullptr) {
    if (fractalName == "Mandelbrot") {
        fractalFunction = mandelbrotFunction;
    } else {
        throw std::runtime_error("Unknown fractal name: " + fractalName);
    }
}

void FractalSampler::compute(sf::Image &image, const Viewport &vp) {
    auto size = image.getSize();
    std::size_t image_w = size.x;
    std::size_t image_h = size.y;

    double dx = vp.width / static_cast<double>(image_w);
    double dy = vp.height / static_cast<double>(image_h);

    double left = vp.centerX - vp.width * 0.5;
    double top = vp.centerY + vp.height * 0.5;

    for (std::size_t y = 0; y < image_h; y++) {
        for (std::size_t x = 0; x < image_w; x++) {
            double cx = left + static_cast<double>(x) * dx;
            double cy = top - static_cast<double>(y) * dy;
            std::uint32_t color = fractalFunction(cx, cy);
            sf::Color sfColor((color >> 16) & 0xFF, // R
                              (color >> 8) & 0xFF,  // G
                              color & 0xFF,         // B
                              255  // A always opaque
            );
            image.setPixel({static_cast<unsigned int>(x), static_cast<unsigned int>(y)}, sfColor);
        }
    }
}
