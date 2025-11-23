#include "FractalBase.hpp"

void FractalBase::backupAndReplacePointers(sf::Image *newImage, Viewport *newVp) {
    backupImage = image;
    backupVp = vp;
    image = newImage;
    vp = newVp;
}

void FractalBase::restoreBackedUpPointers() {
    image = backupImage;
    vp = backupVp;
}