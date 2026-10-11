#include "Widgets.h"
#include <algorithm>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <string>

namespace Panels
{
    namespace UiKit = Flappy::UiKit;

    namespace
    {
        std::uint8_t scaleAlpha(std::uint8_t a, float fade)
        {
            return static_cast<std::uint8_t>(std::clamp(a * fade + 0.5f, 0.0f, 255.0f));
        }

        sf::Color fadeColor(sf::Color c, float fade)
        {
            return Flappy::withAlpha(c, scaleAlpha(c.a, fade));
        }

        float easeOutCubic(float t)
        {
            const float u = 1.0f - t;
            return 1.0f - u * u * u;
        }

        // Single line of text, vertically centered on pos.y. rightAlign anchors the right edge at pos.x
        void drawLine(sf::RenderTarget &target, const sf::Font &font, const std::string &text, unsigned int size,
                      sf::Vector2f pos, sf::Color color, bool rightAlign, bool shadow)
        {
            sf::Text t(font, text, size);
            const sf::FloatRect b = t.getLocalBounds();
            const float ox = rightAlign ? b.position.x + b.size.x : b.position.x;
            t.setOrigin({ox, b.position.y + b.size.y * 0.5f});

            if (shadow)
            {
                t.setFillColor(sf::Color(0, 20, 30, 100));
                t.setPosition(pos + sf::Vector2f(0.0f, 2.0f));
                target.draw(t);
                t.setPosition(pos + sf::Vector2f(0.0f, 4.0f));
                target.draw(t);
            }
            t.setFillColor(color);
            t.setPosition(pos);
            target.draw(t);
        }

        void fillCircle(sf::RenderTarget &target, sf::Vector2f center, float radius, sf::Color color)
        {
            sf::CircleShape shape(radius, 32);
            shape.setOrigin({radius, radius});
            shape.setPosition(center);
            shape.setFillColor(color);
            target.draw(shape);
        }

        // Thick line segment as a rotated rectangle with round caps
        void drawSegment(sf::RenderTarget &target, sf::Vector2f a, sf::Vector2f b, float thickness, sf::Color color)
        {
            const sf::Vector2f d = b - a;
            const float len = std::sqrt(d.x * d.x + d.y * d.y);

            sf::RectangleShape bar(sf::Vector2f(len, thickness));
            bar.setOrigin({0.0f, thickness * 0.5f});
            bar.setPosition(a);
            bar.setRotation(sf::radians(std::atan2(d.y, d.x)));
            bar.setFillColor(color);
            target.draw(bar);

            fillCircle(target, a, thickness * 0.5f, color);
            fillCircle(target, b, thickness * 0.5f, color);
        }

        bool inCircle(sf::Vector2f p, sf::Vector2f c, float radius)
        {
            const float dx = p.x - c.x;
            const float dy = p.y - c.y;
            return dx * dx + dy * dy <= radius * radius;
        }

        // Hit area of the toggle pill and its ON / OFF label, for a row starting at rect
        sf::FloatRect toggleHitArea(sf::FloatRect rect)
        {
            const float cy = rect.position.y + rect.size.y * 0.5f;
            return sf::FloatRect(sf::Vector2f(rect.position.x + 246.0f, cy - 26.0f), sf::Vector2f(180.0f, 52.0f));
        }
    } // namespace

    // ---- Card and text ----

