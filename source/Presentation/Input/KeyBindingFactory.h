#pragma once
#include <SFML/Window.hpp>
#include <string>
#include <unordered_map>
#include <functional>

/**
 * Factory class for key bindings
 * Follows Open/Closed Principle - easy to extend with new keys without modifying existing code
 */
class KeyBindingFactory
{
public:
    // Get singleton instance
    static KeyBindingFactory &getInstance();

    // Register a key mapping
    void registerKey(const std::string &keyName, sf::Keyboard::Key keyCode);

    // Convert string to SFML key code
    sf::Keyboard::Key stringToKey(const std::string &keyString) const;

    // Check if key is registered
    bool isKeyRegistered(const std::string &keyName) const;

private:
    // Private constructor for singleton
    KeyBindingFactory();

    // Key mapping storage
    std::unordered_map<std::string, sf::Keyboard::Key> m_keyMappings;

    // Initialize default key mappings
    void initializeDefaultMappings();
};
