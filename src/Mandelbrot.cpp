#include "Mandelbrot.hpp"

#include <cmath>

void Mandelbrot::compute() {
    auto size = image->getSize();
    std::size_t imageWidth = size.x;
    std::size_t imageHeight = size.y;

    double dx = vp->width / static_cast<double>(imageWidth);
    double dy = vp->height / static_cast<double>(imageHeight);

    double left = vp->centerX - vp->width * 0.5;
    double top = vp->centerY + vp->height * 0.5;

    // x axis visible for symmetry?
    double yAxisPos = top / dy; // pixel row where cy = 0
    bool useSymmetry = (yAxisPos >= 0.0 && yAxisPos < static_cast<double>(imageHeight));

    std::size_t y_start = 0, y_end = imageHeight;
    if (useSymmetry) {
        // compute the larger half
        std::size_t axis_row = static_cast<std::size_t>(std::round(yAxisPos));
        std::size_t rows_above = axis_row + 1;
        std::size_t rows_below = imageHeight - axis_row - 1;
        if (rows_above >= rows_below) {
            y_start = 0;
            y_end = axis_row + 1;
        } else {
            y_start = axis_row;
            y_end = imageHeight;
        }
    }

    // First pass: compute iteration counts and find min/max
    std::vector<std::vector<int>> iterCounts(y_end - y_start, std::vector<int>(imageWidth));
    int minIter = std::numeric_limits<int>::max();
    int maxIter = 0;
    for (std::size_t y = y_start; y < y_end; y++) {
        for (std::size_t x = 0; x < imageWidth; x++) {
            double cx = left + static_cast<double>(x) * dx;
            double cy = top - static_cast<double>(y) * dy;
            int n = computePoint(cx, cy);
            iterCounts[y - y_start][x] = n;
            if (n > 0) {
                if (n < minIter)
                    minIter = n;
                if (n > maxIter)
                    maxIter = n;
            }
        }
    }

    // Second pass: map iteration counts to color using min/max
    for (std::size_t y = y_start; y < y_end; y++) {
        for (std::size_t x = 0; x < imageWidth; x++) {
            int n = iterCounts[y - y_start][x];
            std::uint32_t color = 0x000000;
            if (n > 0 && maxIter > minIter) {
                // Normalize n to [0, 1]
                double t = (static_cast<double>(n) - minIter) / (maxIter - minIter);
                unsigned char c = static_cast<unsigned char>(255 * t);
                color = (c << 16) | (c << 8) | c;
            }
            sf::Color sfColor((color >> 16) & 0xFF, // R
                              (color >> 8) & 0xFF,  // G
                              color & 0xFF,         // B
                              255                   // A always opaque
            );
            image->setPixel({static_cast<unsigned int>(x), static_cast<unsigned int>(y)}, sfColor);

            // symmetry maybe
            if (useSymmetry) {
                double cy = top - static_cast<double>(y) * dy;
                double cy_mirror = -cy;
                std::size_t mirror_y = static_cast<std::size_t>(std::round((top - cy_mirror) / dy));
                if (mirror_y != y && mirror_y < imageHeight) {
                    image->setPixel(
                        {static_cast<unsigned int>(x), static_cast<unsigned int>(mirror_y)},
                        sfColor);
                }
            }
        }
    }
}

// Returns iteration count, or -1 if inside set
int Mandelbrot::computePoint(double cr, double ci) const {
    double crShifted = cr - 0.25;
    double q = crShifted * crShifted + ci * ci;
    if (q * (q + crShifted) < 0.25 * ci * ci) {
        return -1;
    }
    double crPlus1 = cr + 1.0;
    if (crPlus1 * crPlus1 + ci * ci < 0.0625) {
        return -1;
    }
    const int maxIterations = 1000;
    double zr = 0.0, zi = 0.0;
    double zr2 = 0.0, zi2 = 0.0;
    double zrOld = 0.0, ziOld = 0.0;
    int checkPeriod = 20;
    int nextCheck = checkPeriod;
    int n = 0;
    while (zr2 + zi2 <= 4.0 && n < maxIterations) {
        zi = 2.0 * zr * zi + ci;
        zr = zr2 - zi2 + cr;
        zr2 = zr * zr;
        zi2 = zi * zi;
        ++n;
        if (n == nextCheck) {
            double diffR = zr - zrOld;
            double diffI = zi - ziOld;
            if (diffR * diffR + diffI * diffI < 1e-20) {
                return -1;
            }
            zrOld = zr;
            ziOld = zi;
            nextCheck += checkPeriod;
            checkPeriod *= 2;
        }
    }
    if (n == maxIterations) {
        return -1;
    } else {
        return n;
    }
}