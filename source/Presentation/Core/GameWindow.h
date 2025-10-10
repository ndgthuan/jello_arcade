#pragma once
#include <SFML/Graphics.hpp>
#include <SFML/System.hpp>
#include <SFML/Window.hpp>
#include <memory>
#include <string>
#include <cstdint>
#include "IGameCoordinator.h"

// Forward declaration
class IResourceService;

/**
 * Main game window class that handles window management only
 * Delegates game logic to GameCoordinator
 * Follows Single Responsibility Principle - only handles window operations
 * Includes WindowManager functionality for fullscreen toggle
 */
class GameWindow
{
public:
    // Constructor - initializes window
    GameWindow();
    // Destructor - cleans up allocated resources
    ~GameWindow();

    // Main game loop methods
    void processEvents();
    void display();
    bool isOpen() const;

    // Window management
    sf::RenderWindow &getWindow();
    void toggleFullscreen();
    void ensureFocus();

    // Set resource service for dependency injection
    void setResourceService(std::shared_ptr<IResourceService> resourceService);

private:
    // Main SFML render window
    sf::RenderWindow m_window;

    // Game coordinator - handles all game logic (using interface for DIP)
    std::unique_ptr<IGameCoordinator> m_gameCoordinator;

    // Resource service - follows DIP
    std::shared_ptr<IResourceService> m_resourceService;

    // Window state tracking - from WindowManager
    bool m_isFullscreen;

    // Private helper methods
    void initializeWindow();
    void setWindowIcon();
};