    void drawCard(sf::RenderTarget &target, const sf::Font &font, sf::FloatRect rect, const std::string &title,
                  float open)
    {
        open = std::clamp(open, 0.0f, 1.0f);
        if (open <= 0.0f)
        {
            return;
        }

        // Eased fade-in, and a scale from 0.92 to 1.0 around the card center
        const float fade = easeOutCubic(open);
        const float s = 0.92f + 0.08f * fade;
        const sf::Vector2f center = rect.position + rect.size * 0.5f;
        const sf::Vector2f size = rect.size * s;
        const sf::Vector2f pos = center - size * 0.5f;
        const float radius = 44.0f * s;

        // Soft shadow: three faint expanded rounded rects, pushed down
        for (int i = 0; i < 3; ++i)
        {
            const float e = (24.0f - i * 10.0f) * s;
            UiKit::drawRoundedRect(target,
                                   sf::FloatRect(sf::Vector2f(pos.x - e, pos.y - e + 24.0f * s),
                                                 sf::Vector2f(size.x + e * 2.0f, size.y + e * 2.0f)),
                                   radius + e, fadeColor(sf::Color(5, 0, 40, 12), fade));
        }

        // Dark edge slab under the card (3D thickness)
        UiKit::drawRoundedRect(target, sf::FloatRect(sf::Vector2f(pos.x, pos.y + 14.0f * s), size), radius,
                               fadeColor(sf::Color(14, 6, 70, 180), fade));

        // Frosted card
        UiKit::drawRoundedRect(target, sf::FloatRect(pos, size), radius, fadeColor(sf::Color(255, 255, 255, 36), fade),
                               fadeColor(sf::Color(255, 255, 255, 128), fade), 3.0f);

        // Thin highlight along the top inside edge
        UiKit::drawRoundedRect(target,
                               sf::FloatRect(sf::Vector2f(pos.x + 40.0f * s, pos.y + 4.0f * s),
                                             sf::Vector2f(size.x - 80.0f * s, 3.0f)),
                               1.5f, fadeColor(sf::Color(255, 255, 255, 140), fade));

        // Extruded title, scaled together with the card
        const sf::Vector2f titleAnchor(rect.position.x + rect.size.x * 0.5f, rect.position.y + 72.0f);
        const sf::Vector2f titleCenter = center + (titleAnchor - center) * s;
        UiKit::drawExtrudedText(target, font, title, 64, titleCenter, UiKit::blueStyle(), s, 0.0f,
                                scaleAlpha(255, fade));
    }

    void drawText(sf::RenderTarget &target, const sf::Font &font, const std::string &text, unsigned int size,
                  sf::Vector2f pos, sf::Color color, bool shadow)
    {
        drawLine(target, font, text, size, pos, color, false, shadow);
    }

    // ---- Slider ----

    Slider::Slider(const sf::Font &font, std::string label, sf::FloatRect rect, int value)
        : m_font(&font), m_label(std::move(label)), m_rect(rect), m_value(0)
    {
        setValue(value);
    }

    void Slider::setValue(int value)
    {
        m_value = std::clamp(value, 0, 100);
    }

    int Slider::valueAt(float x) const
    {
        const float left = trackLeft();
        const float right = trackRight();
        const float t = (x - left) / (right - left);
        return std::clamp(static_cast<int>(std::round(t * 100.0f)), 0, 100);
    }

    bool Slider::handleMouseDown(sf::Vector2f point)
    {
        const float cy = m_rect.position.y + m_rect.size.y * 0.5f;
        const float left = trackLeft();
        const float right = trackRight();
        const float knobX = left + (right - left) * m_value / 100.0f;

        const bool inBand = point.x >= left - 12.0f && point.x <= right + 12.0f && std::abs(point.y - cy) <= 24.0f;
        const bool onKnob = inCircle(point, {knobX, cy}, 24.0f);
        if (!inBand && !onKnob)
        {
            return false;
        }

        m_dragging = true;
        const int v = valueAt(point.x);
        if (v == m_value)
        {
            return false;
        }
        m_value = v;
        return true;
    }

    bool Slider::handleMouseMove(sf::Vector2f point)
    {
        const float cy = m_rect.position.y + m_rect.size.y * 0.5f;
        const float knobX = trackLeft() + (trackRight() - trackLeft()) * m_value / 100.0f;
        m_hovered = inCircle(point, {knobX, cy}, 24.0f);

        if (!m_dragging)
        {
            return false;
        }
        const int v = valueAt(point.x);
        if (v == m_value)
        {
            return false;
        }
        m_value = v;
        return true;
    }

    void Slider::handleMouseUp()
    {
        m_dragging = false;
    }

    void Slider::draw(sf::RenderTarget &target, const Flappy::Accent &accent) const
    {
        const float cy = m_rect.position.y + m_rect.size.y * 0.5f;
        drawLine(target, *m_font, m_label, 26, {m_rect.position.x, cy}, sf::Color::White, false, true);

        // Dark track with a white border
        const float left = trackLeft();
        const float right = trackRight();
        const float width = right - left;
        UiKit::drawRoundedRect(target, sf::FloatRect(sf::Vector2f(left, cy - 6.0f), sf::Vector2f(width, 12.0f)), 6.0f,
                               sf::Color(14, 8, 60, 200), sf::Color(255, 255, 255, 90), 2.0f);

        // Filled part left of the knob
        const float knobX = left + width * m_value / 100.0f;
        const float fillW = knobX - left - 2.0f;
        if (fillW > 4.0f)
        {
            UiKit::drawRoundedRectGradient(target, sf::FloatRect(sf::Vector2f(left + 2.0f, cy - 4.0f), sf::Vector2f(fillW, 8.0f)),
                                           4.0f, accent.light, accent.base);
        }

        // Knob: 3D lower edge, light-blue lower half, white upper half, small highlight
        const bool active = m_hovered || m_dragging;
        const float r = active ? 20.0f : 17.0f;
        fillCircle(target, {knobX, cy + 4.0f}, r, sf::Color(111, 134, 214));
        fillCircle(target, {knobX, cy}, r, sf::Color(207, 224, 255));
        fillCircle(target, {knobX, cy - r * 0.2f}, r * 0.8f, sf::Color(236, 243, 255));
        fillCircle(target, {knobX - r * 0.35f, cy - r * 0.45f}, r * 0.22f, sf::Color::White);

        // Value text, right aligned at the row end
        drawLine(target, *m_font, std::to_string(m_value), 26, {m_rect.position.x + m_rect.size.x, cy},
                 sf::Color(255, 243, 176), true, true);
    }

