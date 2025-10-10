#pragma once
#include <SFML/Graphics.hpp>
#include <SFML/Window.hpp>
#include <SFML/System.hpp>
#include <cstdint>
#include <string>

/**
 * WindowManager class responsible for managing window state and transitions
 * Follows Single Responsibility Principle - only handles window management
 * Separates window logic from game logic for better maintainability
 */
class WindowManager
{
public:
    // Constructor - initializes window state as windowed mode
    WindowManager();

    // Toggle between fullscreen and windowed mode
    void toggleFullscreen(sf::RenderWindow &window, const sf::VideoMode &mode, const std::string &title);

    // Check current window state
    bool isFullscreen() const { return m_isFullscreen; }

private:
    // Window state tracking - needed to toggle between modes
    bool m_isFullscreen;
};
