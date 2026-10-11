#include "Hud.h"
#include "UiKit.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <numbers>
#include <string>
#include <vector>

namespace Flappy
{
    namespace
    {
        constexpr float kPi = std::numbers::pi_v<float>;
        constexpr float kPillRight = 1244.0f; // BEST pill right edge (1280 - 36)
        constexpr float kStreakRight = 1236.0f; // STREAK tag right edge (1280 - 44)

        // Scales a color's alpha by k (0..1)
        sf::Color fade(sf::Color c, float k)
        {
            c.a = static_cast<std::uint8_t>(c.a * k + 0.5f);
            return c;
        }

        sf::FloatRect rectAt(float x, float y, float w, float h)
        {
            return sf::FloatRect(sf::Vector2f{x, y}, sf::Vector2f{w, h});
        }

        sf::FloatRect circleRect(sf::Vector2f c, float r)
        {
            return rectAt(c.x - r, c.y - r, 2.0f * r, 2.0f * r);
        }

        // Width of a single line of text with the given letter spacing
        float textWidth(const sf::Font &font, const std::string &text, unsigned int size, float spacing)
        {
            sf::Text t(font, text, size);
            t.setLetterSpacing(spacing);
            return t.getLocalBounds().size.x;
        }

        // Rounded rectangle with a vertical gradient (top -> bottom), rotated by rotationDeg around pivot.
        // SFML shapes cannot hold gradients, so this is a triangle fan with per-vertex colors.
        void fillRoundedGradient(sf::RenderTarget &target, sf::FloatRect r, float radius, sf::Color top,
                                 sf::Color bottom, float rotationDeg = 0.0f, sf::Vector2f pivot = {})
        {
            constexpr int kSteps = 8; // segments per corner
            radius = std::min(radius, 0.5f * std::min(r.size.x, r.size.y));

            const float x0 = r.position.x;
            const float y0 = r.position.y;
            const float x1 = x0 + r.size.x;
            const float y1 = y0 + r.size.y;
            const std::array<sf::Vector2f, 4> centers{{sf::Vector2f{x0 + radius, y0 + radius},
                                                       sf::Vector2f{x1 - radius, y0 + radius},
                                                       sf::Vector2f{x1 - radius, y1 - radius},
                                                       sf::Vector2f{x0 + radius, y1 - radius}}};
            const std::array<float, 4> startDeg{180.0f, 270.0f, 0.0f, 90.0f};

            std::vector<sf::Vector2f> outline;
            for (std::size_t c = 0; c < centers.size(); ++c)
            {
                for (int i = 0; i <= kSteps; ++i)
                {
                    const float a = (startDeg[c] + 90.0f * static_cast<float>(i) / kSteps) * kPi / 180.0f;
                    outline.push_back(centers[c] + sf::Vector2f{std::cos(a), std::sin(a)} * radius);
                }
            }

            const float span = std::max(r.size.y, 1.0f);
            sf::Vector2f centroid{0.0f, 0.0f};
            for (const auto &p : outline)
            {
                centroid += p;
            }
            centroid /= static_cast<float>(outline.size());

            const float rad = rotationDeg * kPi / 180.0f;
            const float cs = std::cos(rad);
            const float sn = std::sin(rad);
            auto place = [&](sf::Vector2f p)
            {
                const sf::Vector2f d = p - pivot;
                return sf::Vector2f{d.x * cs - d.y * sn, d.x * sn + d.y * cs} + pivot;
            };
            auto colorAt = [&](float y)
            { return mix(top, bottom, (y - y0) / span); };

            sf::VertexArray fan(sf::PrimitiveType::TriangleFan, outline.size() + 2);
            fan[0].position = place(centroid);
            fan[0].color = colorAt(centroid.y);
            for (std::size_t i = 0; i < outline.size(); ++i)
            {
                fan[i + 1].position = place(outline[i]);
                fan[i + 1].color = colorAt(outline[i].y);
            }
            fan[outline.size() + 1] = fan[1];
            target.draw(fan);
        }

