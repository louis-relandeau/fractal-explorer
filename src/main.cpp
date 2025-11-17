#include <vector>
#include <cmath>
#include <limits>
#include <chrono>
#include <iostream>
#include <cstdint>

#include <SFML/Graphics.hpp>
#include <yaml-cpp/yaml.h>

using std::size_t;

struct DERes {
    double smooth;
    double dist;
    int iter;
};

int MAX_HORIZONTAL_SKIP_PIXELS;
double EARLY_EXIT_MULTIPLIER;
double SAFETY_FACTOR;
double DETAIL_MULTIPLIER;
double DZ_GUARD;

inline DERes mandelbrotDEScalar(double cr, double ci, int maxIter, double pixelSize) {
    double zr = 0.0, zi = 0.0;
    double dzr = 0.0, dzi = 0.0;
    int n = 0;
    double dist = pixelSize;

    while (n < maxIter) {
        // dz = 2*z*dz + 1
        double two_z_dzr = 2.0 * (zr * dzr - zi * dzi);
        double two_z_dzi = 2.0 * (zr * dzi + zi * dzr);
        dzr = two_z_dzr + 1.0;
        dzi = two_z_dzi;

        // z = z*z + c
        double zr2 = zr*zr - zi*zi + cr;
        double zi2 = 2.0*zr*zi + ci;
        zr = zr2; zi = zi2;

        ++n;

        double r2 = zr*zr + zi*zi;
        if (r2 > 4.0) {
            // |z| and |dz|
            double r = std::sqrt(r2);
            double dd = std::sqrt(dzr*dzr + dzi*dzi);
            if (dd < DZ_GUARD) dd = DZ_GUARD;
            // dist = 2 * |z| * ln|z| / |dz|
            double lnz = std::log(r);
            dist = 2.0 * r * lnz / dd;
            if (dist > pixelSize * EARLY_EXIT_MULTIPLIER) {
                break;
            }
            break;
        }
    }

    double smooth = double(n);
    if (n < maxIter) {
        double absz = std::sqrt(zr*zr + zi*zi);
        if (absz < 1e-300) absz = 1e-300;
        smooth = n + 1.0 - std::log(std::log(absz)) / std::log(2.0);
    }

    return {smooth, dist, n};
}

sf::Color hsvToRgb(double h, double s, double v) {
    double c = v * s;
    double x = c * (1.0 - std::fabs(std::fmod(h / 60.0, 2.0) - 1.0));
    double m = v - c;
    double r,g,b;
    if (h < 60.0)      { r = c; g = x; b = 0; }
    else if (h < 120.0){ r = x; g = c; b = 0; }
    else if (h < 180.0){ r = 0; g = c; b = x; }
    else if (h < 240.0){ r = 0; g = x; b = c; }
    else if (h < 300.0){ r = x; g = 0; b = c; }
    else               { r = c; g = 0; b = x; }
    uint8_t R = uint8_t(std::round(255.0*(r+m)));
    uint8_t G = uint8_t(std::round(255.0*(g+m)));
    uint8_t B = uint8_t(std::round(255.0*(b+m)));
    return sf::Color(R,G,B);
}

sf::Color mapColorLogScale(double n) {
    if (n >= 1.0) return sf::Color::Black;
    double hue = 360.0 * n;
    double sat = 0.8;
    double val = 1.0 - n * 0.8;
    return hsvToRgb(hue, sat, val);
}

inline size_t idx(unsigned x, unsigned y, unsigned width) { return size_t(y) * size_t(width) + size_t(x); }

