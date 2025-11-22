#pragma once

#include <SFML/Graphics.hpp>

#include <FractalSampler.hpp>
#include <Viewport.hpp>

class EventHandler {
public:
    EventHandler(sf::RenderWindow &window,
                 sf::Sprite &sprite,
                 FractalSampler &sampler,
                 sf::Image &image,
                 sf::Texture &texture,
                 Viewport &viewport);
    void run();

private:
    void handleQuitEvent(sf::RenderWindow &window);
    void handleArrowKeyEvent(sf::Event::KeyPressed const &e, Viewport &viewport);
    void handleMouseWheelEvent(const sf::Event::MouseWheelScrolled &e);
    void handleMouseButtonPressed(const sf::Event::MouseButtonPressed &e);
    void handleMouseButtonReleased(const sf::Event::MouseButtonReleased &e);
    void handleMouseMoved();

    void updateViewportAndRedraw();
    
    sf::RenderWindow &window;
    sf::Sprite &sprite;
    FractalSampler &sampler;
    sf::Image &image;
    sf::Texture &texture;
    Viewport &viewport;

    bool needsRedraw = false;
    
    bool dragging = false;
    sf::Vector2i lastMousePos;
    double zoom = 1.0;
};