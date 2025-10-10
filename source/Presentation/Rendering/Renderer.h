#pragma once
#include <SFML/Graphics.hpp>
#include <memory>
#include "BackgroundManager.h"
#include "ParticleSystem.h"

/**
 * Centralized rendering system that manages all drawing operations
 * Provides a clean interface for rendering backgrounds, particles, and managing views
 * Separates rendering logic from game logic for better maintainability
 * Follows Dependency Inversion Principle by using smart pointers
 * Includes BackgroundManager functionality
 */
class Renderer
{
public:
    // Constructor - initializes renderer state
    Renderer();

    // Background management methods - from BackgroundManager
    void setBackground(std::shared_ptr<BackgroundManager> backgroundManager);
    void loadBackground(const std::string &textureId);
    void setBackgroundScale(float scaleX, float scaleY);
    void setBackgroundPosition(float x, float y);
    void centerBackground(const sf::Vector2f &windowSize);

    // Core rendering methods
    void clear(sf::RenderWindow &window, const sf::Color &color = sf::Color(0, 128, 128));
    void drawBackground(sf::RenderWindow &window);
    void drawParticles(sf::RenderWindow &window, ParticleSystem &particleSystem);

    // View management methods
    void setView(sf::RenderWindow &window, const sf::View &view);
    void resetView(sf::RenderWindow &window);
    void display(sf::RenderWindow &window);

    // Getter methods - from BackgroundManager
    sf::FloatRect getBounds() const;
    bool isInitialized() const;

private:
    // Background manager reference - using smart pointer for better memory management
    std::shared_ptr<BackgroundManager> m_backgroundManager;
};
