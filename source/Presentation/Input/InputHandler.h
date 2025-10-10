#pragma once
#include <SFML/Graphics.hpp>
#include <functional>
#include <map>

/**
 * Input handler class that manages all user input events
 * Uses callback functions to delegate event processing to other systems
 * Supports mouse movement, clicks, window close events, and keyboard input
 * Includes KeyBindingFactory functionality
 */
class InputHandler
{
public:
    // Constructor - initializes input tracking variables
    InputHandler();

    // Process all pending events from the given window
    void processEvents(sf::RenderWindow &window);

    // Callback function setters for different event types
    void setMouseMoveCallback(std::function<void(const sf::Vector2f &, const sf::Vector2f &)> callback);
    void setCloseCallback(std::function<void()> callback);
    void setMouseClickCallback(std::function<void(const sf::Vector2f &)> callback);

    // Getter methods for current input state
    sf::Vector2f getMousePosition() const { return m_mousePosition; }
    sf::Vector2f getWindowSize() const { return m_windowSize; }

    // Check specific key states using key binding names
    bool isKeyPressed(const std::string &keyName) const;
    bool isKeyJustPressed(const std::string &keyName) const;

    // Reset key states (useful after window recreation)
    void resetKeyStates();

private:
    // Event handling methods for specific event types
    void handleMouseMove(const sf::Event &event, sf::RenderWindow &window);
    void handleClose();
    void handleMouseClick(const sf::Event &event, sf::RenderWindow &window);
    void handleKeyPress(const sf::Event &event);

    // Callback functions for different event types
    std::function<void(const sf::Vector2f &, const sf::Vector2f &)> m_mouseMoveCallback; // Mouse movement callback
    std::function<void()> m_closeCallback;                                               // Window close callback
    std::function<void(const sf::Vector2f &)> m_mouseClickCallback;                      // Mouse click callback

    // Current input state
    sf::Vector2f m_mousePosition; // Current mouse position in world coordinates
    sf::Vector2f m_windowSize;    // Current window dimensions

    // Key state tracking
    mutable std::map<std::string, bool> m_keyStates;
    mutable std::map<std::string, bool> m_previousKeyStates;

    // Key bindings from JSON
    std::map<std::string, sf::Keyboard::Key> m_keyBindings;

    // Helper methods
    void loadKeyBindings(const std::string &configFile);
    sf::Keyboard::Key stringToKey(const std::string &keyString) const;
};