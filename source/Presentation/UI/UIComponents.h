#pragma once
#include <SFML/Graphics.hpp>
#include <vector>
#include <memory>
#include <functional>
#include <optional>
#include <string>
#include <cmath>

// Forward declarations
class IResourceService;
class IUIConfigurationService;
class IUIPositioningService;

// ===== POSITIONING STRATEGIES =====

/**
 * Interface for positioning strategies
 * Follows Open/Closed Principle - allows new positioning strategies without modifying existing code
 */
class IPositioningStrategy
{
public:
    virtual ~IPositioningStrategy() = default;

    // Calculate position for UI elements
    virtual sf::Vector2f calculatePosition(const sf::Vector2f &windowSize,
                                           const sf::Vector2f &titlePosition,
                                           const sf::FloatRect &titleBounds,
                                           size_t elementIndex,
                                           size_t totalElements) = 0;
};

/**
 * Icon positioning strategy interface - follows OCP
 */
class IIconPositioningStrategy
{
public:
    virtual ~IIconPositioningStrategy() = default;
    virtual sf::Vector2f calculateIconPosition(
        const sf::Vector2f &circleCenter,
        const sf::FloatRect &iconBounds,
        const std::string &iconName) const = 0;
};

/**
 * Concrete positioning strategy for circle menu layout
 * Implements the specific positioning logic for circles and labels
 */
class CircleMenuPositioningStrategy : public IPositioningStrategy
{
public:
    CircleMenuPositioningStrategy(float circleRadius, float spacing);

    // Calculate position for circle and label
    sf::Vector2f calculatePosition(const sf::Vector2f &windowSize,
                                   const sf::Vector2f &titlePosition,
                                   const sf::FloatRect &titleBounds,
                                   size_t elementIndex,
                                   size_t totalElements) override;

    // Get label position separately (since we need both circle and label positions)
    sf::Vector2f calculateLabelPosition(const sf::Vector2f &circlePosition,
                                        const sf::FloatRect &labelBounds) const;

private:
    float m_circleRadius;
    float m_spacing;
};

/**
 * Default icon positioning strategy
 * Provides standard icon positioning with special handling for specific icons
 */
class DefaultIconPositioningStrategy : public IIconPositioningStrategy
{
public:
    DefaultIconPositioningStrategy() = default;
    ~DefaultIconPositioningStrategy() = default;

    sf::Vector2f calculateIconPosition(
        const sf::Vector2f &circleCenter,
        const sf::FloatRect &iconBounds,
        const std::string &iconName) const override;
};

// ===== BUTTON CLASS =====

/**
 * Button class that handles interactive button functionality
 * Manages hover effects, click detection, and visual feedback
 * Separates button logic from general UI logic
 */
class Button
{
public:
    // Constructor - initializes button with text and font
    Button(const sf::Font &font, const std::string &text, unsigned int size = 24, std::shared_ptr<IUIConfigurationService> configService = nullptr);

    // Button state management
    void setSelected(bool selected);
    void setPosition(float x, float y);
    void setColor(const sf::Color &color);
    void setHoverColor(const sf::Color &color);

    // Interaction methods
    bool isHovered(const sf::Vector2f &mousePos) const;
    void setClickCallback(std::function<void()> callback);
    void handleClick();

    // Animation methods
    void update(float deltaTime);
    void draw(sf::RenderWindow &window);

    // Getter methods
    sf::FloatRect getBounds() const;
    bool isSelected() const { return m_isSelected; }

private:
    // Button elements
    std::unique_ptr<sf::Text> m_text;
    std::unique_ptr<sf::RectangleShape> m_progressBar;

    // Button state
    bool m_isSelected;
    bool m_isInitialized;

    // Visual properties
    sf::Color m_normalColor;
    sf::Color m_hoverColor;
    sf::Color m_currentColor;

    // Animation properties
    float m_progressBarWidth;
    float m_targetProgressBarWidth;
    float m_progressBarAlpha;
    float m_targetProgressBarAlpha;

    // Callback
    std::function<void()> m_clickCallback;

    // Configuration service - follows DIP
    std::shared_ptr<IUIConfigurationService> m_configService;

    // Helper methods
    void updateProgressBar(float deltaTime);
    void updateColor();
};

// ===== TITLE TEXT CLASS =====

/**
 * Title text class that handles large title display with effects
 * Manages title positioning, fonts, and glow effects
 * Separates title rendering from general UI logic
 * Follows Dependency Inversion Principle by depending on abstraction
 */
class TitleText
{
public:
    // Constructor - initializes title text system
    TitleText();

    // Title management methods
    void setFont(const sf::Font &font);
    void setText(const std::string &content);
    void setSize(unsigned int size);
    void setColor(const sf::Color &color);

    // Positioning methods
    void centerText(const sf::Vector2f &windowSize);
    void setCenterOffset(float x, float y);

    // Effect methods
    void enableGlow(bool enable);
    void setGlowIntensity(float intensity);
    void setGlowColor(const sf::Color &color);
    void updateGlow(float deltaTime);

    // Rendering methods
    void draw(sf::RenderWindow &window);

    // Getter methods
    sf::FloatRect getBounds() const;
    sf::Vector2f getPosition() const;
    bool isInitialized() const { return m_isInitialized; }

