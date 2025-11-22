#include <cstdint>
#include <filesystem>
#include <string>

#include <SFML/Graphics.hpp>

#include <ConfigLoader.hpp>
#include <FractalSampler.hpp>
#include <Timer.hpp>
#include <Viewport.hpp>
#include <EventHandler.hpp>

int main() {
    std::string rootPath = std::filesystem::current_path().string();
    ConfigLoader config(rootPath + "/config.yaml");

    std::string windowTitle = "Fractal Explorer - " + config.fractalParams.name;
    sf::RenderWindow window(sf::VideoMode({config.windowParams.width, config.windowParams.height}),
                            windowTitle);
    window.setFramerateLimit(60);

    FractalSampler sampler(config.fractalParams.name);
    double initialWidth = 4.0;
    double initialHeight = initialWidth * config.windowParams.height / config.windowParams.width;
    Viewport viewport{0, 0, initialWidth, initialHeight};
    
    sf::Image image({config.windowParams.width, config.windowParams.height});
    sf::Texture texture(image);
    sf::Sprite sprite(texture);
    timeFunction([&] { sampler.compute(image, viewport); });
    texture.update(image);
    sprite.setTexture(texture);

    EventHandler eventHandler(window, sprite, sampler, image, texture, viewport);
    eventHandler.run();

    return 0;
}