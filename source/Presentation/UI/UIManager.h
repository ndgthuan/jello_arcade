#pragma once
#include <SFML/Graphics.hpp>
#include <SFML/System/Clock.hpp>
#include <memory>
#include "MenuUI.h"
#include "UIServices.h"

/**
 * UI Manager class
 * Follows Single Responsibility Principle - only handles UI management
 * Separates UI management from system coordination
 */
class UIManager
{
public:
    UIManager();
    ~UIManager() = default;

    // UI management
    void initialize(
        const sf::Vector2f &windowSize,
        std::shared_ptr<IUIConfigurationService> uiConfigService,
        std::shared_ptr<IResourceService> resourceService);
    void update(float deltaTime);
    void draw(sf::RenderWindow &window);
    void handleWindowResize(const sf::Vector2f &windowSize);

    // UI interaction
    void updateHover(const sf::Vector2f &mousePos);
    void handleClick(const sf::Vector2f &clickPos);
    bool isExitButtonClicked(const sf::Vector2f &clickPos);
    void setExitCallback(std::function<void()> callback);

    // Getters
    MenuUI &getMenuUI() { return m_menuUI; }
    bool isInitialized() const { return m_isInitialized; }

private:
    // UI components
    MenuUI m_menuUI;

    // State
    bool m_isInitialized;

    // Services
    std::shared_ptr<IUIConfigurationService> m_uiConfigService;
    std::shared_ptr<IResourceService> m_resourceService;

    // Callbacks
    std::function<void()> m_exitCallback;

    // Delayed exit handling so click sound can play
    bool m_pendingExit = false;
    sf::Clock m_exitClock;
    float m_exitDelayMs = 500.0f;
};
