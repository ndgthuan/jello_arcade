#pragma once
#include "FlappyCommon.h"
#include <string>

namespace Flappy
{
    // "Get ready" screen content (no dimming): sunburst rays behind the blob, a big pink extruded
    // "GET READY!" title with a gentle bounce, and a 3D SPACE key cap + "to bounce!" text at the bottom.
    class ReadyOverlay
    {
    public:
        explicit ReadyOverlay(const sf::Font &font);

        // Texts of the screen (defaults are for Jello Bounce): the big title, the key cap label and the action
        void setTexts(std::string title, std::string keyLabel, std::string action)
        {
            m_title = std::move(title);
            m_keyLabel = std::move(keyLabel);
            m_action = std::move(action);
        }

        void update(float dt);

        // `blobCenter` is where the sunburst rays are centered; `fade` 0..1 (1 = fully visible) so the game can
        // fade the overlay out when the first flap happens.
        void draw(sf::RenderTarget &target, sf::Vector2f blobCenter, float fade = 1.0f) const;

    private:
        const sf::Font *m_font;
        float m_time = 0.0f;
        std::string m_title = "GET READY!";
        std::string m_keyLabel = "SPACE";
        std::string m_action = "to bounce!";
    };
} // namespace Flappy