    // ---- Segmented ----

    Segmented::Segmented(const sf::Font &font, std::string label, sf::FloatRect rect, std::vector<std::string> options,
                         int selected)
        : m_font(&font), m_label(std::move(label)), m_rect(rect), m_options(std::move(options)), m_selected(0)
    {
        setSelected(selected);
    }

    void Segmented::setSelected(int index)
    {
        m_selected = std::clamp(index, 0, std::max(0, static_cast<int>(m_options.size()) - 1));
    }

    sf::FloatRect Segmented::optionRect(int index) const
    {
        const float left = m_rect.position.x + 250.0f;
        const float width = m_rect.position.x + m_rect.size.x - left;
        const float count = static_cast<float>(std::max<std::size_t>(1, m_options.size()));
        const float slot = width / count;
        const float gap = 8.0f;
        const float cy = m_rect.position.y + m_rect.size.y * 0.5f;
        return sf::FloatRect(sf::Vector2f(left + slot * index + gap * 0.5f, cy - 24.0f),
                             sf::Vector2f(slot - gap, 48.0f));
    }

    bool Segmented::handleMouseDown(sf::Vector2f point)
    {
        for (int i = 0; i < static_cast<int>(m_options.size()); ++i)
        {
            if (optionRect(i).contains(point))
            {
                if (i == m_selected)
                {
                    return false;
                }
                m_selected = i;
                return true;
            }
        }
        return false;
    }

    void Segmented::handleMouseMove(sf::Vector2f point)
    {
        m_hover = -1;
        for (int i = 0; i < static_cast<int>(m_options.size()); ++i)
        {
            if (optionRect(i).contains(point))
            {
                m_hover = i;
            }
        }
    }

    void Segmented::draw(sf::RenderTarget &target, const Flappy::Accent &accent) const
    {
        const float cy = m_rect.position.y + m_rect.size.y * 0.5f;
        drawLine(target, *m_font, m_label, 26, {m_rect.position.x, cy}, sf::Color::White, false, true);

        const int count = static_cast<int>(m_options.size());
        for (int i = 0; i < count; ++i)
        {
            const std::string &option = m_options[static_cast<std::size_t>(i)];
            const sf::FloatRect r = optionRect(i);
            const sf::Vector2f center = r.position + r.size * 0.5f;
            const bool hovered = i == m_hover;

            if (i == m_selected)
            {
                // Candy pill: dark slab below, dark border, accent gradient face
                UiKit::drawRoundedRect(target, sf::FloatRect(sf::Vector2f(r.position.x, r.position.y + 6.0f), r.size),
                                       24.0f, accent.dark);
                UiKit::drawRoundedRect(target, r, 24.0f, accent.dark);

                const sf::Color top = hovered ? Flappy::mix(accent.light, sf::Color::White, 0.25f) : accent.light;
                const sf::Color bottom = hovered ? Flappy::mix(accent.base, sf::Color::White, 0.15f) : accent.base;
                UiKit::drawRoundedRectGradient(target,
                                               sf::FloatRect(r.position + sf::Vector2f(3.0f, 3.0f),
                                                             r.size - sf::Vector2f(6.0f, 6.0f)),
                                               21.0f, top, bottom);

                UiKit::drawLabel(target, *m_font, option, 24, center, sf::Color(26, 10, 58), false, 1.0f);
            }
            else
            {
                const sf::Color fill = hovered ? sf::Color(34, 22, 96, 190) : sf::Color(14, 8, 60, 170);
                const sf::Color border = sf::Color(255, 255, 255, hovered ? 150 : 90);
                UiKit::drawRoundedRect(target, r, 24.0f, fill, border, 2.0f);
                UiKit::drawLabel(target, *m_font, option, 24, center, sf::Color::White, true, 1.0f);
            }
        }
    }