        void fillTriangle(sf::RenderTarget &target, sf::Vector2f a, sf::Vector2f b, sf::Vector2f c, sf::Color color)
        {
            sf::ConvexShape tri(3);
            tri.setPoint(0, a);
            tri.setPoint(1, b);
            tri.setPoint(2, c);
            tri.setFillColor(color);
            target.draw(tri);
        }

        // Straight stroke between a and b (SFML has no line shape with thickness)
        void strokeSegment(sf::RenderTarget &target, sf::Vector2f a, sf::Vector2f b, float width, sf::Color color)
        {
            const sf::Vector2f d = b - a;
            const float len = std::hypot(d.x, d.y);
            if (len <= 0.0f)
            {
                return;
            }
            const sf::Vector2f n = sf::Vector2f{-d.y, d.x} * (width * 0.5f / len);
            sf::ConvexShape quad(4);
            quad.setPoint(0, a + n);
            quad.setPoint(1, b + n);
            quad.setPoint(2, b - n);
            quad.setPoint(3, a - n);
            quad.setFillColor(color);
            target.draw(quad);
        }

        // Gold crown icon with its top-left at `origin` (same outline as the mockup's 30x26 SVG)
        void drawCrown(sf::RenderTarget &target, sf::Vector2f origin, float k)
        {
            const std::array<sf::Vector2f, 7> p{{sf::Vector2f{3.0f, 22.0f}, sf::Vector2f{1.5f, 7.0f},
                                                 sf::Vector2f{9.0f, 13.0f}, sf::Vector2f{15.0f, 3.0f},
                                                 sf::Vector2f{21.0f, 13.0f}, sf::Vector2f{28.5f, 7.0f},
                                                 sf::Vector2f{27.0f, 22.0f}}};
            std::array<sf::Vector2f, 7> q{};
            for (std::size_t i = 0; i < q.size(); ++i)
            {
                q[i] = origin + p[i];
            }

            // The crown is concave, so it is filled as a union of convex triangles
            const sf::Color fill = fade(sf::Color(255, 194, 71), k);
            fillTriangle(target, q[0], q[1], q[2], fill);
            fillTriangle(target, q[0], q[2], q[4], fill);
            fillTriangle(target, q[2], q[3], q[4], fill);
            fillTriangle(target, q[0], q[4], q[6], fill);
            fillTriangle(target, q[4], q[5], q[6], fill);

            const sf::Color line = fade(sf::Color(184, 107, 12), k);
            for (std::size_t i = 0; i < q.size(); ++i)
            {
                strokeSegment(target, q[i], q[(i + 1) % q.size()], 2.0f, line);
            }
        }
    } // namespace

    Hud::Hud(const sf::Font &font) : m_font(&font)
    {
    }

    void Hud::setScore(int score)
    {
        if (score != m_score && score > 0)
        {
            m_pop = 1.0f;
        }
        m_score = score;
    }

    void Hud::setBest(int best)
    {
        m_best = best;
    }

    void Hud::setStreak(int streak)
    {
        m_streak = streak;
    }

    void Hud::update(float dt)
    {
        m_time += dt;
        m_pop = std::max(0.0f, m_pop - dt * 4.0f); // ~0.25 s
    }

    bool Hud::pauseHit(sf::Vector2f point) const
    {
        const sf::Vector2f d = point - sf::Vector2f{68.0f, 66.0f};
        return d.x * d.x + d.y * d.y <= 34.0f * 34.0f;
    }

