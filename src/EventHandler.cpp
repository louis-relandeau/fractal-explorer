#include "EventHandler.hpp"

#include "Timer.hpp"

EventHandler::EventHandler(sf::RenderWindow &window,
                           sf::Sprite &sprite,
                           FractalSampler &sampler,
                           sf::Image &image,
                           sf::Texture &texture,
                           Viewport &viewport)
    : window(window),
      image(image),
      texture(texture),
      sprite(sprite),
      sampler(sampler),
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

void EventHandler::handleArrowKeyEvent(sf::Event::KeyPressed const &e, Viewport &viewport) {
    double panFactor = 0.1; // % of the viewport size
    if (e.code == sf::Keyboard::Key::Left) {
        viewport.centerX += panFactor * viewport.width;
    } else if (e.code == sf::Keyboard::Key::Right) {
        viewport.centerX -= panFactor * viewport.width;
    } else if (e.code == sf::Keyboard::Key::Up) {
        viewport.centerY -= panFactor * viewport.height;
    } else if (e.code == sf::Keyboard::Key::Down) {
        viewport.centerY += panFactor * viewport.height;
    }
    needsRedraw = true;
}

void EventHandler::handleMouseWheelEvent(const sf::Event::MouseWheelScrolled &e) {
    double oldZoom = zoom;
    if (e.delta > 0)
        zoom *= 1.2;
    else if (e.delta < 0)
        zoom /= 1.2;
    sf::Vector2i mouse = sf::Mouse::getPosition(window);
    auto winSize = window.getSize();
    double mx = (mouse.x - int(winSize.x) / 2.0) * (viewport.width / double(winSize.x)) / oldZoom +
                viewport.centerX;
    double my = (mouse.y - int(winSize.y) / 2.0) * (viewport.height / double(winSize.y)) / oldZoom +
                viewport.centerY;
    viewport.width = viewport.width / oldZoom * zoom;
    viewport.height = viewport.height / oldZoom * zoom;
    viewport.centerX = mx - (mouse.x - int(winSize.x) / 2.0) *
                                  (viewport.width / double(winSize.x)) / zoom;
    viewport.centerY = my - (mouse.y - int(winSize.y) / 2.0) *
                                  (viewport.height / double(winSize.y)) / zoom;
    needsRedraw = true;
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
    viewport.centerX -= dx * (viewport.width / double(winSize.x)) / zoom;
    viewport.centerY += dy * (viewport.height / double(winSize.y)) / zoom;
    lastMousePos = mouse;
    needsRedraw = true;
}

void EventHandler::updateViewportAndRedraw() {
    timeFunction([&] { sampler.compute(image, viewport); });
    texture.update(image);
    sprite.setTexture(texture);
}