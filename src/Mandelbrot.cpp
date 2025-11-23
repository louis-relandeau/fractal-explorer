#include "Mandelbrot.hpp"

#include <cmath>
#include <iostream>

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

    for (std::size_t y = y_start; y < y_end; y++) {
        std::size_t x = 0;

        while (x < imageWidth) {
            double cx = left + static_cast<double>(x) * dx;
            double cy = top - static_cast<double>(y) * dy;
            const auto [color, distanceEstimate] = computePoint(cx, cy);
            sf::Color sfColor((color >> 16) & 0xFF, // R
                              (color >> 8) & 0xFF,  // G
                              color & 0xFF,         // B
                              255                   // A always opaque
            );

            size_t distanceEstimatePx = 1;
            if (distanceEstimate > 0) {
                distanceEstimatePx =
                    std::max(static_cast<std::size_t>(distanceEstimate / dx * 0.9), size_t(1));
            }

            std::size_t endX = std::min(x + distanceEstimatePx, imageWidth);
            for (std::size_t fillX = x; fillX < endX; fillX++) {
                image->setPixel({static_cast<unsigned int>(fillX), static_cast<unsigned int>(y)},
                                sfColor);

                // symmetry maybe
                if (useSymmetry) {
                    double cy_mirror = -cy;
                    std::size_t mirror_y =
                        static_cast<std::size_t>(std::round((top - cy_mirror) / dy));
                    if (mirror_y != y && mirror_y < imageHeight) {
                        image->setPixel(
                            {static_cast<unsigned int>(fillX), static_cast<unsigned int>(mirror_y)},
                            sfColor);
                    }
                }
            }

            x += distanceEstimatePx;
        }
    }
}

std::pair<std::uint32_t, double> Mandelbrot::computePoint(double cr, double ci) const {
    double distanceEstimate = 0.0;

    // main cardioid: (q(q + (cr - 0.25)) < 0.25 * ci^2) where q = (cr - 0.25)^2 + ci^2
    double crShifted = cr - 0.25;
    double q = crShifted * crShifted + ci * ci;
    if (q * (q + crShifted) < 0.25 * ci * ci) {
        return {0x000000, distanceEstimate};
    }

    // period-2 bulb: (cr + 1)^2 + ci^2 < 0.0625 (radius 0.25 circle centered at (-1, 0))
    double crPlus1 = cr + 1.0;
    if (crPlus1 * crPlus1 + ci * ci < 0.0625) {
        return {0x000000, distanceEstimate};
    }

    const int maxIterations = 1000;
    double zr = 0.0, zi = 0.0;
    double zr2 = 0.0, zi2 = 0.0;

    double dzr = 1.0, dzi = 0.0;
    double zrOld = 0.0, ziOld = 0.0;

    int checkPeriod = 20;
    int nextCheck = checkPeriod;

    int n = 0;
    while (n < maxIterations) {
        double temp_zr = zr * zr - zi * zi + cr;
        double temp_zi = 2.0 * zr * zi + ci;

        double temp_dzr = 2.0 * (zr * dzr - zi * dzi) + 1.0;
        double temp_dzi = 2.0 * (zr * dzi + zi * dzr);

        zr = temp_zr;
        zi = temp_zi;
        dzr = temp_dzr;
        dzi = temp_dzi;

        zr2 = zr * zr;
        zi2 = zi * zi;
        ++n;

        // periodicity check
        if (n == nextCheck) {
            double diffR = zr - zrOld;
            double diffI = zi - ziOld;
            if (diffR * diffR + diffI * diffI < 1e-20) {
                return {0x000000, distanceEstimate};
            }
            zrOld = zr;
            ziOld = zi;
            nextCheck += checkPeriod;
            checkPeriod *= 2; // exponential backoff
        }

        if (zr2 + zi2 > 4.0) {
            double z_mag = std::sqrt(zr2 + zi2);
            double dz_mag = std::sqrt(dzr * dzr + dzi * dzi);
            distanceEstimate = 0.5 * z_mag * std::log(z_mag) / dz_mag;
            break;
        }
    }

    if (n == maxIterations) {
        return {0x000000, distanceEstimate};
    } else {
        unsigned char color = static_cast<unsigned char>(255 * n / maxIterations);
        return {(color << 16) | (color << 8) | color, distanceEstimate};
    }
}