#include "Mandelbrot.hpp"

#include <cmath>
#include <cstdint>
#include <iostream>

Mandelbrot::Mandelbrot(sf::Image *image, Viewport *vp)
    : FractalBase(image, vp),
      maxIterations(2000),
      colorPalette{
          sf::Color(0, 7, 100),     // navy
          sf::Color(18, 0, 30),     // very dark purple
          sf::Color(60, 10, 80),    // purple
          sf::Color(20, 30, 90),    // dark blue
          sf::Color(80, 150, 255),  // light blue
          sf::Color(200, 255, 200), // white
          sf::Color(120, 200, 150), // soft green
      } {
    initPaletteCache();
}

void Mandelbrot::compute() {
    auto size = image->getSize();
    std::size_t imageWidth = size.x;
    std::size_t imageHeight = size.y;
    std::size_t totalPixels = imageWidth * imageHeight;

    double dx = vp->width / static_cast<double>(imageWidth);
    double dy = vp->height / static_cast<double>(imageHeight);

    std::vector<double> iterCounts(totalPixels, -1.0);

    bool useOverlapOptimization = false;
    double offsetX = 0.0;
    double offsetY = 0.0;

    if (hasPrevVp) {
        if (std::abs(vp->width - prevVp.width) < 1e-12 &&
            std::abs(vp->height - prevVp.height) < 1e-12) {
            offsetX = (prevVp.centerX - vp->centerX) / dx;
            offsetY = (prevVp.centerY - vp->centerY) / dy;
            useOverlapOptimization = true;
        }
    }

    double left = vp->centerX - vp->width * 0.5;
    double top = vp->centerY + vp->height * 0.5;

    double minIter = std::numeric_limits<double>::max();
    double maxIter = 0;

#pragma omp parallel for collapse(2) reduction(min : minIter) reduction(max : maxIter)
    for (std::size_t y = 0; y < imageHeight; y++) {
        for (std::size_t x = 0; x < imageWidth; x++) {
            std::size_t idx = y * imageWidth + x;
            double n = -1.0;
            bool reused = false;

            if (useOverlapOptimization && !prevIterCounts.empty()) {
                double srcX = static_cast<double>(x) - offsetX;
                double srcY = static_cast<double>(y) + offsetY;

                if (srcX >= 0.0 && srcX < static_cast<double>(imageWidth) && srcY >= 0.0 &&
                    srcY < static_cast<double>(imageHeight)) {

                    std::size_t srcIdx = static_cast<std::size_t>(srcY + 0.5) * imageWidth +
                                         static_cast<std::size_t>(srcX + 0.5);

                    if (srcIdx < prevIterCounts.size()) {
                        n = prevIterCounts[srcIdx];
                        reused = true;
                    }
                }
            }

            if (!reused) {
                double cx = left + static_cast<double>(x) * dx;
                double cy = top - static_cast<double>(y) * dy;
                n = computePoint(cx, cy);
            }

            iterCounts[idx] = n;

            if (n > 0) {
                minIter = std::min(minIter, n);
                maxIter = std::max(maxIter, n);
            }
        }
    }

    prevIterCounts = std::move(iterCounts);
    prevVp = *vp;
    hasPrevVp = true;

    double logScale = 1.0 / std::log(maxIter - minIter + 1.0);

#pragma omp parallel for collapse(2)
    for (std::size_t y = 0; y < imageHeight; y++) {
        for (std::size_t x = 0; x < imageWidth; x++) {
            std::size_t idx = y * imageWidth + x;
            double n = prevIterCounts[idx];

            sf::Color sfColor(0, 0, 0, 255);
            if (n > 0.0 && maxIter > minIter) {
                double t = std::log(n - minIter + 1.0) * logScale;
                sfColor = getPaletteColor(t);
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

void Mandelbrot::initPaletteCache() {
    paletteCache.resize(PALETTE_CACHE_SIZE);

    for (int i = 0; i < PALETTE_CACHE_SIZE; i++) {
        double t = i / static_cast<double>(PALETTE_CACHE_SIZE - 1);

        if (t <= 0.0) {
            paletteCache[i] = colorPalette.front();
        } else if (t >= 1.0) {
            paletteCache[i] = colorPalette.back();
        } else {
            double scaled = t * (colorPalette.size() - 1);
            std::size_t idx = static_cast<std::size_t>(scaled);
            double frac = scaled - idx;

            const sf::Color &c1 = colorPalette[idx];
            const sf::Color &c2 = colorPalette[std::min(idx + 1, colorPalette.size() - 1)];

            paletteCache[i] = sf::Color(static_cast<uint8_t>(c1.r + frac * (c2.r - c1.r)),
                                        static_cast<uint8_t>(c1.g + frac * (c2.g - c1.g)),
                                        static_cast<uint8_t>(c1.b + frac * (c2.b - c1.b)),
                                        255);
        }
    }
}

inline sf::Color Mandelbrot::getPaletteColor(double t) const {
    if (t <= 0.0)
        return paletteCache.front();
    if (t >= 1.0)
        return paletteCache.back();

    int index = static_cast<int>(t * (PALETTE_CACHE_SIZE - 1) + 0.5);
    return paletteCache[index];
}