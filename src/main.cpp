#include <complex>
#include <functional>
#include <cstdint>
#include <iostream>
#include <chrono>
#include <vector>
#include <cmath>
#include <limits>

#include <SFML/Graphics.hpp>

using namespace std;

struct DEResult {
    double smooth;
    double dist;
    int iter;
};

int const MAX_HORIZONTAL_SKIP = 128;
double const EARLY_EXIT_MULTIPLIER = 4.0;
double const SAFETY_FACTOR = 0.9;
double const DETAIL_MULTIPLIER = 1.5;

DEResult mandelbrotDE(std::complex<double> c, int maxIter, double pixelSize) {
    std::complex<double> z = 0.0;
    std::complex<double> dz = 0.0;
    int n = 0;
    double dist = pixelSize;

    while (n < maxIter) {
        dz = 2.0 * z * dz + 1.0;
        z  = z*z + c;
        ++n;

        double r = std::abs(z);
        if (r > 2.0) {
            double dd = std::abs(dz);
            if (dd < 1e-300) dd = 1e-300;
            dist = 2.0 * r * std::log(r) / dd;
            if (dist > pixelSize * EARLY_EXIT_MULTIPLIER) {
                break;
            }
            break;
        }
    }

    double smooth = double(n);
    if (n < maxIter) {
        double absz = std::abs(z);
        if (absz < 1e-300) absz = 1e-300;
        smooth = n + 1.0 - std::log(std::log(absz)) / std::log(2.0);
    }

    return {smooth, dist, n};
}

sf::Color hsvToRgb(double h, double s, double v) {
    double c = v * s;
    double x = c * (1 - std::fabs(fmod(h / 60.0, 2.0) - 1.0));
    double m = v - c;
    double r, g, b;
    if (h < 60.0)      { r = c; g = x; b = 0; }
    else if (h < 120.0) { r = x; g = c; b = 0; }
    else if (h < 180.0) { r = 0; g = c; b = x; }
    else if (h < 240.0) { r = 0; g = x; b = c; }
    else if (h < 300.0) { r = x; g = 0; b = c; }
    else                { r = c; g = 0; b = x; }
    uint8_t R = static_cast<uint8_t>(255.0 * (r + m));
    uint8_t G = static_cast<uint8_t>(255.0 * (g + m));
    uint8_t B = static_cast<uint8_t>(255.0 * (b + m));
    return sf::Color(R, G, B);
}

sf::Color mapColorLogScale(double n) {
    if (n >= 1.0) return sf::Color::Black;
    double hue = 360.0 * n;
    double sat = 0.8;
    double val = 1.0 - n * 0.8;
    return hsvToRgb(hue, sat, val);
}

