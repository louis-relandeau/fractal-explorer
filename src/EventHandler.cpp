#include "EventHandler.hpp"

#include <map>
#include <iostream>

#include "Timer.hpp"

EventHandler::EventHandler(sf::RenderWindow &window,
                           sf::Sprite &sprite,
                           std::unique_ptr<FractalBase> fractal,
                           sf::Image *image,
                           sf::Texture &texture,
                           Viewport *viewport)
    : window(window),
      image(image),
      texture(texture),
      sprite(sprite),
      fractal(std::move(fractal)),
      viewport(viewport) {}

void EventHandler::run() {
    while (window.isOpen()) {
        while (auto event = window.pollEvent()) {
            if (event->is<sf::Event::Closed>()) {
                handleQuitEvent(window);
            }
            if (auto *e = event->getIf<sf::Event::KeyPressed>()) {
                if (e->code == sf::Keyboard::Key::Left || e->code == sf::Keyboard::Key::Right ||
                    e->code == sf::Keyboard::Key::Up || e->code == sf::Keyboard::Key::Down) {
                    handleArrowKeyEvent(*e, viewport);
                } else if (e->code == sf::Keyboard::Key::Escape ||
                           e->code == sf::Keyboard::Key::Q) {
                    handleQuitEvent(window);
                } else if (e->code == sf::Keyboard::Key::J || e->code == sf::Keyboard::Key::K) {
                    handleZoomWithKeyboard(*e);
                } else if (e->code == sf::Keyboard::Key::S) {
                    handleSaveImageEvent();
                }
            }
            if (auto *e = event->getIf<sf::Event::MouseWheelScrolled>()) {
                handleMouseWheelEvent(*e);
            }
            if (auto *e = event->getIf<sf::Event::MouseButtonPressed>()) {
                handleMouseButtonPressed(*e);
            }
            if (auto *e = event->getIf<sf::Event::MouseButtonReleased>()) {
                handleMouseButtonReleased(*e);
            }
            if (event->is<sf::Event::MouseMoved>() && dragging) {
                handleMouseMoved();
            }
        }
        if (needsRedraw) {
            updateViewportAndRedraw();
            needsRedraw = false;
        }
        window.clear();
        window.draw(sprite);
        window.display();
    }
}

void EventHandler::handleQuitEvent(sf::RenderWindow &window) { window.close(); }

void EventHandler::handleArrowKeyEvent(sf::Event::KeyPressed const &e, Viewport *viewport) {
    double panFactor = 0.1; // % of the viewport size
    if (e.code == sf::Keyboard::Key::Left) {
        viewport->centerX += panFactor * viewport->width;
    } else if (e.code == sf::Keyboard::Key::Right) {
        viewport->centerX -= panFactor * viewport->width;
    } else if (e.code == sf::Keyboard::Key::Up) {
        viewport->centerY -= panFactor * viewport->height;
    } else if (e.code == sf::Keyboard::Key::Down) {
        viewport->centerY += panFactor * viewport->height;
    }
    needsRedraw = true;
}

void EventHandler::handleMouseWheelEvent(const sf::Event::MouseWheelScrolled &e) {
    double zoomFactor;
    if (e.delta > 0)
        zoomFactor = 1.2;
    else if (e.delta < 0)
        zoomFactor = 1.0 / 1.2;
    else
        return;

    applyZoomAtMouse(zoomFactor);
}

void EventHandler::handleMouseButtonPressed(const sf::Event::MouseButtonPressed &e) {
    if (e.button == sf::Mouse::Button::Left) {
        dragging = true;
        lastMousePos = sf::Mouse::getPosition(window);
    }
}

void EventHandler::handleMouseButtonReleased(const sf::Event::MouseButtonReleased &e) {
    if (e.button == sf::Mouse::Button::Left) {
        dragging = false;
    }
}