    void Hud::draw(sf::RenderTarget &target, bool showScore, std::uint8_t alpha) const
    {
        const float k = alpha / 255.0f;
        const sf::Font &font = *m_font;

        // Hearts (lives)
        for (int i = 0; i < m_maxLives; ++i)
        {
            const sf::Vector2f c{150.0f + 44.0f * static_cast<float>(i), 66.0f};
            const bool full = i < m_lives;
            const sf::Color body = full ? sf::Color(255, 70, 110) : sf::Color(40, 30, 90);
            const sf::Color edge = full ? sf::Color(138, 20, 60) : sf::Color(120, 130, 190);
            const float pulse = (full && m_lives <= 1) ? 1.0f + 0.08f * std::sin(m_time * 9.0f) : 1.0f;
            const float r = 12.5f * pulse;

            sf::CircleShape lobe(r, 24);
            lobe.setOrigin(sf::Vector2f{r, r});
            lobe.setFillColor(fade(body, k));
            lobe.setOutlineThickness(3.0f);
            lobe.setOutlineColor(fade(edge, k));
            lobe.setPosition(c + sf::Vector2f{-r * 0.8f, -r * 0.35f});
            target.draw(lobe);
            lobe.setPosition(c + sf::Vector2f{r * 0.8f, -r * 0.35f});
            target.draw(lobe);

            sf::ConvexShape tip(3);
            tip.setPoint(0, c + sf::Vector2f{-r * 1.75f, -r * 0.1f});
            tip.setPoint(1, c + sf::Vector2f{r * 1.75f, -r * 0.1f});
            tip.setPoint(2, c + sf::Vector2f{0.0f, r * 1.9f});
            tip.setFillColor(fade(body, k));
            target.draw(tip);

            // Redraw the lobes' inner part to hide the seam, then a glossy highlight
            sf::CircleShape cover(r * 0.9f, 24);
            cover.setOrigin(sf::Vector2f{r * 0.9f, r * 0.9f});
            cover.setFillColor(fade(body, k));
            cover.setPosition(c + sf::Vector2f{-r * 0.8f, -r * 0.35f});
            target.draw(cover);
            cover.setPosition(c + sf::Vector2f{r * 0.8f, -r * 0.35f});
            target.draw(cover);
            if (full)
            {
                sf::CircleShape shine(r * 0.28f, 12);
                shine.setOrigin(sf::Vector2f{r * 0.28f, r * 0.28f});
                shine.setFillColor(fade(sf::Color(255, 255, 255, 190), k));
                shine.setPosition(c + sf::Vector2f{-r * 1.0f, -r * 0.65f});
                target.draw(shine);
            }
        }

        // Active player under the pause button
        if (!m_playerName.empty())
        {
            UiKit::drawLabel(target, font, m_playerName, 20, sf::Vector2f{68.0f, 117.0f}, fade(sf::Color(214, 224, 255), k), true, 1.0f);
        }

        // Big score with a faint spinning starburst behind it
        if (showScore)
        {
            const sf::Vector2f center{640.0f, 95.0f};
            UiKit::drawStarburst(target, center, 230.0f, 110.0f, 14, m_time * 4.0f,
                                 fade(sf::Color(255, 210, 74, 60), k), fade(sf::Color(255, 210, 74, 0), k));
            const float scale = 1.0f + 0.3f * m_pop * m_pop;
            UiKit::drawExtrudedText(target, font, std::to_string(m_score), 150, center, UiKit::orangeStyle(),
                                    scale, 0.0f, alpha);
        }

        // BEST pill (top-right), width follows its content
        {
            const float bestW = textWidth(font, "BEST", 15, 2.0f);
            const std::string bestNum = std::to_string(m_best);
            const float numW = textWidth(font, bestNum, 30, 1.0f);
            const float width = 18.0f + 30.0f + 12.0f + bestW + 12.0f + numW + 24.0f;
            const float left = kPillRight - width;
            const float top = 36.0f;

            UiKit::drawRoundedRect(target, rectAt(left, top + 7.0f, width, 56.0f), 28.0f,
                                   fade(sf::Color(6, 2, 40, 153), k));
            UiKit::drawRoundedRect(target, rectAt(left, top, width, 56.0f), 28.0f, fade(sf::Color(14, 8, 60, 150), k),
                                   fade(sf::Color(255, 255, 255, 100), k), 2.0f);

            drawCrown(target, sf::Vector2f{left + 18.0f, 51.0f}, k);

            const float bestX = left + 18.0f + 30.0f + 12.0f;
            UiKit::drawLabel(target, font, "BEST", 15, sf::Vector2f{bestX + bestW * 0.5f, 64.0f},
                      fade(sf::Color(207, 216, 255), k), false, 2.0f);

            UiKit::ExtrudeStyle bestStyle;
            bestStyle.face = sf::Color(255, 243, 176);
            bestStyle.layers = {sf::Color(184, 107, 12)};
            bestStyle.step = 3.0f;
            bestStyle.shadowAlpha = 0.0f;
            const float numX = bestX + bestW + 12.0f + numW * 0.5f;
            UiKit::drawExtrudedText(target, font, bestNum, 30, sf::Vector2f{numX, 63.0f}, bestStyle, 1.0f, 0.0f,
                                    alpha);
        }

        // Pink streak tag under the pill, tilted and bobbing
        if (m_streak >= 2)
        {
            const std::string text = "x" + std::to_string(m_streak) + " " + m_streakWord;
            const float w = textWidth(font, text, 22, 1.0f) + 44.0f;
            const float bob = std::sin(m_time * 3.0f) * 3.0f;
            const sf::Vector2f pivot{kStreakRight - w * 0.5f, 134.0f + bob};
            const float x = pivot.x - w * 0.5f;
            const float y = pivot.y - 22.0f;
            const float rot = -4.0f;
            const sf::Color dark = fade(sf::Color(138, 31, 85), k);

            // Soft shadow, edge, border, then the gradient face
            const sf::Color shadow = fade(sf::Color(5, 0, 50, 90), k);
            fillRoundedGradient(target, rectAt(x, y + 12.0f, w, 44.0f), 14.0f, shadow, shadow, rot, pivot);
            fillRoundedGradient(target, rectAt(x, y + 6.0f, w, 44.0f), 14.0f, dark, dark, rot, pivot);
            fillRoundedGradient(target, rectAt(x, y, w, 44.0f), 14.0f, dark, dark, rot, pivot);
            fillRoundedGradient(target, rectAt(x + 3.0f, y + 3.0f, w - 6.0f, 38.0f), 11.0f,
                                fade(sf::Color(255, 138, 184), k), fade(sf::Color(255, 63, 134), k), rot, pivot);

            UiKit::ExtrudeStyle streakStyle;
            streakStyle.face = sf::Color::White;
            streakStyle.layers = {sf::Color(138, 31, 85)};
            streakStyle.step = 2.0f;
            streakStyle.shadowAlpha = 0.0f;
            UiKit::drawExtrudedText(target, font, text, 22, pivot, streakStyle, 1.0f, rot, alpha);
        }

        // Round pause button (center 68, 66), lifts 3px and brightens on hover
        {
            const sf::Vector2f base{68.0f, 66.0f};
            const float lift = m_pauseHovered ? 3.0f : 0.0f;
            const sf::Vector2f c{base.x, base.y - lift};
            auto bright = [this](sf::Color col)
            { return m_pauseHovered ? mix(col, sf::Color::White, 0.12f) : col; };

            UiKit::drawGlow(target, sf::Vector2f{base.x, base.y + 16.0f}, 44.0f, fade(sf::Color(5, 0, 50, 120), k));

            const sf::Color edge = fade(sf::Color(111, 134, 214), k);
            fillRoundedGradient(target, circleRect(sf::Vector2f{c.x, c.y + 7.0f}, 32.0f), 32.0f, edge, edge);

            const sf::Color border = fade(bright(sf::Color(122, 155, 214)), k);
            fillRoundedGradient(target, circleRect(c, 32.0f), 32.0f, border, border);

            fillRoundedGradient(target, circleRect(c, 29.0f), 29.0f, fade(bright(sf::Color::White), k),
                                fade(bright(sf::Color(207, 224, 255)), k));

            const sf::Color navy = fade(sf::Color(27, 31, 92), k);
            UiKit::drawRoundedRect(target, rectAt(c.x - 8.0f, c.y - 9.0f, 6.0f, 18.0f), 2.5f, navy);
            UiKit::drawRoundedRect(target, rectAt(c.x + 2.0f, c.y - 9.0f, 6.0f, 18.0f), 2.5f, navy);
        }
    }
} // namespace Flappy
