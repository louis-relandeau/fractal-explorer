#include <ConfigLoader.hpp>

#include <yaml-cpp/yaml.h>

ConfigLoader::ConfigLoader(std::string const& filename) {
    YAML::Node config = YAML::LoadFile(filename);
    
    const auto windowNode = config["Window"];
    windowParams.width = windowNode["Width"].as<unsigned>();
    windowParams.height = windowNode["Height"].as<unsigned>();

    const auto fractalNode = config["Fractal"];
    fractalParams.name = fractalNode["Name"].as<std::string>();
}