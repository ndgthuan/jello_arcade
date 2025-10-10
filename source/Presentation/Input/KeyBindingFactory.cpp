#include "KeyBindingFactory.h"

KeyBindingFactory &KeyBindingFactory::getInstance()
{
    static KeyBindingFactory instance;
    return instance;
}

KeyBindingFactory::KeyBindingFactory()
{
    initializeDefaultMappings();
}

void KeyBindingFactory::initializeDefaultMappings()
{
    // Register all default key mappings
    registerKey("F11", sf::Keyboard::Key::F11);
    registerKey("Escape", sf::Keyboard::Key::Escape);
    registerKey("P", sf::Keyboard::Key::P);
    registerKey("M", sf::Keyboard::Key::M);
    registerKey("Q", sf::Keyboard::Key::Q);
    registerKey("W", sf::Keyboard::Key::W);
    registerKey("A", sf::Keyboard::Key::A);
    registerKey("S", sf::Keyboard::Key::S);
    registerKey("D", sf::Keyboard::Key::D);
    registerKey("Space", sf::Keyboard::Key::Space);
    registerKey("Enter", sf::Keyboard::Key::Enter);
    registerKey("Tab", sf::Keyboard::Key::Tab);
    registerKey("Shift", sf::Keyboard::Key::LShift);
    registerKey("Ctrl", sf::Keyboard::Key::LControl);
    registerKey("Alt", sf::Keyboard::Key::LAlt);
}

void KeyBindingFactory::registerKey(const std::string &keyName, sf::Keyboard::Key keyCode)
{
    m_keyMappings[keyName] = keyCode;
}

sf::Keyboard::Key KeyBindingFactory::stringToKey(const std::string &keyString) const
{
    auto it = m_keyMappings.find(keyString);
    if (it != m_keyMappings.end())
    {
        return it->second;
    }

    // Return Unknown if key not found
    return sf::Keyboard::Key::Unknown;
}

bool KeyBindingFactory::isKeyRegistered(const std::string &keyName) const
{
    return m_keyMappings.find(keyName) != m_keyMappings.end();
}