void renderFractalCombined(
    sf::Image& image,
    int maxIter,
    double zoom,
    double offsetX,
    double offsetY,
    std::vector<std::vector<double>>& prevSmoothVals,
    const sf::Image* prevImage = nullptr,
    int dx = 0,
    int dy = 0
) {
    auto size = image.getSize();
    unsigned width  = size.x;
    unsigned height = size.y;

    std::vector<std::vector<double>> smoothVals(width, std::vector<double>(height, std::numeric_limits<double>::quiet_NaN()));
    std::vector<std::vector<char>> needsDetail(width, std::vector<char>(height, 1));
    double minVal = std::numeric_limits<double>::max();
    double maxVal = std::numeric_limits<double>::lowest();

    double pixelSizeX = (4.0 / double(width)) / zoom;
    double pixelSizeY = (4.0 / double(height)) / zoom;
    double pixelSize = pixelSizeX;

    bool doReuse = (prevImage != nullptr);

    if (doReuse && (dx != 0 || dy != 0)) {
        for (unsigned yy = 0; yy < height; ++yy) {
            for (unsigned xx = 0; xx < width; ++xx) {
                int px = int(xx) + dx;
                int py = int(yy) + dy;
                if (px >= 0 && px < int(width) && py >= 0 && py < int(height)) {
                    smoothVals[xx][yy] = prevSmoothVals[px][py];
                    needsDetail[xx][yy] = 0;
                    image.setPixel({xx, yy}, prevImage->getPixel({(unsigned)px, (unsigned)py}));
                    double v = smoothVals[xx][yy];
                    if (!std::isnan(v)) {
                        if (v < minVal) minVal = v;
                        if (v > maxVal) maxVal = v;
                    }
                }
            }
        }
    }

    for (unsigned y = 0; y < height; ++y) {
        unsigned x = 0;
        while (x < width) {
            if (!needsDetail[x][y]) { ++x; continue; }

            double real = (double(x) - double(width)  / 2.0) * (4.0 / double(width))  / zoom + offsetX;
            double imag = (double(y) - double(height) / 2.0) * (4.0 / double(height)) / zoom + offsetY;
            std::complex<double> c(real, imag);

            DEResult de = mandelbrotDE(c, maxIter, pixelSize);

            if (!std::isnan(de.smooth)) {
                if (de.smooth < minVal) minVal = de.smooth;
                if (de.smooth > maxVal) maxVal = de.smooth;
            }

            if (de.dist <= pixelSize * DETAIL_MULTIPLIER) {
                needsDetail[x][y] = 1;
                smoothVals[x][y] = std::numeric_limits<double>::quiet_NaN();
                ++x;
                continue;
            }

            double stepPixelsD = (de.dist / pixelSize) * SAFETY_FACTOR;
            int skip = int(std::floor(stepPixelsD));
            if (skip < 1) skip = 1;
            if (skip > MAX_HORIZONTAL_SKIP) skip = MAX_HORIZONTAL_SKIP;

            for (int k = 0; k < skip && (x + k) < int(width); ++k) {
                smoothVals[x + k][y] = de.smooth;
                needsDetail[x + k][y] = 0;
                double norm = (maxVal > minVal) ? (de.smooth - minVal) / (maxVal - minVal) : 0.0;
                sf::Color color = (de.iter >= maxIter) ? sf::Color::Black : mapColorLogScale(norm);
                image.setPixel({unsigned(x + k), y}, color);
            }

            x += skip;
        }
    }

    for (unsigned y = 0; y < height; ++y) {
        for (unsigned x = 0; x < width; ++x) {
            if (!needsDetail[x][y]) continue;

            double real = (double(x) - double(width)  / 2.0) * (4.0 / double(width))  / zoom + offsetX;
            double imag = (double(y) - double(height) / 2.0) * (4.0 / double(height)) / zoom + offsetY;
            std::complex<double> c(real, imag);

            DEResult de = mandelbrotDE(c, maxIter, pixelSize);

            smoothVals[x][y] = de.smooth;
            if (de.smooth < minVal) minVal = de.smooth;
            if (de.smooth > maxVal) maxVal = de.smooth;
            double norm = (maxVal > minVal) ? (de.smooth - minVal) / (maxVal - minVal) : 0.0;
            sf::Color color = (de.iter >= maxIter) ? sf::Color::Black : mapColorLogScale(norm);
            image.setPixel({x, y}, color);
        }
    }

    for (unsigned y = 0; y < height; ++y) {
        for (unsigned x = 0; x < width; ++x) {
            double s = smoothVals[x][y];
            if (std::isnan(s)) continue;
            double norm = (maxVal > minVal) ? (s - minVal) / (maxVal - minVal) : 0.0;
            sf::Color color = (s >= maxIter) ? sf::Color::Black : mapColorLogScale(norm);
            image.setPixel({x, y}, color);
        }
    }

    prevSmoothVals = std::move(smoothVals);
}

void timeFunction(std::function<void()> func) {
    auto start = std::chrono::high_resolution_clock::now();
    func();
    auto end = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double> elapsed = end - start;
    std::cout << "Elapsed time: " << elapsed.count() << " seconds\n";
}

int main() {
    const unsigned width = 800;
    const unsigned height = 600;
    const int maxIter = 1000;
    double zoom = 1.0;
    double offsetX = -0.5;
    double offsetY = 0.0;

    sf::RenderWindow window(sf::VideoMode({width, height}), "Mandelbrot Combined Renderer");
    window.setFramerateLimit(60);

    sf::Image image({width, height});
    sf::Image prevImage;
    std::vector<std::vector<double>> prevSmoothVals(width, std::vector<double>(height, 0.0));

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
                    renderFractalCombined(image, maxIter, zoom, offsetX, offsetY, prevSmoothVals, &prevImage, -totalDragDx, -totalDragDy);
                } else {
                    renderFractalCombined(image, maxIter, zoom, offsetX, offsetY, prevSmoothVals, nullptr, 0, 0);
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
