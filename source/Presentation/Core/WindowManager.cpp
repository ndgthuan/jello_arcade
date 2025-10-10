#include "WindowManager.h"
#include <iostream>

WindowManager::WindowManager() : m_isFullscreen(false)
{
    // Initialize window state as windowed mode
}

void WindowManager::toggleFullscreen(sf::RenderWindow &window, const sf::VideoMode &mode, const std::string &title)
{
    // Store current window position and size for restoration
    sf::Vector2i windowPosition = window.getPosition();
    sf::Vector2u windowSize = window.getSize();

    // Toggle window state
    m_isFullscreen = !m_isFullscreen;

    if (m_isFullscreen)
    {
        // Switch to fullscreen mode
        window.create(mode, title, sf::Style::None);
    }
    else
    {
        // Switch to windowed mode - only close button
        window.create(mode, title, sf::Style::Close);

        // Restore window position and size
        window.setPosition(windowPosition);
    }

    // Restore view and ensure window is focused
    window.setView(window.getDefaultView());

    // Force focus and make sure window is active
    window.requestFocus();

// Additional focus handling for Windows
#ifdef _WIN32
    // Give the window a moment to process the focus request
    sf::sleep(sf::milliseconds(50));
    window.requestFocus();
#endif
}
