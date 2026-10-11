#pragma once
#include "Panel.h"
#include "SettingsPanel.h"
#include "InfoPanels.h"
#include "AccountPanel.h"
#include "GamePickerPanel.h"
#include "TrophiesPanel.h"
#include <memory>

namespace Panels
{
    enum class PanelId
    {
        Settings,
        Credits,
        Help,
        Accounts,
        Games,
        Trophies
    };

    // Owns the four panels and shows one at a time over the menu.
    //   draw(): in the window's default view fill the whole window with a dark violet dim (alpha ~170, animated
    //   in with the panel); then switch to a letterboxed 16:9 view of the logical 1280x720 space (the same math as
    //   Flappy::ArcadeGame::applyViewport) and draw the panel; finally restore the default view.
    //   Events: mouse move / press / release (left button) are mapped with window.mapPixelToCoords to the logical
    //   view; TextEntered goes to Panel::handleText; KeyPressed goes to Panel::handleKey, except Escape which
    //   closes the panel. A panel's closeRequested() closes it as well.
    class PanelManager
    {
    public:
        explicit PanelManager(const sf::Font &font);

        void open(PanelId id);
        void close();
        bool isOpen() const { return m_current != nullptr; }

        void handleEvent(const sf::Event &event, sf::RenderWindow &window);
        void update(float dt);
        void draw(sf::RenderWindow &window);

        // The window owner polls these after handling events.
        bool consumeFullscreenRequest();
        bool consumeChanged();
        // Id (0..3) of the game picked in the Games panel, or -1
        int consumeLaunchGame();

    private:
        std::unique_ptr<SettingsPanel> m_settings;
        std::unique_ptr<CreditsPanel> m_credits;
        std::unique_ptr<HelpPanel> m_help;
        std::unique_ptr<AccountPanel> m_accounts;
        std::unique_ptr<GamePickerPanel> m_games;
        std::unique_ptr<TrophiesPanel> m_trophies;
        Panel *m_current = nullptr;
        float m_dim = 0.0f; // 0..1 backdrop animation
        sf::View m_view;
        bool m_fullscreenRequested = false;
        bool m_changed = false;
        int m_launchGame = -1;

        void applyViewport(const sf::RenderWindow &window);
    };
} // namespace Panels
