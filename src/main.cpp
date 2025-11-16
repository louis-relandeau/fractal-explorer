#include <complex>
#include <functional>
#include <cstdint>
#include <iostream>
#include <chrono>
#include <vector>
#include <cmath>
#include <limits>

#include <SFML/Graphics.hpp>

using FractalFun = std::function<std::pair<double, int>(std::complex<double>, int, double)>;

std::pair<double,int> mandelbrot(std::complex<double> c, int maxIter, double pixelSize) {
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

            // early exit
            if (dist > pixelSize * 4.0) {
                break;
            }

            break;
        }
    }

    int pixelDist = int(dist / pixelSize);
    if (pixelDist < 1) pixelDist = 1;
    if (pixelDist > 8) pixelDist = 8;

    if (n == maxIter) return {double(n), pixelDist};

    double smooth = n + 1.0 - std::log(std::log(std::abs(z))) / std::log(2.0);
    return {smooth, pixelDist};
}

sf::Color hsvToRgb(double h, double s, double v) {
    double c = v * s;
    double x = c * (1 - std::fabs(fmod(h / 60.0, 2) - 1));
    double m = v - c;
    double r, g, b;
    if (h < 60)      { r = c; g = x; b = 0; }
    else if (h < 120) { r = x; g = c; b = 0; }
    else if (h < 180) { r = 0; g = c; b = x; }
    else if (h < 240) { r = 0; g = x; b = c; }
    else if (h < 300) { r = x; g = 0; b = c; }
    else              { r = c; g = 0; b = x; }
    uint8_t R = static_cast<uint8_t>(255 * (r + m));
    uint8_t G = static_cast<uint8_t>(255 * (g + m));
    uint8_t B = static_cast<uint8_t>(255 * (b + m));
    return sf::Color(R, G, B);
}

sf::Color mapColorLogScale(double n) {
    if (n >= 1.0) return sf::Color::Black;
    double hue = 360.0 * n;
    double sat = 0.8;
    double val = 1.0 - n * 0.8;
    return hsvToRgb(hue, sat, val);
}

