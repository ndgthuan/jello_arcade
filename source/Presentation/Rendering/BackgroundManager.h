#pragma once
#include <SFML/Graphics.hpp>
#include <memory>

// Forward declaration
class IResourceService;

/**
 * Background manager class that handles background sprite and positioning
 * Manages background scaling, positioning, and texture loading
 * Separates background logic from window management
 * Follows Dependency Inversion Principle by depending on IResourceService
 */
class BackgroundManager
{
public:
    // Constructor - initializes background manager
    BackgroundManager();
    // Constructor with resource service injection
    BackgroundManager(std::shared_ptr<IResourceService> resourceService);
    // Copy constructor - needed for shared_ptr
    BackgroundManager(const BackgroundManager &other);

    // Background management methods
    void loadBackground(const std::string &textureId);
    void setBackgroundScale(float scaleX, float scaleY);
    void setBackgroundPosition(float x, float y);
    void centerBackground(const sf::Vector2f &windowSize);

    // Rendering methods
    void draw(sf::RenderWindow &window);

    // Getter methods
    sf::FloatRect getBounds() const;
    bool isInitialized() const { return m_isInitialized; }

    // Setter for resource service
    void setResourceService(std::shared_ptr<IResourceService> resourceService);

private:
    // Background sprite
    std::unique_ptr<sf::Sprite> m_background;

    // State
    bool m_isInitialized;
    std::string m_textureId;

    // Resource service - follows DIP
    std::shared_ptr<IResourceService> m_resourceService;
};
