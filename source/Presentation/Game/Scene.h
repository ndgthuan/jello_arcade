#pragma once
#include "FlappyCommon.h"
#include <vector>

namespace Flappy
{
    // The whole backdrop: twilight sky, stars, big sun, glowing clouds, two parallax hill layers and the
    // perspective neon-grid floor. Everything is drawn in logical 1280x720 coordinates.
    class Scene
    {
    public:
        Scene();

        // `scrollSpeed` is the world speed in px/s (0 = frozen world). Clouds/hills/floor move proportionally
        // slower the further away they are (parallax); stars twinkle with time regardless of speed.
        void update(float dt, float scrollSpeed);

        // Sky gradient, stars, sun glow + sun, clouds, hills, floor with moving grid lines.
        // Must not draw anything below the floor top that would hide the pipes (pipes are drawn after this).
        void draw(sf::RenderTarget &target) const;

        // Dark-violet vignette over the whole screen (draw last, before HUD). `strength` 0..1.
        void drawVignette(sf::RenderTarget &target, float strength = 1.0f) const;

    private:
        struct Star
        {
            sf::Vector2f pos;
            float radius;
            float phase;
        };
        struct Cloud
        {
            float x, y, w, h;
        };
        std::vector<Star> m_stars;
        std::vector<Cloud> m_clouds;
        float m_time = 0.0f;
        float m_farOffset = 0.0f;   // far hills scroll offset
        float m_nearOffset = 0.0f;  // near hills scroll offset
        float m_cloudOffset = 0.0f;
        float m_gridOffset = 0.0f;  // floor grid scroll (0..1 of one cell)
    };
} // namespace Flappy
