#pragma once
#include <SFML/Graphics.hpp>
#include <functional>
#include <memory>

// Forward declarations
class InputHandler;
class BackgroundManager;
class CameraManager;
class MenuUI;

/**
 * Game Callback Manager class
 * Follows Single Responsibility Principle - only handles callback setup and management
 * Separates callback logic from GameCoordinator
 */
class GameCallbackManager
{
public:
    GameCallbackManager();
    ~GameCallbackManager() = default;

    // Setup all input callbacks
    void setupInputCallbacks(
        InputHandler &inputHandler,
        std::function<void(const sf::Vector2f &, const sf::Vector2f &)> mouseMoveCallback,
        std::function<void(const sf::Vector2f &)> mouseClickCallback,
        std::function<void()> exitCallback);

    // Set callbacks
    void setExitCallback(std::function<void()> callback);

private:
    // Callback storage
    std::function<void()> m_exitCallback;

    // Helper methods for specific callback setup
    void setupMouseMoveCallback(
        InputHandler &inputHandler,
        std::function<void(const sf::Vector2f &, const sf::Vector2f &)> mouseMoveCallback);

    void setupCloseCallback(InputHandler &inputHandler);

    void setupMouseClickCallback(
        InputHandler &inputHandler,
        std::function<void(const sf::Vector2f &)> mouseClickCallback);
};
