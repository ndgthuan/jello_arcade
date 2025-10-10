#include "ResourceManager.h"
#include "Audio.h"
#include <fstream>
#include <iostream>
#include "../Logging/Logger.h"

ResourceManager &ResourceManager::getInstance()
{
    // Singleton pattern - return static instance
    static ResourceManager instance;
    return instance;
}

bool ResourceManager::loadConfig(const std::string &filename)
{
    Logger::getInstance().info("Loading config from: " + filename);

    // Open configuration file
    std::ifstream file(filename);
    if (!file.is_open())
    {
        Logger::getInstance().error("Failed to open config file: " + filename);
        return false;
    }

    try
    {
        // Parse JSON configuration
        nlohmann::json j;
        file >> j;

        // Load textures and fonts if they exist in config
        if (j.contains("textures"))
        {
            Logger::getInstance().info("Found textures section in config");
            if (!loadTextures(j["textures"]))
            {
                Logger::getInstance().warning("Some textures failed to load from config");
            }
        }
        else
        {
            Logger::getInstance().warning("No textures section found in config");
        }

        if (j.contains("fonts"))
        {
            Logger::getInstance().info("Found fonts section in config");
            if (!loadFonts(j["fonts"]))
            {
                Logger::getInstance().warning("Some fonts failed to load from config");
            }
        }
        else
        {
            Logger::getInstance().warning("No fonts section found in config");
        }

        if (j.contains("audio"))
        {
            Logger::getInstance().info("Found audio section in config");
            if (!loadAudio(j["audio"]))
            {
                Logger::getInstance().warning("Some audio failed to load from config");
            }
        }
        else
        {
            Logger::getInstance().warning("No audio section found in config");
        }

        Logger::getInstance().info("Successfully loaded resource config: " + filename);
        return true;
    }
    catch (const std::exception &e)
    {
        Logger::getInstance().error("Failed to parse config file " + filename + ": " + e.what());
        return false;
    }
}

bool ResourceManager::loadTextures(const nlohmann::json &textures)
{
    int successCount = 0;
    int totalCount = 0;

    // Load each texture from the configuration
    for (auto &[id, path] : textures.items())
    {
        totalCount++;
        sf::Texture texture;
        if (!texture.loadFromFile(path))
        {
            Logger::getInstance().warning("Failed to load texture: " + std::string(path) + " (ID: " + id + ")");
            continue;
        }
        m_textures[id] = texture;
        successCount++;
    }

    Logger::getInstance().info("Loaded " + std::to_string(successCount) + "/" + std::to_string(totalCount) + " textures");
    return successCount > 0;
}

bool ResourceManager::loadFonts(const nlohmann::json &fonts)
{
    int successCount = 0;
    int totalCount = 0;

    // Load each font from the configuration
    for (auto &[id, path] : fonts.items())
    {
        totalCount++;
        sf::Font font;
        if (!font.openFromFile(path))
        {
            Logger::getInstance().warning("Failed to load font: " + std::string(path) + " (ID: " + id + ")");
            continue;
        }
        m_fonts[id] = font;
        successCount++;
    }

    Logger::getInstance().info("Loaded " + std::to_string(successCount) + "/" + std::to_string(totalCount) + " fonts");
    return successCount > 0;
}

const sf::Texture &ResourceManager::getTexture(const std::string &id) const
{
    try
    {
        return m_textures.at(id);
    }
    catch (const std::out_of_range &e)
    {
        Logger::getInstance().error("Texture not found: " + id);
        throw std::runtime_error("Texture not found: " + id);
    }
}

const sf::Font &ResourceManager::getFont(const std::string &id) const
{
    try
    {
        return m_fonts.at(id);
    }
    catch (const std::out_of_range &e)
    {
        Logger::getInstance().error("Font not found: " + id);
        throw std::runtime_error("Font not found: " + id);
    }
}

bool ResourceManager::loadAudio(const nlohmann::json &audio)
{
    int successCount = 0;
    int totalCount = 0;

    // Load each audio from the configuration
    for (auto &[id, path] : audio.items())
    {
        totalCount++;
        try
        {
            AudioManager::getInstance().loadSound(id, path);
            successCount++;
        }
        catch (const std::exception &e)
        {
            Logger::getInstance().error("Failed to load audio " + id + ": " + e.what());
        }
    }

    Logger::getInstance().info("Loaded " + std::to_string(successCount) + "/" + std::to_string(totalCount) + " audio files");
    return successCount == totalCount;
}