void renderCombinedFast(
    sf::Image &image,
    int maxIter,
    double zoom,
    double offsetX,
    double offsetY,
    std::vector<double> &prevSmoothFlat,
    const sf::Image* prevImage = nullptr,
    int dx = 0,
    int dy = 0
) {
    auto sz = image.getSize();
    unsigned width = sz.x;
    unsigned height = sz.y;
    size_t N = size_t(width) * size_t(height);

    std::vector<double> smoothFlat(N, std::numeric_limits<double>::quiet_NaN());
    std::vector<char>   needsDetail(N, 1);
    std::vector<int>    iterFlat(N, 0);

    std::vector<uint8_t> pixels;
    pixels.resize(N * 4);

    double pixelSizeX = (4.0 / double(width)) / zoom;
    double pixelSizeY = (4.0 / double(height)) / zoom;
    double pixelSize = pixelSizeX;

    double minVal = std::numeric_limits<double>::max();
    double maxVal = std::numeric_limits<double>::lowest();

    if (prevImage && prevSmoothFlat.size() == N && (dx != 0 || dy != 0)) {
        for (unsigned y = 0; y < height; ++y) {
            for (unsigned x = 0; x < width; ++x) {
                int px = int(x) + dx;
                int py = int(y) + dy;
                size_t id = idx(x,y,width);
                if (px >= 0 && px < int(width) && py >= 0 && py < int(height)) {
                    size_t pid = idx(unsigned(px), unsigned(py), width);
                    smoothFlat[id] = prevSmoothFlat[pid];
                    needsDetail[id] = 0;
                    sf::Color c = prevImage->getPixel({unsigned(px), unsigned(py)});
                    pixels[4*id + 0] = c.r;
                    pixels[4*id + 1] = c.g;
                    pixels[4*id + 2] = c.b;
                    pixels[4*id + 3] = 255;
                    if (!std::isnan(smoothFlat[id])) {
                        if (smoothFlat[id] < minVal) minVal = smoothFlat[id];
                        if (smoothFlat[id] > maxVal) maxVal = smoothFlat[id];
                    }
                }
            }
            std::cout << "Rendered row " << y << " / " << height << std::flush << "\r";
        }
    }
    
    for (unsigned y = 0; y < height; ++y) {
        unsigned x = 0;
        while (x < width) {
            size_t id = idx(x,y,width);
            if (!needsDetail[id]) { ++x; continue; }
            
            double cr = (double(x) - double(width)/2.0) * (4.0/double(width)) / zoom + offsetX;
            double ci = (double(y) - double(height)/2.0) * (4.0/double(height)) / zoom + offsetY;
            
            DERes de = mandelbrotDEScalar(cr, ci, maxIter, pixelSize);
            
            double rep = de.smooth;
            if (!std::isnan(rep)) {
                if (rep < minVal) minVal = rep;
                if (rep > maxVal) maxVal = rep;
            }
            
            if (de.dist <= pixelSize * DETAIL_MULTIPLIER) {
                needsDetail[id] = 1;
                smoothFlat[id] = std::numeric_limits<double>::quiet_NaN();
                iterFlat[id] = de.iter;
                ++x;
                continue;
            }
            
            double skipPixelsD = (de.dist / pixelSize) * SAFETY_FACTOR;
            int skip = int(std::floor(skipPixelsD));
            if (skip < 1) skip = 1;
            if (skip > MAX_HORIZONTAL_SKIP_PIXELS) skip = MAX_HORIZONTAL_SKIP_PIXELS;
            if (skip > int(width) - int(x)) skip = int(width) - int(x);
            
            for (int k = 0; k < skip; ++k) {
                unsigned xx = x + k;
                size_t bid = idx(xx, y, width);
                smoothFlat[bid] = de.smooth;
                iterFlat[bid] = de.iter;
                needsDetail[bid] = 0;
                double norm = (maxVal > minVal) ? (de.smooth - minVal) / (maxVal - minVal) : 0.0;
                sf::Color col = (de.iter >= maxIter) ? sf::Color::Black : mapColorLogScale(norm);
                pixels[4*bid + 0] = col.r;
                pixels[4*bid + 1] = col.g;
                pixels[4*bid + 2] = col.b;
                pixels[4*bid + 3] = 255;
            }
            x += skip;
        }
        std::cout << "Rendered row " << y << " / " << height << std::flush << "\r";
    }

    for (unsigned y = 0; y < height; ++y) {
        for (unsigned x = 0; x < width; ++x) {
            size_t id = idx(x,y,width);
            if (!needsDetail[id]) continue;

            double cr = (double(x) - double(width)/2.0) * (4.0/double(width)) / zoom + offsetX;
            double ci = (double(y) - double(height)/2.0) * (4.0/double(height)) / zoom + offsetY;

            DERes de = mandelbrotDEScalar(cr, ci, maxIter, pixelSize);

            smoothFlat[id] = de.smooth;
            iterFlat[id] = de.iter;
            needsDetail[id] = 0;

            if (de.smooth < minVal) minVal = de.smooth;
            if (de.smooth > maxVal) maxVal = de.smooth;

            double norm = (maxVal > minVal) ? (de.smooth - minVal) / (maxVal - minVal) : 0.0;
            sf::Color col = (de.iter >= maxIter) ? sf::Color::Black : mapColorLogScale(norm);
            pixels[4*id + 0] = col.r;
            pixels[4*id + 1] = col.g;
            pixels[4*id + 2] = col.b;
            pixels[4*id + 3] = 255;
        }
    }

    bool validRange = (maxVal > minVal);
    for (unsigned y = 0; y < height; ++y) {
        for (unsigned x = 0; x < width; ++x) {
            size_t id = idx(x,y,width);
            double s = smoothFlat[id];
            if (std::isnan(s)) {
                pixels[4*id + 0] = 0;
                pixels[4*id + 1] = 0;
                pixels[4*id + 2] = 0;
                pixels[4*id + 3] = 255;
                continue;
            }
            double norm = validRange ? (s - minVal) / (maxVal - minVal) : 0.0;
            sf::Color col = (iterFlat[id] >= maxIter) ? sf::Color::Black : mapColorLogScale(norm);
            pixels[4*id + 0] = col.r;
            pixels[4*id + 1] = col.g;
            pixels[4*id + 2] = col.b;
            pixels[4*id + 3] = 255;
        }
    }

    image = sf::Image(sf::Vector2u(width, height), sf::Color::Black);
    for (unsigned y = 0; y < height; ++y) {
        for (unsigned x = 0; x < width; ++x) {
            size_t id = idx(x,y,width);
            sf::Color c(pixels[4*id+0], pixels[4*id+1], pixels[4*id+2], 255);
            image.setPixel({x, y}, c);
        }
    }

    prevSmoothFlat.swap(smoothFlat);
}