    // Dependency injection for positioning service
    void setPositioningService(std::shared_ptr<IUIPositioningService> positioningService);

private:
    // Text elements
    std::unique_ptr<sf::Text> m_text;
    std::vector<sf::Text> m_glowLayers;
    const sf::Font *m_font;

    // State
    bool m_isInitialized;
    bool m_glowEnabled;
    float m_glowIntensity;
    float m_animationTime;
    sf::Color m_glowColor;
    sf::Vector2f m_centerOffset;
    sf::Vector2f m_basePosition;

    // Positioning service - follows DIP
    std::shared_ptr<IUIPositioningService> m_positioningService;

    // Helper methods
    void updateGlowLayers();
    void applyGlowEffect();
};

// ===== CIRCLE ITEM CLASS =====

/**
 * Individual circle item class that manages a single circle with its icon and label
 * Follows Single Responsibility Principle - only handles one circle item
 * Follows Open/Closed Principle - uses strategy pattern for icon positioning
 */
class CircleItem
{
public:
    // Constructor - initializes circle item
    CircleItem();

    // Initialization methods
    void initialize(float radius, const sf::Color &color, const sf::Color &outlineColor, float outlineThickness);
    void setIcon(const std::string &iconName, std::shared_ptr<IResourceService> resourceService = nullptr);
    void setLabel(const std::string &labelText, const sf::Font &font);

    // Positioning methods
    void setPosition(float x, float y);
    void setLabelPosition(float x, float y);

    // Visual customization
    void setCircleColor(const sf::Color &color);
    void setOutlineColor(const sf::Color &color);
    void setOutlineThickness(float thickness);

    // Strategy injection for icon positioning
    void setIconPositioningStrategy(std::shared_ptr<IIconPositioningStrategy> strategy);

    // Hover and animation methods
    void updateHover(const sf::Vector2f &mousePos);
    void update(float deltaTime);
    void setHoverScale(float scale);

    // Primary item gets an accent fill and a pulsing ring so it reads as the main action
    void setPrimary(bool primary, const sf::Color &accent);

    // Rendering methods
    void draw(sf::RenderWindow &window);

    // Getter methods
    sf::FloatRect getBounds() const;
    sf::FloatRect getLabelBounds() const;
    bool isHovered() const { return m_isHovered; }
    float getHoverScale() const { return m_hoverScale; }

private:
    // Circle elements
    sf::CircleShape m_circle;
    std::optional<sf::Sprite> m_icon;
    std::optional<sf::Text> m_label;

    // State
    bool m_isInitialized;
    bool m_isHovered;
    float m_hoverScale;
    float m_hoverScaleTarget;
    float m_hoverScaleSpeed;

    // Primary item highlight
    bool m_primary = false;
    sf::Color m_accent = sf::Color::White;
    float m_pulseTime = 0.0f;

    // Icon identification
    std::string m_iconName;

    // Icon positioning strategy - follows OCP
    std::shared_ptr<IIconPositioningStrategy> m_iconPositioningStrategy;

    // Helper methods
    void updateHoverAnimation(float deltaTime);
    sf::Vector2f calculateIconPosition() const;
};

// ===== CIRCLE MENU CLASS =====

/**
 * Circle menu layout manager that handles positioning and arrangement of circle items
 * Follows Single Responsibility Principle - only handles layout and positioning
 * Delegates individual circle management to CircleItem class
 * Uses positioning strategy to follow Open/Closed Principle
 */
class CircleMenu
{
public:
    // Constructor - initializes circle menu layout
    CircleMenu();

    // Layout management methods
    void initialize(int count = 5, std::shared_ptr<IUIConfigurationService> configService = nullptr, std::shared_ptr<IResourceService> resourceService = nullptr);
    void setCircleRadius(float radius);
    void setSpacing(float spacing);

    // Positioning methods
    void positionCircles(const sf::Vector2f &windowSize, const sf::Vector2f &titlePosition, const sf::FloatRect &titleBounds);

    // Visual customization
    void setCircleColor(const sf::Color &color);
    void setOutlineColor(const sf::Color &color);
    void setOutlineThickness(float thickness);

    // Hover and animation methods
    void updateHover(const sf::Vector2f &mousePos);
    void update(float deltaTime);

    // Rendering methods
    void draw(sf::RenderWindow &window);

    // Getter methods
    size_t getCircleCount() const { return m_circleItems.size(); }
    sf::FloatRect getCircleBounds(size_t index) const;
    bool isInitialized() const { return m_isInitialized; }

private:
    // Circle items - each manages its own state
    std::vector<std::unique_ptr<CircleItem>> m_circleItems;

    // Layout properties
    bool m_isInitialized;
    float m_circleRadius;
    float m_spacing;
    sf::Color m_circleColor;
    sf::Color m_outlineColor;
    float m_outlineThickness;

    // Positioning strategy - follows Strategy pattern and OCP
    std::unique_ptr<CircleMenuPositioningStrategy> m_positioningStrategy;

    // Services - follows DIP
    std::shared_ptr<IUIConfigurationService> m_configService;
    std::shared_ptr<IResourceService> m_resourceService;

    // Helper methods
    void setupCircleItems();
    void updatePositioningStrategy();
};
