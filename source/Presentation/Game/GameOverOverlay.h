#pragma once
#include "FlappyCommon.h"
#include "UiKit.h"
#include <vector>

namespace Flappy
{
    // Game over screen: darkened scene, result card on the RIGHT half (x 640..1220, y 40..680) with a gold medal
    // on top, red extruded "GAME OVER", an optional gold "NEW BEST!" tag, a big orange score, a
    // "PREVIOUS BEST n" chip (+ green "+delta" chip on a new best), a "Play again" primary button and a
    // square home button. The left half stays free so the explosion is visible.
    class GameOverOverlay
    {
    public:
        GameOverOverlay(const sf::Font &font, const Accent &accent);

        // Call when the game ends. Restarts the intro animation (card slides/scales in, score counts up,
        // medal pops, tag wobbles).
        void show(int score, int previousBest, bool newBest);

        void update(float dt);
        void handleMouseMove(sf::Vector2f mouse);
        void handleMouseDown(sf::Vector2f mouse);
        // Restart when "Play again" is clicked, Menu when the home button is clicked, else None.
        Action handleMouseUp(sf::Vector2f mouse);

        // True once the intro is far enough along that input (Space to retry) should be accepted (~0.5 s).
        bool acceptsInput() const { return m_time > 0.5f; }

        // `fade` 0..1 multiplies the overall alpha.
        void draw(sf::RenderTarget &target, float fade = 1.0f) const;

    private:
        const sf::Font *m_font;
        Accent m_accent;
        int m_score = 0;
        int m_previousBest = 0;
        bool m_newBest = false;
        float m_time = 0.0f;
        std::vector<UiKit::Button3D> m_buttons; // [0]=Play again [1]=home
        int m_pressedIndex = -1;
    };
} // namespace Flappy
