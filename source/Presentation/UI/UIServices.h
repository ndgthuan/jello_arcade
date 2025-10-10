#pragma once
#include <SFML/Graphics.hpp>
#include <string>
#include <memory>

// Forward declarations
class IResourceService;

// ===== INTERFACES =====

/**
 * Interface for configuration service
 * Follows Dependency Inversion Principle - Presentation layer depends on abstraction
 * Breaks direct dependency on Data layer
 */
class IConfigurationService
{
public:
    virtual ~IConfigurationService() = default;

    // UI Configuration constants
    virtual float getTitleCenterOffsetX() const = 0;
    virtual float getTitleCenterOffsetY() const = 0;
    virtual float getTitleCircleSpacing() const = 0;
    virtual float getLabelOffsetX() const = 0;
    virtual float getLabelOffsetY() const = 0;
    virtual float getDefaultCircleRadius() const = 0;
    virtual float getDefaultCircleSpacing() const = 0;
    virtual float getCircleOutlineThickness() const = 0;
    virtual float getButtonProgressBarHeight() const = 0;
    virtual float getButtonAnimationSpeed() const = 0;
    virtual float getButtonLabelSpacing() const = 0;
    virtual unsigned int getTitleFontSize() const = 0;
    virtual float getTitleGlowAnimationSpeed() const = 0;
    virtual float getTitleGlowIntensityMin() const = 0;
    virtual float getTitleGlowIntensityMax() const = 0;
    virtual int getTitleGlowLayers() const = 0;
    virtual float getTitleGlowScaleIncrement() const = 0;
    virtual float getTitleGlowAlphaBase() const = 0;
    virtual float getTitleGlowAlphaDecrement() const = 0;
    virtual float getIconScale() const = 0;
    virtual float getIconHoverScale() const = 0;
    virtual float getIconAnimationSpeed() const = 0;
    virtual float getAccountIconOffsetX() const = 0;
    virtual float getAccountIconOffsetY() const = 0;

    // Colors
    virtual sf::Color getTitleColor() const = 0;
    virtual sf::Color getTitleGlowColor() const = 0;
    virtual sf::Color getButtonNormalColor() const = 0;
    virtual sf::Color getButtonHoverColor() const = 0;
    virtual sf::Color getCircleColor() const = 0;
    virtual sf::Color getCircleOutlineColor() const = 0;
    virtual sf::Color getLabelColor() const = 0;

    // Game Configuration constants
    virtual int getDefaultParticleCount() const = 0;
    virtual float getCameraSmoothSpeed() const = 0;
    virtual float getCameraMomentum() const = 0;
    virtual float getCameraMomentumMultiplier() const = 0;
};

/**
 * Interface for UI-specific configuration
 * Follows Interface Segregation Principle - only UI-related configuration
 */
class IUIConfigurationService
{
public:
    virtual ~IUIConfigurationService() = default;

    // UI Positioning constants
    virtual float getTitleCenterOffsetX() const = 0;
    virtual float getTitleCenterOffsetY() const = 0;
    virtual float getTitleCircleSpacing() const = 0;
    virtual float getLabelOffsetX() const = 0;
    virtual float getLabelOffsetY() const = 0;

    // Circle menu constants
    virtual float getDefaultCircleRadius() const = 0;
    virtual float getDefaultCircleSpacing() const = 0;
    virtual float getCircleOutlineThickness() const = 0;

    // Button constants
    virtual float getButtonProgressBarHeight() const = 0;
    virtual float getButtonAnimationSpeed() const = 0;
    virtual float getButtonLabelSpacing() const = 0;

    // Title text constants
    virtual unsigned int getTitleFontSize() const = 0;
    virtual float getTitleGlowAnimationSpeed() const = 0;
    virtual float getTitleGlowIntensityMin() const = 0;
    virtual float getTitleGlowIntensityMax() const = 0;
    virtual int getTitleGlowLayers() const = 0;
    virtual float getTitleGlowScaleIncrement() const = 0;
    virtual float getTitleGlowAlphaBase() const = 0;
    virtual float getTitleGlowAlphaDecrement() const = 0;

    // Icon constants
    virtual float getIconScale() const = 0;
    virtual float getIconHoverScale() const = 0;
    virtual float getIconAnimationSpeed() const = 0;
    virtual float getAccountIconOffsetX() const = 0;
    virtual float getAccountIconOffsetY() const = 0;

