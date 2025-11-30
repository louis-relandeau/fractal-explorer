#include <cstdint>
#include <filesystem>
#include <string>

#include <SFML/Graphics.hpp>

#include <ConfigLoader.hpp>
#include <EventHandler.hpp>
#include <Mandelbrot.hpp>
#include <Timer.hpp>
#include <Viewport.hpp>

#include <unistd.h>
#include <limits.h>
#include <filesystem>
#include <string>

std::string getExecutableDir() {
    char result[PATH_MAX];
    ssize_t count = readlink("/proc/self/exe", result, PATH_MAX);
    std::string exePath = std::string(result, (count > 0) ? count : 0);
    return std::filesystem::path(exePath).parent_path().string();
}

int main() {
    std::string exeDir = getExecutableDir();
    std::string rootPath = std::filesystem::path(exeDir).parent_path().string();
    std::string configPath = rootPath + "/config.yaml";
    ConfigLoader config(configPath);

    std::string windowTitle = "Fractal Explorer - " + config.fractalParams.name;
    sf::RenderWindow window(sf::VideoMode({config.windowParams.width, config.windowParams.height}),
                            windowTitle);
    window.setFramerateLimit(60);

    auto actualSize = window.getSize(); // i3 resizes

    double initialWidth = 4.0;
    double initialHeight = initialWidth * actualSize.y / double(actualSize.x);
    Viewport viewport{0, 0, initialWidth, initialHeight};

    sf::Image *image = new sf::Image(actualSize);
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