#pragma once
#include "FlappyCommon.h"
#include <vector>

namespace Flappy
{
    struct Pipe
    {
        float x = 0.0f;          // left edge of the pipe body (the rim extends (kRimWidth-kPipeWidth)/2 on each side)
        float gapCenterY = 0.0f; // vertical center of the opening
        bool scored = false;
    };

    // All pipes on screen: spawning, scrolling, collision, scoring and drawing (glossy cylinders with rims).
    class PipeField
    {
    public:
        PipeField();

        // Clears pipes; the first pipe appears at x = kWidth + 200 and the rest follow every kPipeSpacing.
        // `seed` makes the gap heights reproducible (0 = random).
        void reset(unsigned int seed = 0);

        // Scrolls left by kPipeSpeed * dt (only when scrolling is enabled), removes pipes that left the
        // screen, spawns new ones on the right so there are always pipes up to x = kWidth + kPipeSpacing.
        // Gap center is random in [kGapMargin + kGapSize/2, kGroundY - kGapMargin - kGapSize/2] and never
        // differs more than 260 px from the previous pipe's gap center (so it stays playable).
        void update(float dt);

        // Gap size and scroll speed for a difficulty (0 easy, 1 normal, 2 hard); call before reset()
        void setDifficulty(int difficulty);
        float speed() const { return m_speed; }
        float gapSize() const { return m_gapSize; }

        void setScrolling(bool scrolling) { m_scrolling = scrolling; }
        bool isScrolling() const { return m_scrolling; }

        // True if the box overlaps any pipe body or rim.
        bool collides(const sf::FloatRect &box) const;

        // Marks pipes whose right edge passed `playerX` as scored and returns how many were newly scored.
        int collectScore(float playerX);

        // Center of the gap of the pipe most recently counted by collectScore (x = pipe center, y = gap center).
        sf::Vector2f lastScoredGapCenter() const { return m_lastScored; }
        // Gap center y of the pipe the player is currently approaching (first not-yet-scored pipe), or kHeight/2.
        float nextGapCenterY() const;

        const std::vector<Pipe> &pipes() const { return m_pipes; }

        // Draw all pipes. Style (see design): cylinder shading across the width
        //   (dark blue -> blue -> cyan -> near-white highlight at ~43% -> cyan -> blue -> dark blue),
        // 3px dark navy outline on the sides, rim wider than the body with rounded corners, inner top highlight and a
        // drop shadow under each rim, and a soft elliptical contact shadow on the ground under the bottom pipe.
        // Pipes already scored are drawn slightly darker (about 18% darker).
        void draw(sf::RenderTarget &target) const;

    private:
        std::vector<Pipe> m_pipes;
        bool m_scrolling = true;
        float m_gapSize = kGapSize;
        float m_speed = kPipeSpeed;
        sf::Vector2f m_lastScored{0.0f, 0.0f};
        unsigned int m_rngState = 1u;
        float nextRandom01();
        void spawn(float x);
    };
} // namespace Flappy
