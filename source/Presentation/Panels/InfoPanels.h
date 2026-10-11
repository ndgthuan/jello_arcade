#pragma once
#include "Panel.h"
#include "Widgets.h"
#include <vector>

namespace Panels
{
    // Help: card x 190..1090, y 40..680, title "HELP". Six tabs (General, Bounce, Smash, Blaster, Slither, Stack)
    // switch the page with a click or the Left / Right keys. Each tab shows:
    //   HOW TO PLAY  - one or two lines about the game
    //   CONTROLS     - key caps + action in two columns, plus an optional power-up / pickup line
    //   TIPS         - three short bullets
    // Plus a "Back" button at the bottom.
    class HelpPanel : public Panel
    {
    public:
        explicit HelpPanel(const sf::Font &font);

        void onOpen() override;
        void update(float dt) override;
        void draw(sf::RenderTarget &target) const override;
        void handleMouseMove(sf::Vector2f point) override;
        void handleMouseDown(sf::Vector2f point) override;
        void handleMouseUp(sf::Vector2f point) override;
        void handleKey(sf::Keyboard::Key key) override;

    private:
        const sf::Font *m_font;
        std::vector<Flappy::UiKit::Button3D> m_buttons; // [0] Back
        int m_pressed = -1;
        int m_tab = 0;         // selected tab (0..5)
        int m_hoverTab = -1;   // tab under the cursor
        int m_pressedTab = -1; // tab pressed by the mouse, switches on release
        float m_time = 0.0f;
        Flappy::Accent accent() const;
    };

    // Credits: card x 290..990, y 40..680, title "CREDITS". Shows the game name "Jello Bounce" with a tiny
    // jello logo (shaded ellipse with two eyes), then centered lines:
    //   DESIGN AND PROGRAMMING   Nguyen Duong Gia Thuan
    //   BUILT WITH               C++23, SFML 3.0, nlohmann/json
    //   SOUND AND MUSIC          synthesized in code, no samples used
    //   ART                      every picture is drawn in code
    //   FONTS                    Geomanist, Corbel Light, Helvetica Neue UltraLight (their owners' property)
    //   MADE WITH THE HELP OF    Claude Code
    // Plus "Thanks for playing!" and a "Back" button. The lines slowly drift (a gentle sin wave) to feel alive.
    class CreditsPanel : public Panel
    {
    public:
        explicit CreditsPanel(const sf::Font &font);

        void onOpen() override;
        void update(float dt) override;
        void draw(sf::RenderTarget &target) const override;
        void handleMouseMove(sf::Vector2f point) override;
        void handleMouseDown(sf::Vector2f point) override;
        void handleMouseUp(sf::Vector2f point) override;

    private:
        const sf::Font *m_font;
        std::vector<Flappy::UiKit::Button3D> m_buttons; // [0] Back
        int m_pressed = -1;
        float m_time = 0.0f;
        Flappy::Accent accent() const;
    };
} // namespace Panels