    // UI Colors
    virtual sf::Color getTitleColor() const = 0;
    virtual sf::Color getTitleGlowColor() const = 0;
    virtual sf::Color getButtonNormalColor() const = 0;
    virtual sf::Color getButtonHoverColor() const = 0;
    virtual sf::Color getCircleColor() const = 0;
    virtual sf::Color getCircleOutlineColor() const = 0;
    virtual sf::Color getLabelColor() const = 0;
};

/**
 * Interface for game-specific configuration
 * Follows Interface Segregation Principle - only game-related configuration
 */
class IGameConfigurationService
{
public:
    virtual ~IGameConfigurationService() = default;

    // Game system constants
    virtual int getDefaultParticleCount() const = 0;
    virtual float getCameraSmoothSpeed() const = 0;
    virtual float getCameraMomentum() const = 0;
    virtual float getCameraMomentumMultiplier() const = 0;
};

/**
 * Interface for resource service
 * Follows Dependency Inversion Principle - Presentation layer depends on abstraction
 * Replaces direct singleton usage with dependency injection
 */
class IResourceService
{
public:
    virtual ~IResourceService() = default;

    // Resource access methods
    virtual const sf::Texture &getTexture(const std::string &id) const = 0;
    virtual const sf::Font &getFont(const std::string &id) const = 0;

    // Resource loading methods
    virtual bool loadConfig(const std::string &filename) = 0;
};

/**
 * Interface for UI positioning service
 * Follows Dependency Inversion Principle - high-level modules depend on abstraction
 */
class IUIPositioningService
{
public:
    virtual ~IUIPositioningService() = default;

    // Center text on screen
    virtual void centerText(sf::Text &text, const sf::Vector2f &windowSize, const sf::Vector2f &offset = sf::Vector2f(0, 0)) = 0;

    // Position elements in a row
    virtual std::vector<sf::Vector2f> positionElementsInRow(const std::vector<sf::FloatRect> &elementBounds,
                                                            const sf::Vector2f &windowSize,
                                                            float spacing = 0.0f) = 0;

    // Calculate position below another element
    virtual sf::Vector2f positionBelow(const sf::FloatRect &aboveElement,
                                       const sf::FloatRect &currentElement,
                                       float spacing = 0.0f) = 0;
};

// ===== CONCRETE SERVICES =====

/**
 * Concrete implementation of configuration service
 * Provides access to configuration values without direct dependency on Data layer
 */
class ConfigurationService : public IConfigurationService
{
public:
    ConfigurationService() = default;

    // UI Configuration constants
    float getTitleCenterOffsetX() const override;
    float getTitleCenterOffsetY() const override;
    float getTitleCircleSpacing() const override;
    float getLabelOffsetX() const override;
    float getLabelOffsetY() const override;
    float getDefaultCircleRadius() const override;
    float getDefaultCircleSpacing() const override;
    float getCircleOutlineThickness() const override;
    float getButtonProgressBarHeight() const override;
    float getButtonAnimationSpeed() const override;
    float getButtonLabelSpacing() const override;
    unsigned int getTitleFontSize() const override;
    float getTitleGlowAnimationSpeed() const override;
    float getTitleGlowIntensityMin() const override;
    float getTitleGlowIntensityMax() const override;
    int getTitleGlowLayers() const override;
    float getTitleGlowScaleIncrement() const override;
    float getTitleGlowAlphaBase() const override;
    float getTitleGlowAlphaDecrement() const override;
    float getIconScale() const override;
    float getIconHoverScale() const override;
    float getIconAnimationSpeed() const override;
    float getAccountIconOffsetX() const override;
    float getAccountIconOffsetY() const override;

    // Colors
    sf::Color getTitleColor() const override;
    sf::Color getTitleGlowColor() const override;
    sf::Color getButtonNormalColor() const override;
    sf::Color getButtonHoverColor() const override;
    sf::Color getCircleColor() const override;
    sf::Color getCircleOutlineColor() const override;
    sf::Color getLabelColor() const override;

    // Game Configuration constants
    int getDefaultParticleCount() const override;
    float getCameraSmoothSpeed() const override;
    float getCameraMomentum() const override;
    float getCameraMomentumMultiplier() const override;
};

/**
 * Concrete implementation of UI configuration service
 * Provides access to UI configuration values
 */
