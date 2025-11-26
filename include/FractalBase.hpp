#pragma once

#include <SFML/Graphics.hpp>

#include <Viewport.hpp>
#include <cstdint>
#include <string>
#include <vector>

class FractalBase {
public:
    virtual ~FractalBase() = default;
    FractalBase(sf::Image *image, Viewport *vp) : image(image), vp(vp) {}

    virtual void compute() = 0;

    /* Returns iteration count of a single point, or -1 if inside set */
    virtual double computePoint(double x, double y) const = 0;

    void backupAndReplacePointers(sf::Image *newImage, Viewport *newVp);
    void restoreBackedUpPointers();

protected:
    sf::Image *image;
    Viewport *vp;
    sf::Image *backupImage;
    Viewport *backupVp;
};