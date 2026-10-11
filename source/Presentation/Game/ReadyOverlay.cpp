#include "ReadyOverlay.h"
#include <algorithm>
#include "UiKit.h"
#include <cmath>
#include <cstdint>

namespace Flappy
{
    namespace
    {
        constexpr float kPi = 3.14159265f;

        // Scales a 0..255 alpha by the overlay fade
        std::uint8_t fadedAlpha(float alpha, float fade)
        {
            return static_cast<std::uint8_t>(std::clamp(alpha * fade, 0.0f, 255.0f) + 0.5f);
        }

        sf::FloatRect rectAt(float x, float y, float w, float h)
        {
            return sf::FloatRect(sf::Vector2f{x, y}, sf::Vector2f{w, h});
        }

        // Pale yellow sparkles at fixed spots of the mockup (x, y, outer size, twinkle phase)
        struct SparkleSpot
        {
            float x;
            float y;
            float size;
            float phase;
        };

        constexpr SparkleSpot kSparkles[] = {
            {470.0f, 400.0f, 16.0f, 0.0f},
            {820.0f, 360.0f, 12.0f, 1.7f},
            {760.0f, 560.0f, 10.0f, 3.1f},
            {430.0f, 540.0f, 9.0f, 4.4f},
        };
    } // namespace

    ReadyOverlay::ReadyOverlay(const sf::Font &font)
        : m_font(&font)
    {
    }

    void ReadyOverlay::update(float dt)
    {
        m_time += dt;
    }

    void ReadyOverlay::draw(sf::RenderTarget &target, sf::Vector2f blobCenter, float fade) const
    {
        fade = std::clamp(fade, 0.0f, 1.0f);
        if (fade <= 0.0f)
        {
            return;
        }

        // Color helper: rgb plus a 0..255 alpha that is multiplied by the fade
        const auto tint = [fade](std::uint8_t r, std::uint8_t g, std::uint8_t b, float a = 255.0f)
        {
            return sf::Color(r, g, b, fadedAlpha(a, fade));
        };

        // 1. Sunburst behind the blob
        UiKit::drawSunburstRays(target, blobCenter, 760.0f, 24, m_time * 6.0f,
                                tint(255, 255, 255, 55.0f));

        // 2. "GET READY!" title with a gentle bounce and pulse
        const float titleY = 150.0f + std::sin(m_time * 3.0f) * 6.0f;
        const float titleScale = 1.0f + 0.03f * std::sin(m_time * 4.0f);
        UiKit::drawExtrudedText(target, *m_font, m_title, 120, sf::Vector2f{640.0f, titleY},
                                UiKit::pinkStyle(), titleScale, 0.0f, fadedAlpha(255.0f, fade));

        // 3. Bottom hint: SPACE key cap (presses down for 0.15 s every 1.2 s) and "to bounce!"
        const float pressPhase = std::fmod(m_time, 1.2f);
        float sink = 0.0f;
        if (pressPhase < 0.15f)
        {
            sink = 5.0f * std::sin(kPi * pressPhase / 0.15f);
        }

        // The key cap grows with its label; cap and action text are centered together
        sf::Text label(*m_font, m_keyLabel, 30);
        label.setLetterSpacing(6.0f);
        const float capW = std::max(240.0f, label.getLocalBounds().size.x + 80.0f);
        sf::Text text(*m_font, m_action, 38);
        const sf::FloatRect bounds = text.getLocalBounds();
        const float gap = 24.0f;
        const float startX = 640.0f - (capW + gap + bounds.size.x) / 2.0f;
        const float capCenterX = startX + capW / 2.0f;

        // Soft shadow under the cap
        UiKit::drawGlow(target, sf::Vector2f{capCenterX, 700.0f}, 110.0f, tint(5, 0, 50, 120.0f));

        // Edge slab stays put; the cap sinks onto it
        UiKit::drawRoundedRect(target, rectAt(startX, 633.0f, capW, 72.0f), 20.0f, tint(111, 134, 214));

        const float capY = 624.0f + sink;
        UiKit::drawRoundedRect(target, rectAt(startX, capY, capW, 72.0f), 20.0f, tint(122, 155, 214));
        UiKit::drawRoundedRectGradient(target, rectAt(startX + 3.0f, capY + 3.0f, capW - 6.0f, 66.0f), 17.0f,
                                       tint(255, 255, 255), tint(207, 224, 255));

        // White inner highlight along the top of the face
        UiKit::drawRoundedRect(target, rectAt(startX + 20.0f, capY + 3.0f, capW - 40.0f, 3.0f), 1.5f,
                               tint(255, 255, 255, 242.0f));

        UiKit::drawLabel(target, *m_font, m_keyLabel, 30, sf::Vector2f{capCenterX, capY + 36.0f},
                         tint(27, 31, 92), false, 6.0f);

        // Action text with a stacked navy / dark offset shadow (drawn farthest first)
        text.setOrigin(sf::Vector2f{bounds.position.x, bounds.position.y + bounds.size.y / 2.0f});

        const float textX = startX + capW + gap;
        const auto drawTextLayer = [&target, &text, textX](float dy, sf::Color color)
        {
            text.setPosition(sf::Vector2f{textX, 660.0f + dy});
            text.setFillColor(color);
            target.draw(text);
        };
        drawTextLayer(6.0f, tint(10, 15, 60));
        drawTextLayer(3.0f, tint(27, 31, 92));
        drawTextLayer(0.0f, tint(255, 255, 255));

        // 4. Twinkling sparkles
        for (const SparkleSpot &spot : kSparkles)
        {
            const float twinkle = 0.5f + 0.5f * std::sin(m_time * 2.5f + spot.phase);
            const float size = spot.size * (0.6f + 0.4f * twinkle);
            const float alpha = 255.0f * (0.45f + 0.55f * twinkle);
            UiKit::drawSparkle(target, sf::Vector2f{spot.x, spot.y}, size, tint(255, 243, 176, alpha));
        }
    }
} // namespace Flappy