void saveCurrentViewAsImage(unsigned imageWidth, unsigned imageHeight, int maxIter, double zoom, double offsetX, double offsetY) {
    std::cout << "Saving current view as image..." << std::endl;
    std::cout << "At position (" << offsetX << ", " << offsetY << ") with zoom " << zoom << std::endl;
    sf::Image image(sf::Vector2u(imageWidth, imageHeight));
    std::vector<double> smoothFlat(imageWidth * imageHeight, 0.0);
    renderCombinedFast(image, maxIter, zoom, offsetX, offsetY, smoothFlat, nullptr, 0, 0);
    std::string filename = "fractal.png";
    if (image.saveToFile(filename)) {
        std::cout << "Saved image to " << filename << std::endl;
    } else {
        std::cerr << "Failed to save image!" << std::endl;
    }
}

template<typename F>
void timeFunction(F f) {
    auto t0 = std::chrono::high_resolution_clock::now();
    f();
    auto t1 = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double> d = t1 - t0;
    std::cout << "Elapsed: " << d.count() << " s\n";
}

int main() {
    YAML::Node config = YAML::LoadFile("params.yaml");

    const auto renderNode = config["Render"];
    MAX_HORIZONTAL_SKIP_PIXELS = renderNode["MaxHorizontalSkipPixels"].as<int>();
    EARLY_EXIT_MULTIPLIER = renderNode["EarlyExitMultiplier"].as<double>();
    SAFETY_FACTOR = renderNode["SafetyFactor"].as<double>();
    DETAIL_MULTIPLIER = renderNode["DetailMultiplier"].as<double>();
    DZ_GUARD = renderNode["DZGuard"].as<double>();

    const auto windowNode = config["Window"];
    const unsigned width = windowNode["Width"].as<unsigned>();
    const unsigned height = windowNode["Height"].as<unsigned>();
    const unsigned imageWidth = windowNode["ImageWidth"].as<unsigned>();
    const unsigned imageHeight = windowNode["ImageHeight"].as<unsigned>();

    const auto fractalNode = config["Fractal"];
    const int maxIter = fractalNode["MaxIterations"].as<int>();
    double zoom = fractalNode["InitialZoom"].as<double>();
    double offsetX = fractalNode["InitialOffsetX"].as<double>();
    double offsetY = fractalNode["InitialOffsetY"].as<double>();

    sf::RenderWindow window(sf::VideoMode({width, height}), "Mandelbrot Combined Renderer");
    window.setFramerateLimit(60);

    sf::Image image({width, height});
    sf::Image prevImage;
    std::vector<double> prevSmoothFlat(width * height, 0.0);

    sf::Texture texture(image);
    sf::Sprite sprite(texture);
    bool needsUpdate = true;
    bool dragging = false;
    sf::Vector2i lastMousePos;
    int totalDragDx = 0, totalDragDy = 0;

    while (window.isOpen()) {
        while (auto event = window.pollEvent()) {
            if (event->is<sf::Event::Closed>()) {
                window.close();
            }
            if (auto* e = event->getIf<sf::Event::MouseWheelScrolled>()) {
                double oldZoom = zoom;
                if (e->delta > 0) zoom *= 1.2;
                else if (e->delta < 0) zoom /= 1.2;
                sf::Vector2i mouse = sf::Mouse::getPosition(window);
                double mx = (mouse.x - width / 2.0) * (4.0 / width) / oldZoom + offsetX;
                double my = (mouse.y - height / 2.0) * (4.0 / height) / oldZoom + offsetY;
                offsetX = mx - (mouse.x - width / 2.0) * (4.0 / width) / zoom;
                offsetY = my - (mouse.y - height / 2.0) * (4.0 / height) / zoom;
                needsUpdate = true;
            }
            if (auto* e = event->getIf<sf::Event::MouseButtonPressed>()) {
                if (e->button == sf::Mouse::Button::Left) {
                dragging = true;
                    lastMousePos = sf::Mouse::getPosition(window);
                }
            }
            if (auto* e = event->getIf<sf::Event::MouseButtonReleased>()) {
                if (e->button == sf::Mouse::Button::Left) {
                dragging = false;
            }
            }
            if (event->is<sf::Event::MouseMoved>() && dragging) {
                sf::Vector2i mouse = sf::Mouse::getPosition(window);
                int dx = mouse.x - lastMousePos.x;
                int dy = mouse.y - lastMousePos.y;
                offsetX -= dx * (4.0 / double(width)) / zoom;
                offsetY -= dy * (4.0 / double(height)) / zoom;
                lastMousePos = mouse;
                totalDragDx += dx;
                totalDragDy += dy;
                needsUpdate = true;
            }

            if (auto* e = event->getIf<sf::Event::KeyPressed>()) {
                int panAmount = 20;
                int pdx = 0, pdy = 0;
                if (e->code == sf::Keyboard::Key::Left) {
                    offsetX -= panAmount * (4.0 / double(width)) / zoom;
                    pdx = -panAmount;
                    needsUpdate = true;
                } else if (e->code == sf::Keyboard::Key::Right) {
                    offsetX += panAmount * (4.0 / double(width)) / zoom;
                    pdx = panAmount;
                    needsUpdate = true;
                } else if (e->code == sf::Keyboard::Key::Up) {
                    offsetY -= panAmount * (4.0 / double(height)) / zoom;
                    pdy = -panAmount;
                    needsUpdate = true;
                } else if (e->code == sf::Keyboard::Key::Down) {
                    offsetY += panAmount * (4.0 / double(height)) / zoom;
                    pdy = panAmount;
                    needsUpdate = true;
                } else if (e->code == sf::Keyboard::Key::J || e->code == sf::Keyboard::Key::K) {
                    double oldZoom = zoom;
                    double factor = 1.5;
                    if (e->code == sf::Keyboard::Key::J) zoom /= factor;
                    else zoom *= factor;
                    sf::Vector2i mouse = sf::Mouse::getPosition(window);
                    double mx = (mouse.x - width / 2.0) * (4.0 / double(width)) / oldZoom + offsetX;
                    double my = (mouse.y - height / 2.0) * (4.0 / double(height)) / oldZoom + offsetY;
                    offsetX = mx - (mouse.x - width / 2.0) * (4.0 / double(width)) / zoom;
                    offsetY = my - (mouse.y - height / 2.0) * (4.0 / double(height)) / zoom;
                    needsUpdate = true;
                } else if (e->code == sf::Keyboard::Key::S) {
                    saveCurrentViewAsImage(imageWidth, imageHeight, maxIter, zoom, offsetX, offsetY);
                }
                totalDragDx += pdx;
                totalDragDy += pdy;
            }
        }

        if (needsUpdate) {
            std::cout << "Updating fractal...\n";
            prevImage = image;
            timeFunction([&]() {
                if ((totalDragDx != 0 || totalDragDy != 0) && (dragging || totalDragDx != 0 || totalDragDy != 0)) {
                    renderCombinedFast(image, maxIter, zoom, offsetX, offsetY, prevSmoothFlat, &prevImage, -totalDragDx, -totalDragDy);
                } else {
                    renderCombinedFast(image, maxIter, zoom, offsetX, offsetY, prevSmoothFlat, nullptr, 0, 0);
                }
            });
            if (texture.loadFromImage(image)) {
                sprite.setTexture(texture);
            }
            needsUpdate = false;
            totalDragDx = 0; totalDragDy = 0;
        }

        window.clear();
        window.draw(sprite);
        window.display();
    }
    return 0;
}
