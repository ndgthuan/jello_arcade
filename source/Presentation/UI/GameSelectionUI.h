#pragma once
#include <SFML/Graphics.hpp>
#include <memory>
#include <vector>
#include <functional>
#include "UIComponents.h"
#include "UIServices.h"

// Forward declarations
class IResourceService;
class IUIConfigurationService;

namespace Presentation
{

    /**
     * Game Selection UI for choosing between 4 games
     * Replaces the old circle menu with game-specific selection
     */
    class GameSelectionUI
    {
    public:
        GameSelectionUI();
        ~GameSelectionUI() = default;

        // Initialization
        void initialize(
            const sf::Font &font,
            std::shared_ptr<IUIConfigurationService> configService,
            std::shared_ptr<IResourceService> resourceService);

        // UI management
        void update(float deltaTime);
        void draw(sf::RenderWindow &window);
        void handleMouseMove(const sf::Vector2f &mousePos);
        void handleMouseClick(const sf::Vector2f &clickPos);

        // Positioning
        void setPosition(float x, float y);
        void centerOnWindow(const sf::Vector2f &windowSize);

        // Game selection callbacks
        void setGameSelectionCallback(std::function<void(int)> callback);
        void setBackToMenuCallback(std::function<void()> callback);

        // Game data
        void setGames(const std::vector<std::string> &gameNames);
        int getSelectedGame() const { return m_selectedGame; }

        // Visibility
        void setVisible(bool visible) { m_visible = visible; }
        bool isVisible() const { return m_visible; }

    private:
        // UI components
        std::vector<std::unique_ptr<CircleItem>> m_gameItems;
        std::unique_ptr<Button> m_backButton;
        std::unique_ptr<TitleText> m_titleText;

        // Game data
        std::vector<std::string> m_gameNames;
        int m_selectedGame;
        bool m_visible;

        // Services
        std::shared_ptr<IUIConfigurationService> m_configService;
        std::shared_ptr<IResourceService> m_resourceService;

        // Callbacks
        std::function<void(int)> m_gameSelectionCallback;
        std::function<void()> m_backToMenuCallback;

        // Helper methods
        void createGameItems();
        void setupGameItem(int index, const std::string &gameName);
        void updateGameItemPositions();
        void handleGameItemHover(const sf::Vector2f &mousePos);
        void handleGameItemClick(const sf::Vector2f &clickPos);
    };

} // namespace Presentation
