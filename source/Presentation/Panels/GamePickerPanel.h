#pragma once
#include "Panel.h"
#include "Widgets.h"
#include <array>

namespace Panels
{
    // Game picker: card x 90..1190, y 40..680, title "CHOOSE A GAME".
    // Five game cards in a row (184 x 330, gap 16, starting at x 148 / y 190). Each card has a themed gradient,
    // a small drawn illustration, the game name, its tagline and a "BEST n" chip for the current difficulty.
    // When somebody has scored, a "TOP n NAME" line sits under the chip. A status line "Playing as <profile>  -
    // <difficulty>" is drawn at the bottom. Hover lifts a card, pressed sinks it.
    // Clicking a card (or pressing keys 1..5) sets m_launchGame to its id (0..4). "Back" sets m_closeRequested.
    class GamePickerPanel : public Panel
    {
    public:
        explicit GamePickerPanel(const sf::Font &font);

        void onOpen() override;
        void update(float dt) override;
        void draw(sf::RenderTarget &target) const override;
        void handleMouseMove(sf::Vector2f point) override;
        void handleMouseDown(sf::Vector2f point) override;
        void handleMouseUp(sf::Vector2f point) override;
        void handleKey(sf::Keyboard::Key key) override;

    private:
        const sf::Font *m_font;
        Flappy::UiKit::Button3D m_back;
        std::array<float, 5> m_lift{}; // 0..1 hover amount per card
        std::array<float, 5> m_sink{}; // 0..1 pressed amount per card
        int m_hoverCard = -1;
        int m_pressedCard = -1;
        bool m_backPressed = false;
        float m_time = 0.0f;
    };
} // namespace Panels
