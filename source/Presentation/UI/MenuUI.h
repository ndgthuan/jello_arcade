#pragma once
#include <SFML/Graphics.hpp>
#include <memory>
#include "UIComponents.h"
#include "UIServices.h"

/**
 * Main menu UI class that coordinates all UI components
 * Delegates specific functionality to specialized UI classes
 * Acts as a facade for the entire menu system
 */
class MenuUI
{
public:
    // Constructor - initializes UI state variables
    MenuUI();

    // Core UI management methods
    void initialize(const sf::Font &font, std::shared_ptr<IUIConfigurationService> configService = nullptr, std::shared_ptr<IResourceService> resourceService = nullptr); // Setup UI with specified font
    void setJelloFont(const sf::Font &font);                                                                                                                              // Set font for Jello text
    void setPosition(float x, float y);                                                                                                                                   // Position UI elements
    void centerJelloText(const sf::Vector2f &windowSize);                                                                                                                 // Center Jello text on screen
    void positionCircles(const sf::Vector2f &windowSize);                                                                                                                 // Position circles below Jello text
    void setSelected(bool selected);                                                                                                                                      // Set hover/selection state
    void update(float deltaTime);                                                                                                                                         // Update animations and effects
    void updateHover(const sf::Vector2f &mousePos);                                                                                                                       // Update hover state for all components
    void handleClick(const sf::Vector2f &clickPos);                                                                                                                       // Handle button clicks
    void draw(sf::RenderWindow &window);                                                                                                                                  // Render UI elements

    // Getter methods for UI state
    bool isSelected() const;         // Check if UI is currently selected/hovered
    sf::FloatRect getBounds() const; // Get UI bounds for collision detection

    // Public method to recenter Jello text (useful for window resize events)
    void recenterJelloText(const sf::Vector2f &windowSize);

private:
    // UI components - each handles its own specific functionality
    std::unique_ptr<Button> m_exitButton;     // Exit button with hover effects
    std::unique_ptr<TitleText> m_titleText;   // Jello title with glow effects
    std::unique_ptr<CircleMenu> m_circleMenu; // Circle menu with icons

    // State
    bool m_isInitialized;   // Initialization state flag
    const sf::Font *m_font; // Font reference for text rendering

    // Services - follows DIP
    std::shared_ptr<IUIConfigurationService> m_configService;
    std::shared_ptr<IResourceService> m_resourceService;

    // Helper methods
    void initializeComponents(const sf::Font &font);
    void setupCircleMenu();
};
