#pragma once
#include "FlappyCommon.h"
#include "UiKit.h"
#include <vector>

namespace Flappy
{
    // Pause menu: dark blurred-looking dim over the whole screen, a frosted 3D panel in the middle with a blue
    // extruded "PAUSED" title, SCORE / BEST chips and three chunky buttons (Resume = primary, Restart, Main menu).
    class PauseOverlay
    {
    public:
        PauseOverlay(const sf::Font &font, const Accent &accent);

        void setScores(int score, int best);

        void update(float dt);
        void handleMouseMove(sf::Vector2f mouse);
        void handleMouseDown(sf::Vector2f mouse); // sets the pressed look
        // Returns Resume / Restart / Menu when a button was clicked (mouse released inside it), else None.
        Action handleMouseUp(sf::Vector2f mouse);

        // `fade` 0..1 for fade-in/out of the whole overlay.
        void draw(sf::RenderTarget &target, float fade = 1.0f) const;

    private:
        const sf::Font *m_font;
        int m_score = 0;
        int m_best = 0;
        float m_time = 0.0f;
        std::vector<UiKit::Button3D> m_buttons; // [0]=Resume [1]=Restart [2]=Main menu
        int m_pressedIndex = -1;
    };
} // namespace Flappy
