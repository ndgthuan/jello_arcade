#pragma once
#include <SFML/Graphics.hpp>
#include <map>
#include <string>
#include <memory>
#include "../../Data/Config/json.hpp"

/**
 * Resource manager class implementing the Singleton pattern
 * Handles loading and management of game assets (textures, fonts)
 * Provides centralized access to all game resources
 */
class ResourceManager
{
public:
    // Get the singleton instance of the resource manager
    static ResourceManager &getInstance();

    // Load resources from a JSON configuration file
    bool loadConfig(const std::string &filename);

    // Get texture by resource ID
    const sf::Texture &getTexture(const std::string &id) const;
    // Get font by resource ID
    const sf::Font &getFont(const std::string &id) const;

private:
    // Private constructor for singleton pattern
    ResourceManager() = default;

    // Helper methods for loading specific resource types
    bool loadTextures(const nlohmann::json &textures);
    bool loadFonts(const nlohmann::json &fonts);
    bool loadAudio(const nlohmann::json &audio);

    // Storage containers for loaded resources
    std::map<std::string, sf::Texture> m_textures; // Texture storage by ID
    std::map<std::string, sf::Font> m_fonts;       // Font storage by ID
};
