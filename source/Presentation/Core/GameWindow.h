#pragma once
#include <SFML/Graphics.hpp>
#include <SFML/System.hpp>
#include <SFML/Window.hpp>
#include <memory>
#include <string>
#include <cstdint>
#include "IGameCoordinator.h"
#include "../Game/FlappyGame.h"
#include "../Game/SmashGame.h"
#include "../Game/BlasterGame.h"
#include "../Game/SlitherGame.h"
#include "../Game/StackGame.h"
#include "../Game/MenuMascot.h"
#include <vector>
#include "../Panels/PanelManager.h"

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

    // Jello Bounce game, shown when Play is clicked in the menu
    std::vector<std::unique_ptr<Flappy::IMiniGame>> m_games; // 0 Bounce, 1 Smash, 2 Blaster, 3 Slither
    Flappy::IMiniGame *m_current = nullptr;
    Flappy::MenuMascot m_mascot; // the jello character on the main menu
    std::unique_ptr<Panels::PanelManager> m_panels; // Settings / Credits / Help / Alt Manager over the menu
    bool m_inGame = false;

    // Esc that closed a panel / left the game may still be held down: the menu must not read it as "exit app"
    bool m_waitEscRelease = false;

    // Window state tracking - from WindowManager
    bool m_isFullscreen;

    // Private helper methods
    void initializeWindow();
    static sf::VideoMode windowedMode(const sf::VideoMode &desktop);
    void centerWindow(const sf::VideoMode &desktop);
    void setWindowIcon();
    void connectIconCallback();
    void processGameEvents();
    void processPanelEvents();
    void applyAudioSettings();
};
