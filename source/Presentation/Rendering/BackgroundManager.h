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
    // Renders the twilight scene of the game (sky, sun, hills, neon floor) into a texture and uses it as the
    // background; scaled to cover the window with a margin so the camera can pan.
    void generateTwilight();
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
    // Background sprite (the generated texture, when used, is shared so copies keep it alive)
    std::unique_ptr<sf::Sprite> m_background;
    std::shared_ptr<sf::Texture> m_generatedTexture;
    bool m_cover = false;

    // State
    bool m_isInitialized;
    std::string m_textureId;

    // Resource service - follows DIP
    std::shared_ptr<IResourceService> m_resourceService;
};
