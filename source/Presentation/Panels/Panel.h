#pragma once
#include "../Game/FlappyCommon.h"

namespace Panels
{
    // Base class of every menu panel. Panels live in the 1280x720 logical space; PanelManager takes care of
    // dimming the menu, the letterboxed view and routing window events here.
    class Panel
    {
    public:
        virtual ~Panel() = default;

        // Called every time the panel is opened: reload values from UserData, reset focus / scroll / animation.
        virtual void onOpen() = 0;
        virtual void update(float dt) = 0;
        // Draws the card and everything on it (the dim backdrop is drawn by the manager).
        virtual void draw(sf::RenderTarget &target) const = 0;

        virtual void handleMouseMove(sf::Vector2f point) = 0;
        virtual void handleMouseDown(sf::Vector2f point) = 0;
        virtual void handleMouseUp(sf::Vector2f point) = 0;
        virtual void handleText(char32_t) {}
        virtual void handleKey(sf::Keyboard::Key) {}

        // Set by the panel's own Back / Close button.
        bool closeRequested() const { return m_closeRequested; }
        void clearCloseRequest() { m_closeRequested = false; }

        // Set by Settings when the player presses "Fullscreen"; the window owner performs the toggle.
        bool consumeFullscreenRequest()
        {
            const bool requested = m_fullscreenRequested;
            m_fullscreenRequested = false;
            return requested;
        }

        // The game picker sets this to the chosen game id (0..3); the window owner starts that game.
        int consumeLaunchGame()
        {
            const int game = m_launchGame;
            m_launchGame = -1;
            return game;
        }

        // A panel can ask for another one to be opened (a PanelId value, see PanelManager.h), or -1
        int consumeOpenPanel()
        {
            const int panel = m_openPanel;
            m_openPanel = -1;
            return panel;
        }

        // True after the panel changed something the rest of the app should react to (volumes, profile).
        bool consumeChanged()
        {
            const bool changed = m_changed;
            m_changed = false;
            return changed;
        }

    protected:
        bool m_closeRequested = false;
        bool m_fullscreenRequested = false;
        bool m_changed = false;
        int m_launchGame = -1;
        int m_openPanel = -1;
        float m_open = 0.0f; // open animation 0..1, advance it in update()
    };
} // namespace Panels