    // ---- SwatchRow ----

    SwatchRow::SwatchRow(const sf::Font &font, std::string label, sf::FloatRect rect, std::vector<sf::Color> colors,
                         int selected)
        : m_font(&font), m_label(std::move(label)), m_rect(rect), m_colors(std::move(colors)), m_selected(0)
    {
        setSelected(selected);
    }

    void SwatchRow::setSelected(int index)
    {
        m_selected = std::clamp(index, 0, std::max(0, static_cast<int>(m_colors.size()) - 1));
    }

    sf::Vector2f SwatchRow::centerOf(int index) const
    {
        return {m_rect.position.x + 250.0f + 23.0f + index * 66.0f, m_rect.position.y + m_rect.size.y * 0.5f};
    }

    bool SwatchRow::handleMouseDown(sf::Vector2f point)
    {
        for (int i = 0; i < static_cast<int>(m_colors.size()); ++i)
        {
            if (inCircle(point, centerOf(i), 24.0f))
            {
                if (i == m_selected)
                {
                    return false;
                }
                m_selected = i;
                return true;
            }
        }
        return false;
    }

    void SwatchRow::handleMouseMove(sf::Vector2f point)
    {
        m_hover = -1;
        for (int i = 0; i < static_cast<int>(m_colors.size()); ++i)
        {
            if (inCircle(point, centerOf(i), 24.0f))
            {
                m_hover = i;
            }
        }
    }

    void SwatchRow::draw(sf::RenderTarget &target) const
    {
        const float cy = m_rect.position.y + m_rect.size.y * 0.5f;
        drawLine(target, *m_font, m_label, 26, {m_rect.position.x, cy}, sf::Color::White, false, true);

        const int count = static_cast<int>(m_colors.size());
        for (int i = 0; i < count; ++i)
        {
            const sf::Color color = m_colors[static_cast<std::size_t>(i)];
            const sf::Vector2f c = centerOf(i);
            const float s = (i == m_hover) ? 1.12f : 1.0f;
            const float r = 23.0f * s;

            UiKit::drawShadedEllipse(target, c, {r, r}, Flappy::mix(color, sf::Color::White, 0.6f), color,
                                     Flappy::mix(color, sf::Color(20, 10, 60), 0.55f));

            if (i == m_selected)
            {
                UiKit::drawRing(target, c, 31.0f * s, 4.0f, sf::Color::White);

                // Check mark
                const float t = 5.0f * s;
                const sf::Vector2f p1 = c + sf::Vector2f(-9.0f, 1.0f) * s;
                const sf::Vector2f p2 = c + sf::Vector2f(-3.0f, 7.0f) * s;
                const sf::Vector2f p3 = c + sf::Vector2f(10.0f, -7.0f) * s;
                drawSegment(target, p1, p2, t, sf::Color::White);
                drawSegment(target, p2, p3, t, sf::Color::White);
            }
        }
    }

    // ---- Toggle ----

    Toggle::Toggle(const sf::Font &font, std::string label, sf::FloatRect rect, bool on)
        : m_font(&font), m_label(std::move(label)), m_rect(rect), m_on(on), m_anim(on ? 1.0f : 0.0f)
    {
    }

    bool Toggle::handleMouseDown(sf::Vector2f point)
    {
        if (!toggleHitArea(m_rect).contains(point))
        {
            return false;
        }
        m_on = !m_on;
        return true;
    }

    void Toggle::handleMouseMove(sf::Vector2f point)
    {
        m_hovered = toggleHitArea(m_rect).contains(point);
    }

    void Toggle::update(float dt)
    {
        const float target = m_on ? 1.0f : 0.0f;
        m_anim += (target - m_anim) * std::min(1.0f, dt * 14.0f);
        m_anim = std::clamp(m_anim, 0.0f, 1.0f);
    }

