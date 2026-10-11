#include "PauseOverlay.h"
#include <cmath>
#include <string>

namespace Flappy
{
    namespace
    {
        std::uint8_t scaleAlpha(std::uint8_t a, float fade)
        {
            return static_cast<std::uint8_t>(std::clamp(a * fade + 0.5f, 0.0f, 255.0f));
        }

        sf::Color fadeColor(sf::Color c, float fade)
        {
            return withAlpha(c, scaleAlpha(c.a, fade));
        }

        // Width of a text line, used to lay out the SCORE / BEST chip contents
        float measureText(const sf::Font &font, const std::string &text, unsigned int size, float spacing)
        {
            sf::Text t(font, text, size);
            t.setLetterSpacing(spacing);
            return t.getLocalBounds().size.x;
        }

        // Rounded chip with "LABEL value" centered inside
        void drawChip(sf::RenderTarget &target, const sf::Font &font, sf::FloatRect rect, const std::string &label,
                      const std::string &value, sf::Color valueColor, float fade)
        {
            UiKit::drawRoundedRect(target, rect, 23.0f, fadeColor(sf::Color(14, 8, 60, 180), fade),
                                   fadeColor(sf::Color(255, 255, 255, 90), fade), 2.0f);

            const float gap = 10.0f;
            const float labelW = measureText(font, label, 14, 2.0f);
            const float valueW = measureText(font, value, 26, 1.0f);
            const float startX = rect.position.x + rect.size.x * 0.5f - (labelW + gap + valueW) * 0.5f;
            const float centerY = rect.position.y + rect.size.y * 0.5f;

            UiKit::drawLabel(target, font, label, 14, {startX + labelW * 0.5f, centerY},
                             fadeColor(sf::Color(207, 216, 255), fade), false, 2.0f);
            UiKit::drawLabel(target, font, value, 26, {startX + labelW + gap + valueW * 0.5f, centerY},
                             fadeColor(valueColor, fade), false, 1.0f);
        }
    } // namespace

    PauseOverlay::PauseOverlay(const sf::Font &font, const Accent &accent)
        : m_font(&font)
    {
        m_buttons.emplace_back(font, "Resume", sf::FloatRect({450.0f, 314.0f}, {380.0f, 76.0f}),
                               UiKit::Button3DStyle::primary(accent), 32);
        m_buttons.emplace_back(font, "Restart", sf::FloatRect({450.0f, 412.0f}, {380.0f, 68.0f}),
                               UiKit::Button3DStyle::secondary(), 28);
        m_buttons.emplace_back(font, "Main menu", sf::FloatRect({450.0f, 502.0f}, {380.0f, 68.0f}),
                               UiKit::Button3DStyle::secondary(), 28);
    }

    void PauseOverlay::setScores(int score, int best)
    {
        m_score = score;
        m_best = best;
    }

    void PauseOverlay::update(float dt)
    {
        m_time += dt;
        for (auto &button : m_buttons)
        {
            button.update(dt);
        }
    }

    void PauseOverlay::handleMouseMove(sf::Vector2f mouse)
    {
        for (auto &button : m_buttons)
        {
            button.setHovered(button.contains(mouse));
        }
    }

    void PauseOverlay::handleMouseDown(sf::Vector2f mouse)
    {
        m_pressedIndex = -1;
        for (int i = 0; i < static_cast<int>(m_buttons.size()); ++i)
        {
            m_buttons[i].setPressed(false);
            if (m_pressedIndex < 0 && m_buttons[i].contains(mouse))
            {
                m_pressedIndex = i;
            }
        }
        if (m_pressedIndex >= 0)
        {
            m_buttons[m_pressedIndex].setPressed(true);
        }
    }

    Action PauseOverlay::handleMouseUp(sf::Vector2f mouse)
    {
        for (auto &button : m_buttons)
        {
            button.setPressed(false);
        }

        static constexpr Action kActions[] = {Action::Resume, Action::Restart, Action::Menu};

        Action result = Action::None;
        if (m_pressedIndex >= 0 && m_pressedIndex < static_cast<int>(m_buttons.size()) &&
            m_buttons[m_pressedIndex].contains(mouse))
        {
            result = kActions[m_pressedIndex];
        }
        m_pressedIndex = -1;
        return result;
    }

    void PauseOverlay::draw(sf::RenderTarget &target, float fade) const
    {
        fade = std::clamp(fade, 0.0f, 1.0f);
        if (fade <= 0.0f)
        {
            return;
        }
        const sf::Font &font = *m_font;

        // Dim the scene behind the menu
        UiKit::drawRoundedRect(target, sf::FloatRect({0.0f, 0.0f}, {kWidth, kHeight}), 0.0f,
                               fadeColor(sf::Color(14, 6, 60, 158), fade));

        // Soft drop shadow under the panel (kept faint, the panel is translucent)
        for (int i = 0; i < 3; ++i)
        {
            const float e = 24.0f - i * 10.0f;
            UiKit::drawRoundedRect(target, sf::FloatRect({390.0f - e, 126.0f - e}, {500.0f + e * 2.0f, 528.0f + e * 2.0f}),
                                   44.0f + e, fadeColor(sf::Color(5, 0, 40, 12), fade));
        }

        // Dark edge slab underneath the panel (3D thickness)
        UiKit::drawRoundedRect(target, sf::FloatRect({390.0f, 110.0f}, {500.0f, 528.0f}), 44.0f,
                               fadeColor(sf::Color(14, 6, 70, 180), fade));

        // Frosted panel
        UiKit::drawRoundedRect(target, sf::FloatRect({390.0f, 96.0f}, {500.0f, 528.0f}), 44.0f,
                               fadeColor(sf::Color(255, 255, 255, 36), fade),
                               fadeColor(sf::Color(255, 255, 255, 128), fade), 3.0f);

        // Thin highlight along the top inside edge
        UiKit::drawRoundedRect(target, sf::FloatRect({430.0f, 100.0f}, {420.0f, 3.0f}), 1.5f,
                               fadeColor(sf::Color(255, 255, 255, 140), fade));

        // Bobbing extruded title
        const float bob = std::sin(m_time * 2.0f) * 3.0f;
        UiKit::drawExtrudedText(target, font, "PAUSED", 84, {640.0f, 168.0f + bob}, UiKit::blueStyle(), 1.0f,
                                0.0f, scaleAlpha(255, fade));

        // Score chips
        drawChip(target, font, sf::FloatRect({470.0f, 236.0f}, {170.0f, 46.0f}), "SCORE", std::to_string(m_score),
                 sf::Color(255, 255, 255), fade);
        drawChip(target, font, sf::FloatRect({654.0f, 236.0f}, {156.0f, 46.0f}), "BEST", std::to_string(m_best),
                 sf::Color(255, 243, 176), fade);

        // Buttons (Button3D has no alpha control, so they are drawn at full opacity)
        for (const auto &button : m_buttons)
        {
            button.draw(target);
        }

        UiKit::drawLabel(target, font, "Press ESC to resume", 18, {640.0f, 603.0f},
                         fadeColor(sf::Color(214, 224, 255), fade), false, 1.0f);
    }
} // namespace Flappy
