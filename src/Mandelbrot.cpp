#include "Mandelbrot.hpp"

#include <array>
#include <cmath>
#include <cstdint>
#include <iostream>

void Mandelbrot::compute() {
    auto size = image->getSize();
    std::size_t imageWidth = size.x;
    std::size_t imageHeight = size.y;

    double dx = vp->width / static_cast<double>(imageWidth);
    double dy = vp->height / static_cast<double>(imageHeight);

    bool useOverlapOptimization = false;
    double offsetX = 0.0;
    double offsetY = 0.0;
    if (prevVp) {
        if (std::abs(vp->width - prevVp->width) < 1e-12 &&
            std::abs(vp->height - prevVp->height) < 1e-12) {
            offsetX = (prevVp->centerX - vp->centerX) / dx;
            offsetY = (prevVp->centerY - vp->centerY) / dy;
            useOverlapOptimization = true;
        }
    }

    double left = vp->centerX - vp->width * 0.5;
    double top = vp->centerY + vp->height * 0.5;

    std::vector<std::vector<double>> iterCounts(imageHeight, std::vector<double>(imageWidth, -1.0));

    double minIter = std::numeric_limits<double>::max();
    double maxIter = 0;

    for (std::size_t y = 0; y < imageHeight; y++) {
        for (std::size_t x = 0; x < imageWidth; x++) {
            double n = -1.0;
            bool reused = false;

            if (useOverlapOptimization) {
                double srcX = static_cast<double>(x) - offsetX;
                double srcY = static_cast<double>(y) + offsetY;
                if (srcX >= 0.0 && srcX < static_cast<double>(imageWidth) && srcY >= 0.0 &&
                    srcY < static_cast<double>(imageHeight)) {
                    std::size_t srcXIdx = static_cast<std::size_t>(std::round(srcX));
                    std::size_t srcYIdx = static_cast<std::size_t>(std::round(srcY));
                    if (prevIterCounts && srcYIdx < prevIterCounts->size() &&
                        srcXIdx < (*prevIterCounts)[srcYIdx].size()) {
                        n = (*prevIterCounts)[srcYIdx][srcXIdx];
                        reused = true;
                    }
                }
            }

            if (!reused) {
                double cx = left + static_cast<double>(x) * dx;
                double cy = top - static_cast<double>(y) * dy;
                n = computePoint(cx, cy);
            }

            iterCounts[y][x] = n;

            if (n > 0) {
                if (n < minIter)
                    minIter = n;
                if (n > maxIter)
                    maxIter = n;
            }
        }
    }

    prevIterCounts = iterCounts;
    prevVp = *vp;

    for (std::size_t y = 0; y < imageHeight; y++) {
        for (std::size_t x = 0; x < imageWidth; x++) {
            double n = iterCounts[y][x];
            sf::Color sfColor(0, 0, 0, 255);
            if (n > 0.0 && maxIter > minIter) {
                double t = std::log(n - minIter + 1.0) / std::log(maxIter - minIter + 1.0);
                sfColor = mapToPalette(t);
            }
            image->setPixel({static_cast<unsigned int>(x), static_cast<unsigned int>(y)}, sfColor);
        }
    }
}

// returns iteration count, or -1 if inside set
double Mandelbrot::computePoint(double cr, double ci) const {
    double crShifted = cr - 0.25;
    double q = crShifted * crShifted + ci * ci;
    if (q * (q + crShifted) < 0.25 * ci * ci) {
        return -1;
    }
    double crPlus1 = cr + 1.0;
    if (crPlus1 * crPlus1 + ci * ci < 0.0625) {
        return -1;
    }
    const int maxIterations = 2000;
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
        double log_zn = std::log(zr2 + zi2) / 2.0;
        double nu = std::log(log_zn / std::log(2.0)) / std::log(2.0);
        return n + 1.0 - nu;
    }
}

static const std::array<sf::Color, 7> colorPalette = {
    sf::Color(0, 7, 100),     // navy
    sf::Color(18, 0, 30),     // very dark purple
    sf::Color(60, 10, 80),    // purple
    sf::Color(20, 30, 90),    // dark blue
    sf::Color(80, 150, 255),  // light blue
    sf::Color(200, 255, 200), // white
    sf::Color(120, 200, 150), // soft green
};

sf::Color Mandelbrot::mapToPalette(double t) {
    if (t <= 0.0)
        return colorPalette.front();
    if (t >= 1.0)
        return colorPalette.back();

    double scaled = t * (colorPalette.size() - 1);
    std::size_t idx = static_cast<std::size_t>(scaled);
    double frac = scaled - idx;
    const sf::Color &c1 = colorPalette[idx];
    const sf::Color &c2 = colorPalette[std::min(idx + 1, colorPalette.size() - 1)];
    return sf::Color(static_cast<uint8_t>(c1.r + frac * (c2.r - c1.r)),
                     static_cast<uint8_t>(c1.g + frac * (c2.g - c1.g)),
                     static_cast<uint8_t>(c1.b + frac * (c2.b - c1.b)),
                     255);
}