class UIConfigurationService : public IUIConfigurationService
{
public:
    UIConfigurationService() = default;

    // UI Positioning constants
    float getTitleCenterOffsetX() const override;
    float getTitleCenterOffsetY() const override;
    float getTitleCircleSpacing() const override;
    float getLabelOffsetX() const override;
    float getLabelOffsetY() const override;

    // Circle menu constants
    float getDefaultCircleRadius() const override;
    float getDefaultCircleSpacing() const override;
    float getCircleOutlineThickness() const override;

    // Button constants
    float getButtonProgressBarHeight() const override;
    float getButtonAnimationSpeed() const override;
    float getButtonLabelSpacing() const override;

    // Title text constants
    unsigned int getTitleFontSize() const override;
    float getTitleGlowAnimationSpeed() const override;
    float getTitleGlowIntensityMin() const override;
    float getTitleGlowIntensityMax() const override;
    int getTitleGlowLayers() const override;
    float getTitleGlowScaleIncrement() const override;
    float getTitleGlowAlphaBase() const override;
    float getTitleGlowAlphaDecrement() const override;

    // Icon constants
    float getIconScale() const override;
    float getIconHoverScale() const override;
    float getIconAnimationSpeed() const override;
    float getAccountIconOffsetX() const override;
    float getAccountIconOffsetY() const override;

    // UI Colors
    sf::Color getTitleColor() const override;
    sf::Color getTitleGlowColor() const override;
    sf::Color getButtonNormalColor() const override;
    sf::Color getButtonHoverColor() const override;
    sf::Color getCircleColor() const override;
    sf::Color getCircleOutlineColor() const override;
    sf::Color getLabelColor() const override;
};

/**
 * Concrete implementation of game configuration service
 * Provides access to game configuration values
 */
class GameConfigurationService : public IGameConfigurationService
{
public:
    GameConfigurationService() = default;

    // Game system constants
    int getDefaultParticleCount() const override;
    float getCameraSmoothSpeed() const override;
    float getCameraMomentum() const override;
    float getCameraMomentumMultiplier() const override;
};

/**
 * Concrete implementation of resource service
 * Wraps ResourceManager to provide dependency injection instead of singleton
 */
class ResourceService : public IResourceService
{
public:
    ResourceService();
    ResourceService(const ResourceService &other);
    ~ResourceService();

    // Resource access methods
    const sf::Texture &getTexture(const std::string &id) const override;
    const sf::Font &getFont(const std::string &id) const override;

    // Resource loading methods
    bool loadConfig(const std::string &filename) override;

private:
    // Forward declaration to avoid including ResourceManager header
    class ResourceManagerWrapper;
    std::unique_ptr<ResourceManagerWrapper> m_resourceManager;
};

/**
 * Concrete implementation of UI positioning service
 * Provides common positioning logic for UI elements
 */
class UIPositioningService : public IUIPositioningService
{
public:
    UIPositioningService() = default;

    // Center text on screen
    void centerText(sf::Text &text, const sf::Vector2f &windowSize, const sf::Vector2f &offset = sf::Vector2f(0, 0)) override;

    // Position elements in a row
    std::vector<sf::Vector2f> positionElementsInRow(const std::vector<sf::FloatRect> &elementBounds,
                                                    const sf::Vector2f &windowSize,
                                                    float spacing = 0.0f) override;

    // Calculate position below another element
    sf::Vector2f positionBelow(const sf::FloatRect &aboveElement,
                               const sf::FloatRect &currentElement,
                               float spacing = 0.0f) override;
};

/**
 * UI Layout Service that handles positioning of UI elements
 * Follows Single Responsibility Principle - only handles UI layout
 * Uses positioning service to follow Dependency Inversion Principle
 */
class UILayoutService
{
public:
    UILayoutService();

    // Initialize with positioning service
    void initialize(std::shared_ptr<IUIPositioningService> positioningService);

    // Layout methods
    void centerTitleText(sf::Text &titleText, const sf::Vector2f &windowSize, const sf::Vector2f &offset);
    void positionMenuElements(const sf::Vector2f &windowSize, const sf::Vector2f &titlePosition, const sf::FloatRect &titleBounds);

    // Getter for positioning service
    std::shared_ptr<IUIPositioningService> getPositioningService() const { return m_positioningService; }

private:
    std::shared_ptr<IUIPositioningService> m_positioningService;
};
