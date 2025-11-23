#pragma once

#include <SFML/Graphics.hpp>

#include <FractalBase.hpp>
#include <Viewport.hpp>

class EventHandler {
public:
    EventHandler(sf::RenderWindow &window,
                 sf::Sprite &sprite,
                 std::unique_ptr<FractalBase> fractal,
                 sf::Image *image,
                 sf::Texture &texture,
                 Viewport *viewport);
    void run();

private:
    void handleQuitEvent(sf::RenderWindow &window);
    void handleArrowKeyEvent(sf::Event::KeyPressed const &e, Viewport *viewport);
    void handleMouseWheelEvent(const sf::Event::MouseWheelScrolled &e);
    void handleMouseButtonPressed(const sf::Event::MouseButtonPressed &e);
    void handleMouseButtonReleased(const sf::Event::MouseButtonReleased &e);
    void handleMouseMoved();
    void handleZoomWithKeyboard(const sf::Event::KeyPressed &e);
    void handleSaveImageEvent();

    void applyZoomAtMouse(double zoomFactor);
    void updateViewportAndRedraw();

    sf::RenderWindow &window;
    sf::Sprite &sprite;
    std::unique_ptr<FractalBase> fractal;
    sf::Image *image;
    sf::Texture &texture;
    Viewport *viewport;

    bool needsRedraw = false;

    bool dragging = false;
    sf::Vector2i lastMousePos;
    double zoom = 1.0;
};