void EventHandler::handleMouseMoved() {
    sf::Vector2i mouse = sf::Mouse::getPosition(window);
    auto winSize = window.getSize();
    int dx = mouse.x - lastMousePos.x;
    int dy = mouse.y - lastMousePos.y;
    viewport->centerX -= dx * (viewport->width / double(winSize.x));
    viewport->centerY += dy * (viewport->height / double(winSize.y));
    lastMousePos = mouse;
    needsRedraw = true;
}

void EventHandler::handleZoomWithKeyboard(const sf::Event::KeyPressed &e) {
    double zoomFactor;
    if (e.code == sf::Keyboard::Key::J)
        zoomFactor = 1.2;
    else if (e.code == sf::Keyboard::Key::K)
        zoomFactor = 1.0 / 1.2;
    else
        return;

    applyZoomAtMouse(zoomFactor);
}

void EventHandler::handleSaveImageEvent() {
    static const int ppi = 300;
    static const std::map<std::string, std::pair<int, int>> formats = {
        {"a4", {static_cast<int>(11.69 * ppi), static_cast<int>(8.27 * ppi)}},
        {"a3", {static_cast<int>(16.54 * ppi), static_cast<int>(11.69 * ppi)}},
        {"a2", {static_cast<int>(23.39 * ppi), static_cast<int>(16.54 * ppi)}},
        {"a1", {static_cast<int>(33.11 * ppi), static_cast<int>(23.39 * ppi)}},
        {"a0", {static_cast<int>(46.81 * ppi), static_cast<int>(33.11 * ppi)}},
        {"2a0", {static_cast<int>(66.22 * ppi), static_cast<int>(46.81 * ppi)}}
    };

    std::string formatChoice;
    std::cout << "Enter format to save, options: ";
    for (const auto& [fmt, _] : formats) {
        std::cout << fmt << " ";
    }
    std::cin >> formatChoice;
    if (formats.find(formatChoice) == formats.end()) {
        std::cout << "Unknown format: " << formatChoice << std::endl;
        return;
    }

    auto [width, height] = formats.at(formatChoice);
    std::cout << "Rendering image of size " << width << "x" << height << "..." << std::endl;
    sf::Image saveImage({static_cast<unsigned int>(width), static_cast<unsigned int>(height)});
    Viewport saveVP = *viewport;

    double aspectTarget = double(width) / double(height);
    saveVP.height = saveVP.width / aspectTarget;

    FractalBase* fractalPtr = fractal.get();
    
    fractalPtr->backupAndReplacePointers(&saveImage, &saveVP);
    timeFunction([&] { fractalPtr->compute(); });
    fractalPtr->restoreBackedUpPointers();

    // Get current time for timestamp
    auto t = std::time(nullptr);
    std::tm tm;
    localtime_r(&t, &tm);
    char timestamp[32];
    std::strftime(timestamp, sizeof(timestamp), "%Y%m%d_%H%M%S", &tm);
    std::string filename = "images/fractal_" + formatChoice + "_" + timestamp + ".png";
    if (saveImage.saveToFile(filename)) {
        std::cout << "Image saved to " << filename << std::endl;
    } else {
        std::cout << "Failed to save image to " << filename << std::endl;
    }
}

void EventHandler::applyZoomAtMouse(double zoomFactor) {
    sf::Vector2i mouse = sf::Mouse::getPosition(window);
    auto winSize = window.getSize();

    double mx = (mouse.x - winSize.x / 2.0) * (viewport->width / winSize.x) + viewport->centerX;
    double my = -(mouse.y - winSize.y / 2.0) * (viewport->height / winSize.y) + viewport->centerY;

    viewport->width /= zoomFactor;
    viewport->height /= zoomFactor;

    viewport->centerX = mx - (mouse.x - winSize.x / 2.0) * (viewport->width / winSize.x);
    viewport->centerY = my + (mouse.y - winSize.y / 2.0) * (viewport->height / winSize.y);

    needsRedraw = true;

    std::cout << "Zoom level: " << (4.0 / viewport->width) << "x" << std::endl;
}

void EventHandler::updateViewportAndRedraw() {
    timeFunction([&] { fractal->compute(); });
    texture.update(*image);
    sprite.setTexture(texture);
}