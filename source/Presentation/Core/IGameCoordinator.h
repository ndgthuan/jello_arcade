#pragma once
#include <SFML/Graphics.hpp>
#include <functional>

// Forward declarations
class InputHandler;
class WindowManager;

/**
 * Interface for Game Coordinator
 * Follows Dependency Inversion Principle
 */
class IGameCoordinator
{
public:
    virtual ~IGameCoordinator() = default;

    // Game system management
    virtual void initialize(sf::RenderWindow &window) = 0;
    virtual void processEvents() = 0;
    virtual void update(float deltaTime) = 0;
    virtual void render(sf::RenderWindow &window) = 0;
    virtual void handleWindowResize(const sf::Vector2f &windowSize) = 0;

    // System access methods
    virtual InputHandler &getInputHandler() = 0;
    virtual WindowManager &getWindowManager() = 0;

    // Window control methods
    virtual void setWindowCloseCallback(std::function<void()> callback) = 0;
    virtual void setExitCallback(std::function<void()> callback) = 0;
    virtual void setIconCallback(std::function<void(int)> callback) = 0;

    // Game management methods (new for 4 games)
    virtual void switchToGame(int gameIndex) = 0;
    virtual int getCurrentGame() const = 0;
    virtual void pauseGame() = 0;
    virtual void resumeGame() = 0;
    virtual bool isGamePaused() const = 0;
};
