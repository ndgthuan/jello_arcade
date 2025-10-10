#include "GameSelectionUI.h"
#include <iostream>

namespace Presentation
{

    GameSelectionUI::GameSelectionUI()
        : m_selectedGame(0), m_visible(false)
    {
    }

    void GameSelectionUI::initialize(
        const sf::Font &font,
        std::shared_ptr<IUIConfigurationService> configService,
        std::shared_ptr<IResourceService> resourceService)
    {
        m_configService = configService;
        m_resourceService = resourceService;

        // TODO: Initialize UI components
        std::cout << "GameSelectionUI initialized" << std::endl;
    }

    void GameSelectionUI::update(float deltaTime)
    {
        // TODO: Update UI animations
    }

    void GameSelectionUI::draw(sf::RenderWindow &window)
    {
        if (!m_visible)
            return;

        // TODO: Draw UI components
    }

    void GameSelectionUI::handleMouseMove(const sf::Vector2f &mousePos)
    {
        if (!m_visible)
            return;

        handleGameItemHover(mousePos);
    }

    void GameSelectionUI::handleMouseClick(const sf::Vector2f &clickPos)
    {
        if (!m_visible)
            return;

        handleGameItemClick(clickPos);
    }

    void GameSelectionUI::setPosition(float x, float y)
    {
        // TODO: Set position for all UI components
    }

    void GameSelectionUI::centerOnWindow(const sf::Vector2f &windowSize)
    {
        // TODO: Center UI on window
    }

    void GameSelectionUI::setGameSelectionCallback(std::function<void(int)> callback)
    {
        m_gameSelectionCallback = callback;
    }

    void GameSelectionUI::setBackToMenuCallback(std::function<void()> callback)
    {
        m_backToMenuCallback = callback;
    }

    void GameSelectionUI::setGames(const std::vector<std::string> &gameNames)
    {
        m_gameNames = gameNames;
        createGameItems();
    }

    void GameSelectionUI::createGameItems()
    {
        // TODO: Create game item UI components
    }

    void GameSelectionUI::setupGameItem(int index, const std::string &gameName)
    {
        // TODO: Setup individual game item
    }

    void GameSelectionUI::updateGameItemPositions()
    {
        // TODO: Update positions of game items
    }

    void GameSelectionUI::handleGameItemHover(const sf::Vector2f &mousePos)
    {
        // TODO: Handle hover effects
    }

    void GameSelectionUI::handleGameItemClick(const sf::Vector2f &clickPos)
    {
        // TODO: Handle click events and call callbacks
    }

} // namespace Presentation
