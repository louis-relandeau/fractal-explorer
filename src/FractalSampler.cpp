#include "FractalSampler.hpp"

#include "Mandelbrot.hpp"
#include <SFML/Graphics.hpp>
#include <iostream>
#include <vector>

// Forward declarations for AVX/scalar row functions
void mandelbrotAVX(const double *cr, const double *ci, std::uint32_t *results);
std::uint32_t mandelbrotFunction(double cr, double ci);
static void
mandelbrotProcessRow(int width, double y, double x_start, double x_step, std::uint32_t *output) {
    int x = 0;
    for (; x <= width - 4; x += 4) {
        double cr[4] = {x_start + x * x_step,
                        x_start + (x + 1) * x_step,
                        x_start + (x + 2) * x_step,
                        x_start + (x + 3) * x_step};
        double ci[4] = {y, y, y, y};
        mandelbrotAVX(cr, ci, &output[x]);
    }
    for (; x < width; x++) {
        double cr = x_start + x * x_step;
        output[x] = mandelbrotFunction(cr, y);
    }
}

FractalSampler::FractalSampler(std::string const &fractalName)
    : fractalName(fractalName), fractalFunction(nullptr), fractalRowFunction(nullptr) {
    if (fractalName == "Mandelbrot") {
        fractalFunction = mandelbrotFunction;
        fractalRowFunction = mandelbrotProcessRow;
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

    std::vector<std::uint32_t> row_buffer(image_w);
    for (std::size_t y = 0; y < image_h; y++) {
        double cy = top - static_cast<double>(y) * dy;
        fractalRowFunction(static_cast<int>(image_w), cy, left, dx, row_buffer.data());
        for (std::size_t x = 0; x < image_w; x++) {
            std::uint32_t color = row_buffer[x];
            sf::Color sfColor((color >> 16) & 0xFF, // R
                              (color >> 8) & 0xFF,  // G
                              color & 0xFF,         // B
                              255                   // A always opaque
            );
            image.setPixel({static_cast<unsigned int>(x), static_cast<unsigned int>(y)}, sfColor);
        }
    }
}
