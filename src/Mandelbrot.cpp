#include "Mandelbrot.hpp"

#include <array>
#include <cmath>
#include <cstdint>

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

    std::vector<std::vector<double>> iterCounts(y_end - y_start, std::vector<double>(imageWidth));
    double minIter = std::numeric_limits<double>::max();
    double maxIter = 0;
    for (std::size_t y = y_start; y < y_end; y++) {
        for (std::size_t x = 0; x < imageWidth; x++) {
            double cx = left + static_cast<double>(x) * dx;
            double cy = top - static_cast<double>(y) * dy;
            double n = computePoint(cx, cy);
            iterCounts[y - y_start][x] = n;
            if (n > 0) {
                if (n < minIter)
                    minIter = n;
                if (n > maxIter)
                    maxIter = n;
            }
        }
    }

    // const int HISTOGRAM_BINS = 1000;
    // std::vector<int> histogram(HISTOGRAM_BINS, 0);
    // int totalPoints = 0;

    // for (std::size_t y = y_start; y < y_end; y++) {
    //     for (std::size_t x = 0; x < imageWidth; x++) {
    //         double n = iterCounts[y - y_start][x];
    //         if (n > 0.0) {
    //             // Map to bin using LOG scale for histogram
    //             double logValue = std::log(n - minIter + 1.0) / std::log(maxIter - minIter + 1.0);
    //             int bin = static_cast<int>(logValue * (HISTOGRAM_BINS - 1));
    //             bin = std::max(0, std::min(HISTOGRAM_BINS - 1, bin));
    //             histogram[bin]++;
    //             totalPoints++;
    //         }
    //     }
    // }

    // std::vector<double> cumulative(HISTOGRAM_BINS, 0.0);
    // int sum = 0;
    // for (int i = 0; i < HISTOGRAM_BINS; i++) {
    //     sum += histogram[i];
    //     cumulative[i] = static_cast<double>(sum) / totalPoints;
    // }

    for (std::size_t y = y_start; y < y_end; y++) {
        for (std::size_t x = 0; x < imageWidth; x++) {
            double n = iterCounts[y - y_start][x]; // ← DOUBLE not int!
            sf::Color sfColor(0, 0, 0, 255);
            if (n > 0.0 && maxIter > minIter) {
                // Simple log mapping
                double t = std::log(n - minIter + 1.0) / std::log(maxIter - minIter + 1.0);
                sfColor = mapToPalette(t);
            }
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