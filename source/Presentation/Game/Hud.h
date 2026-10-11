#pragma once
#include "FlappyCommon.h"
#include <string>

namespace Flappy
{
    // In-game heads-up display: big 3D score at the top center, round pause button (top-left),
    // "BEST" pill with crown (top-right) and a pink "xN STREAK" tag below it (only when streak >= 2).
    class Hud
    {
    public:
        explicit Hud(const sf::Font &font);

        void setScore(int score); // a change triggers a short "pop" (scale 1.3 -> 1.0) of the score text
        void setBest(int best);
        void setStreak(int streak);
        void setStreakWord(std::string word) { m_streakWord = std::move(word); } // "STREAK" by default, e.g. "COMBO"
        // Hearts next to the pause button (maxLives 0 hides them)
        void setLives(int lives, int maxLives)
        {
            m_lives = lives;
            m_maxLives = maxLives;
        }
        void setPlayerName(std::string name) { m_playerName = std::move(name); } // small label under the pause button
        void setPauseHovered(bool hovered) { m_pauseHovered = hovered; }

        void update(float dt);

        // Pause button hit test (logical coordinates)
        bool pauseHit(sf::Vector2f point) const;

        // `showScore` false hides the big number (Ready screen shows a dim 0 instead if you prefer) ;
        // `alpha` 0..255 fades the whole HUD (used while overlays are open).
        void draw(sf::RenderTarget &target, bool showScore = true, std::uint8_t alpha = 255) const;

    private:
        const sf::Font *m_font;
        int m_score = 0;
        int m_best = 0;
        int m_streak = 0;
        std::string m_playerName;
        std::string m_streakWord = "STREAK";
        int m_lives = 0;
        int m_maxLives = 0;
        float m_pop = 0.0f; // 1 right after the score changes, decays to 0
        float m_time = 0.0f;
        bool m_pauseHovered = false;
    };
} // namespace Flappy