    void Toggle::draw(sf::RenderTarget &target, const Flappy::Accent &accent) const
    {
        const float cy = m_rect.position.y + m_rect.size.y * 0.5f;
        const float x = m_rect.position.x + 250.0f;
        drawLine(target, *m_font, m_label, 26, {m_rect.position.x, cy}, sf::Color::White, false, true);

        // Pill: dark base, accent gradient fades in with m_anim
        const sf::FloatRect pill(sf::Vector2f(x, cy - 21.0f), sf::Vector2f(84.0f, 42.0f));
        UiKit::drawRoundedRect(target, pill, 21.0f, sf::Color(14, 8, 60, 200),
                               sf::Color(255, 255, 255, m_hovered ? 150 : 90), 2.0f);
        if (m_anim > 0.001f)
        {
            const auto a = static_cast<std::uint8_t>(std::clamp(m_anim * 255.0f, 0.0f, 255.0f));
            UiKit::drawRoundedRectGradient(target, pill, 21.0f, Flappy::withAlpha(accent.light, a),
                                           Flappy::withAlpha(accent.base, a));
        }

        // Knob slides from the left end to the right end
        const float knobX = x + 21.0f + 42.0f * m_anim;
        fillCircle(target, {knobX, cy + 3.0f}, 16.0f, sf::Color(90, 80, 160, 200));
        fillCircle(target, {knobX, cy}, 16.0f, sf::Color::White);

        drawLine(target, *m_font, m_on ? "ON" : "OFF", 22, {x + 100.0f, cy}, sf::Color::White, false, true);
    }

    // ---- TextField ----

    TextField::TextField(const sf::Font &font, std::string placeholder, sf::FloatRect rect, std::size_t maxLength)
        : m_font(&font), m_placeholder(std::move(placeholder)), m_rect(rect), m_maxLength(maxLength)
    {
    }

    void TextField::handleMouseDown(sf::Vector2f point)
    {
        const bool wasFocused = m_focused;
        m_focused = m_rect.contains(point);
        if (m_focused && !wasFocused)
        {
            m_time = 0.0f; // show the caret right after focusing
        }
    }

    void TextField::handleText(char32_t unicode)
    {
        if (!m_focused || unicode < 32 || unicode > 126 || m_text.size() >= m_maxLength)
        {
            return;
        }
        m_text += static_cast<char>(unicode);
    }

    void TextField::handleKey(sf::Keyboard::Key key)
    {
        if (!m_focused)
        {
            return;
        }
        if (key == sf::Keyboard::Key::Backspace)
        {
            if (!m_text.empty())
            {
                m_text.pop_back();
            }
        }
        else if (key == sf::Keyboard::Key::Enter)
        {
            m_submit = true;
        }
    }

    bool TextField::consumeSubmit()
    {
        const bool submit = m_submit;
        m_submit = false;
        return submit;
    }

    void TextField::update(float dt)
    {
        m_time += dt;
    }

    void TextField::draw(sf::RenderTarget &target, const Flappy::Accent &accent) const
    {
        const float cy = m_rect.position.y + m_rect.size.y * 0.5f;

        // Faint accent glow around the focused field
        if (m_focused)
        {
            for (int i = 0; i < 3; ++i)
            {
                const float e = 10.0f - i * 3.5f;
                UiKit::drawRoundedRect(target,
                                       sf::FloatRect(m_rect.position - sf::Vector2f(e, e),
                                                     m_rect.size + sf::Vector2f(e * 2.0f, e * 2.0f)),
                                       18.0f + e, Flappy::withAlpha(accent.base, 26));
            }
        }

        if (m_focused)
        {
            UiKit::drawRoundedRect(target, m_rect, 18.0f, sf::Color(14, 8, 60, 210), accent.base, 3.0f);
        }
        else
        {
            UiKit::drawRoundedRect(target, m_rect, 18.0f, sf::Color(14, 8, 60, 210), sf::Color(255, 255, 255, 90), 2.0f);
        }

        const sf::Vector2f textPos(m_rect.position.x + 18.0f, cy);
        if (m_text.empty())
        {
            drawLine(target, *m_font, m_placeholder, 26, textPos, sf::Color(255, 255, 255, 110), false, false);
        }
        else
        {
            drawLine(target, *m_font, m_text, 26, textPos, sf::Color::White, false, true);
        }

        // Blinking caret after the text
        if (m_focused && std::fmod(m_time, 1.0f) < 0.5f)
        {
            sf::Text t(*m_font, m_text, 26);
            const float ox = t.getLocalBounds().position.x;
            const float caretX = textPos.x + t.findCharacterPos(m_text.size()).x - ox;

            sf::RectangleShape caret(sf::Vector2f(3.0f, 30.0f));
            caret.setPosition({caretX, cy - 15.0f});
            caret.setFillColor(sf::Color::White);
            target.draw(caret);
        }
    }
} // namespace Panels
