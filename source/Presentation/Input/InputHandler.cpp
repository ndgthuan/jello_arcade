#include "InputHandler.h"
#include <SFML/Window.hpp>
#include <fstream>
#include <iostream>
#include "../../Data/Config/json.hpp"

InputHandler::InputHandler() : m_mousePosition(0, 0), m_windowSize(0, 0)
{
    // Initialize mouse position and window size to zero
    loadKeyBindings("source/Presentation/Input/Keybinds.json");
}

void InputHandler::processEvents(sf::RenderWindow &window)
{
    // Update window size for coordinate calculations
    m_windowSize = sf::Vector2f(window.getSize());

    // Update previous key states
    m_previousKeyStates = m_keyStates;

    // Process all pending events
    while (auto event = window.pollEvent())
    {
        if (event->is<sf::Event::Closed>())
        {
            handleClose();
        }
        else if (event->is<sf::Event::MouseMoved>())
        {
            handleMouseMove(*event, window);
        }
        else if (event->is<sf::Event::MouseButtonPressed>())
        {
            handleMouseClick(*event, window);
        }
        else if (event->is<sf::Event::KeyPressed>())
        {
            handleKeyPress(*event);
        }
    }
}

void InputHandler::setMouseMoveCallback(std::function<void(const sf::Vector2f &, const sf::Vector2f &)> callback)
{
    // Set callback function for mouse movement events
    m_mouseMoveCallback = callback;
}

void InputHandler::setCloseCallback(std::function<void()> callback)
{
    // Set callback function for window close events
    m_closeCallback = callback;
}

void InputHandler::setMouseClickCallback(std::function<void(const sf::Vector2f &)> callback)
{
    // Set callback function for mouse click events
    m_mouseClickCallback = callback;
}

void InputHandler::handleMouseMove(const sf::Event &event, sf::RenderWindow &window)
{
    // Convert pixel coordinates to world coordinates
    m_mousePosition = window.mapPixelToCoords(sf::Mouse::getPosition(window));

    // Execute mouse move callback if set
    if (m_mouseMoveCallback)
    {
        m_mouseMoveCallback(m_mousePosition, m_windowSize);
    }
}

void InputHandler::handleClose()
{
    // Execute close callback if set
    if (m_closeCallback)
    {
        m_closeCallback();
    }
}

void InputHandler::handleMouseClick(const sf::Event &event, sf::RenderWindow &window)
{
    // Execute mouse click callback if set
    if (m_mouseClickCallback)
    {
        // Convert pixel coordinates to world coordinates for click position
        sf::Vector2f clickPos = window.mapPixelToCoords(sf::Mouse::getPosition(window));
        m_mouseClickCallback(clickPos);
    }
}

void InputHandler::handleKeyPress(const sf::Event &event)
{
    // Update key states for all configured keys
    for (const auto &[keyName, keyCode] : m_keyBindings)
    {
        // Check if this key is currently pressed
        if (sf::Keyboard::isKeyPressed(keyCode))
        {
            m_keyStates[keyName] = true;
        }
        else
        {
            m_keyStates[keyName] = false;
        }
    }
}

void InputHandler::loadKeyBindings(const std::string &configFile)
{
    std::ifstream file(configFile);
    if (!file.is_open())
    {
        std::cerr << "Failed to open keybinds file: " << configFile << "\n";
        return;
    }

    nlohmann::json j;
    file >> j;

    // Load keyboard bindings
    if (j.contains("keyboard"))
    {
        for (const auto &[keyName, keyString] : j["keyboard"].items())
        {
            sf::Keyboard::Key keyCode = stringToKey(keyString);
            m_keyBindings[keyName] = keyCode;
            m_keyStates[keyName] = false;
            m_previousKeyStates[keyName] = false;
        }
    }
}

sf::Keyboard::Key InputHandler::stringToKey(const std::string &keyString) const
{
    // Key mapping
    if (keyString == "F11")
        return sf::Keyboard::Key::F11;
    if (keyString == "Escape")
        return sf::Keyboard::Key::Escape;
    if (keyString == "P")
        return sf::Keyboard::Key::P;
    if (keyString == "M")
        return sf::Keyboard::Key::M;
    if (keyString == "Q")
        return sf::Keyboard::Key::Q;
    if (keyString == "W")
        return sf::Keyboard::Key::W;
    if (keyString == "A")
        return sf::Keyboard::Key::A;
    if (keyString == "S")
        return sf::Keyboard::Key::S;
    if (keyString == "D")
        return sf::Keyboard::Key::D;
    if (keyString == "Space")
        return sf::Keyboard::Key::Space;
    if (keyString == "Enter")
        return sf::Keyboard::Key::Enter;
    if (keyString == "Tab")
        return sf::Keyboard::Key::Tab;
    if (keyString == "Shift")
        return sf::Keyboard::Key::LShift;
    if (keyString == "Ctrl")
        return sf::Keyboard::Key::LControl;
    if (keyString == "Alt")
        return sf::Keyboard::Key::LAlt;

    // Return Unknown if key not found
    return sf::Keyboard::Key::Unknown;
}

bool InputHandler::isKeyPressed(const std::string &keyName) const
{
    // Check if key binding exists
    auto it = m_keyBindings.find(keyName);
    if (it != m_keyBindings.end())
    {
        return sf::Keyboard::isKeyPressed(it->second);
    }
    return false;
}

bool InputHandler::isKeyJustPressed(const std::string &keyName) const
{
    // Check if key binding exists
    auto it = m_keyBindings.find(keyName);
    if (it != m_keyBindings.end())
    {
        // Check if key is currently pressed but wasn't pressed before
        bool currentlyPressed = sf::Keyboard::isKeyPressed(it->second);
        bool wasPressed = m_previousKeyStates[keyName];

        return currentlyPressed && !wasPressed;
    }
    return false;
}

void InputHandler::resetKeyStates()
{
    // Reset all key states to false to prevent stuck keys after window recreation
    for (auto &[keyName, _] : m_keyStates)
    {
        m_keyStates[keyName] = false;
        m_previousKeyStates[keyName] = false;
    }

    // Also reset mouse position to prevent issues
    m_mousePosition = sf::Vector2f(0, 0);
}