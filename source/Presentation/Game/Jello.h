#pragma once
#include "FlappyCommon.h"
#include <deque>

namespace Flappy
{
    // The player: a glossy jello blob with squash & stretch.
    class Jello
    {
    public:
        explicit Jello(const Accent &accent = Accent::from(sf::Color(124, 255, 91)));

        void setAccent(const Accent &accent) { m_accent = accent; }

        // Back to the start pose at x = kPlayerX, alive, no velocity.
        void reset(float y = kHeight * 0.42f);

        // Upward impulse (velocity = kFlapSpeed). Also triggers a stretch animation.
        void flap();

        // physicsActive == true : gravity + movement + squash/stretch + tilt from velocity.
        // physicsActive == false: gentle idle bobbing around the reset height (Ready screen).
        // Must keep animating (wobble) even when dead.
        void update(float dt, bool physicsActive);

        // Dead pose: X eyes, squashed, tilted, stops moving (velocity zeroed, no more gravity in update()).
        void kill();
        bool isDead() const { return m_dead; }

        sf::Vector2f position() const { return m_pos; } // center of the body
        float velocityY() const { return m_velY; }

        // Collision box centered on position(), size 2*kPlayerHalfW x 2*kPlayerHalfH
        sf::FloatRect hitbox() const;

        // Returns true exactly once after each flap() (so the game can spawn an effect), then resets.
        bool consumeFlapEvent();

        // Soft elliptical shadow on the ground (shrinks and fades as the blob goes higher). Draw before the body.
        void drawShadow(sf::RenderTarget &target) const;
        // Faint ghost copies / glow behind the blob (from recent positions). Draw before the body.
        void drawTrail(sf::RenderTarget &target) const;
        // The shaded body, highlights, eyes, mouth, blush. Squashed with X eyes when dead.
        void draw(sf::RenderTarget &target) const;

    private:
        Accent m_accent;
        sf::Vector2f m_pos{kPlayerX, kHeight * 0.42f};
        float m_baseY = kHeight * 0.42f;
        float m_velY = 0.0f;
        float m_time = 0.0f;
        bool m_dead = false;
        bool m_flapEvent = false;
        float m_stretch = 0.0f;     // spring-driven: + = tall/thin, - = squashed
        float m_stretchVel = 0.0f;
        float m_tilt = 0.0f;        // degrees, smoothed
        std::deque<sf::Vector2f> m_history; // recent centers for the trail
        float m_historyTimer = 0.0f;
    };
} // namespace Flappy