void renderFractal(
    sf::Image& image,
    FractalFun fractalFunc,
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

    std::vector<std::vector<double>> smoothVals(width, std::vector<double>(height));

    // nothing computed yet
    for (unsigned x = 0; x < width; ++x)
        for (unsigned y = 0; y < height; ++y)
            smoothVals[x][y] = std::numeric_limits<double>::quiet_NaN();
    double minVal = std::numeric_limits<double>::max();
    double maxVal = std::numeric_limits<double>::lowest();

    double pixelSize = (4.0 / width) / zoom;

    bool doReuse = (prevImage && (dx != 0 || dy != 0));

    for (unsigned y = 0; y < height; ++y) {
        for (unsigned x = 0; x < width; ) {

            if (doReuse) {
                int px = int(x) + dx;
                int py = int(y) + dy;

                if (px >= 0 && px < int(width) && py >= 0 && py < int(height)) {
                    smoothVals[x][y] = prevSmoothVals[px][py];
                    image.setPixel({x,y}, prevImage->getPixel({(unsigned)px,(unsigned)py}));

                    double v = smoothVals[x][y];
                    if (v < minVal) minVal = v;
                    if (v > maxVal) maxVal = v;

                    ++x;
                    continue;
                }
            }

            double real = (double(x) - width  / 2.0) * (4.0 / width)  / zoom + offsetX;
            double imag = (double(y) - height / 2.0) * (4.0 / height) / zoom + offsetY;

            auto [smooth, skip] = fractalFunc({real, imag}, maxIter, pixelSize);

            for (int n=0; n<skip && (x+n) < int(width); ++n) {
                smoothVals[x+n][y] = smooth;
            }
            if (smooth < minVal) minVal = smooth;
            if (smooth > maxVal) maxVal = smooth;

            x += skip;
        }
    }

    prevSmoothVals = smoothVals;

    // coloring
    for (unsigned y = 0; y < height; ++y) {
        for (unsigned x = 0; x < width; ++x) {
            double s = smoothVals[x][y];

            double norm = (maxVal > minVal) ? (s - minVal) / (maxVal - minVal) : 0.0;

            sf::Color color =
                (s >= maxIter) ? sf::Color::Black : mapColorLogScale(norm);

            image.setPixel({x,y}, color);
        }
    }
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

    sf::RenderWindow window(sf::VideoMode({width, height}), "Mandelbrot Set");

    sf::Image image({width, height});
    sf::Image prevImage;
    std::vector<std::vector<double>> prevSmoothVals(width, std::vector<double>(height, 0.0));
    timeFunction([&]() {
        renderFractal(image, mandelbrot, maxIter, zoom, offsetX, offsetY, prevSmoothVals);
    });

    sf::Texture texture(image);
    sf::Sprite sprite(texture);

    bool needsUpdate = false;
    bool dragging = false;
    bool tempDragging = false;
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
            if (auto* e = event->getIf<sf::Event::KeyPressed>()) {
                if (e->code == sf::Keyboard::Key::J || e->code == sf::Keyboard::Key::K) {
                    double oldZoom = zoom;
                    double factor = 1.5;
                    if (e->code == sf::Keyboard::Key::J) zoom /= factor;
                    else if (e->code == sf::Keyboard::Key::K) zoom *= factor;
                    sf::Vector2i mouse = sf::Mouse::getPosition(window);
                    double mx = (mouse.x - width / 2.0) * (4.0 / width) / oldZoom + offsetX;
                    double my = (mouse.y - height / 2.0) * (4.0 / height) / oldZoom + offsetY;
                    offsetX = mx - (mouse.x - width / 2.0) * (4.0 / width) / zoom;
                    offsetY = my - (mouse.y - height / 2.0) * (4.0 / height) / zoom;
                    needsUpdate = true;
                }
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
                offsetX -= dx * (4.0 / width) / zoom;
                offsetY -= dy * (4.0 / height) / zoom;
                lastMousePos = mouse;
                totalDragDx += dx;
                totalDragDy += dy;
                needsUpdate = true;
            }
            // arrows for dragging
            if (auto* e = event->getIf<sf::Event::KeyPressed>()) {
                int panAmount = 20; // pixels
                int dx = 0, dy = 0;
                if (e->code == sf::Keyboard::Key::Left) {
                    offsetX -= panAmount * (4.0 / width) / zoom;
                    dx = -panAmount;
                    needsUpdate = true;
                } else if (e->code == sf::Keyboard::Key::Right) {
                    offsetX += panAmount * (4.0 / width) / zoom;
                    dx = panAmount;
                    needsUpdate = true;
                } else if (e->code == sf::Keyboard::Key::Up) {
                    offsetY -= panAmount * (4.0 / height) / zoom;
                    dy = -panAmount;
                    needsUpdate = true;
                } else if (e->code == sf::Keyboard::Key::Down) {
                    offsetY += panAmount * (4.0 / height) / zoom;
                    dy = panAmount;
                    needsUpdate = true;
                } else {
                    continue;
                }
                totalDragDx += dx;
                totalDragDy += dy;
                tempDragging = true;
            }
        }

        if (needsUpdate) {
            std::cout << "Updating fractal...\n";
            prevImage = image;
            timeFunction([&]() {
                if ((totalDragDx != 0 || totalDragDy != 0) && (dragging || tempDragging)) {
                    tempDragging = false;
                    renderFractal(image, mandelbrot, maxIter, zoom, offsetX, offsetY, prevSmoothVals, &prevImage, -totalDragDx, -totalDragDy);
                } else {
                    renderFractal(image, mandelbrot, maxIter, zoom, offsetX, offsetY, prevSmoothVals);
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
