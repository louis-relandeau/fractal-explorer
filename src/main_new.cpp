#include <cstdint>
#include <filesystem>
#include <string>

#include <SFML/Graphics.hpp>

#include <ConfigLoader.hpp>
#include <EventHandler.hpp>
#include <Mandelbrot.hpp>
#include <Timer.hpp>
#include <Viewport.hpp>

int main() {
    std::string rootPath = std::filesystem::current_path().string();
    ConfigLoader config(rootPath + "/config.yaml");

    std::string windowTitle = "Fractal Explorer - " + config.fractalParams.name;
    sf::RenderWindow window(sf::VideoMode({config.windowParams.width, config.windowParams.height}),
                            windowTitle);
    window.setFramerateLimit(60);

    double initialWidth = 4.0;
    double initialHeight = initialWidth * config.windowParams.height / config.windowParams.width;
    Viewport viewport{0, 0, initialWidth, initialHeight};

    sf::Image *image = new sf::Image({config.windowParams.width, config.windowParams.height});
    sf::Texture texture(image->getSize());
    sf::Sprite sprite(texture);

    std::unique_ptr<FractalBase> fractal;
    if (config.fractalParams.name == "Mandelbrot") {
        fractal = std::make_unique<Mandelbrot>(image, &viewport);
    } else {
        throw std::runtime_error("Unknown fractal: " + config.fractalParams.name);
    }

    timeFunction([&] { fractal->compute(); });
    texture.update(*image);
    sprite.setTexture(texture);

    EventHandler eventHandler(window, sprite, std::move(fractal), image, texture, &viewport);
    eventHandler.run();

    return 0;
}