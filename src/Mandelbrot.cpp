#include "Mandelbrot.hpp"

std::uint32_t mandelbrotFunction(double cr, double ci) {
    const int maxIterations = 1000;
    double zr = 0.0, zi = 0.0;
    double zr2 = 0.0, zi2 = 0.0;
    
    int n = 0;
    while (zr2 + zi2 <= 4.0 && n < maxIterations) {
        zi = 2.0 * zr * zi + ci;
        zr = zr2 - zi2 + cr;
        zr2 = zr * zr;
        zi2 = zi * zi;
        ++n;
    }
    
    if (n == maxIterations) {
        return 0x000000;
    } else {
        unsigned char color = static_cast<unsigned char>(255 * n / maxIterations);
        return (color << 16) | (color << 8) | color;
    }
}