#include <SFML/Graphics.hpp>
#include <complex>
#include <functional>

using FractalFun = std::function<int(std::complex<double>, int)>;

int mandelbrot(std::complex<double> c, int maxIter) {
    std::complex<double> z = 0;
    int n = 0;
    while (std::abs(z) <= 2 && n < maxIter) {
        z = z * z + c;
        ++n;
    }
    return n;
}

void renderFractal(sf::Image& image, FractalFun fractalFunc,
                   int maxIter, double zoom, double offsetX, double offsetY) {
    auto size = image.getSize();
    unsigned width = size.x;
    unsigned height = size.y;

    for (unsigned x = 0; x < width; ++x) {
        for (unsigned y = 0; y < height; ++y) {
            double real = (x - width / 2.0) * (4.0 / width) / zoom + offsetX;
            double imag = (y - height / 2.0) * (4.0 / height) / zoom + offsetY;
            std::complex<double> c(real, imag);
            int n = fractalFunc(c, maxIter);

            sf::Color color =
                (n == maxIter)
                    ? sf::Color::Black
                    : sf::Color(255 - (n * 255 / maxIter), 0, n * 255 / maxIter);

            image.setPixel({x, y}, color);  // ✅ correct SFML 3 call
        }
    }
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
    renderFractal(image, mandelbrot, maxIter, zoom, offsetX, offsetY);

    sf::Texture texture(image);
    sf::Sprite sprite(texture);

    bool needsUpdate = false;
    bool dragging = false;
    sf::Vector2i lastMousePos;

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
                offsetX -= dx * (4.0 / width) / zoom;
                offsetY -= dy * (4.0 / height) / zoom;
                lastMousePos = mouse;
                needsUpdate = true;
            }
        }

        if (needsUpdate) {
            renderFractal(image, mandelbrot, maxIter, zoom, offsetX, offsetY);
            if (texture.loadFromImage(image)) {
                sprite.setTexture(texture);
            }
            needsUpdate = false;
        }

        window.clear();
        window.draw(sprite);
        window.display();
    }

    return 0;
}
